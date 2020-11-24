#include "integrator.h"

#include <sampler/independent.h>

#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>
#include <tbb/concurrent_vector.h>

float Integrator::PowerHeuristic(float a, float b) const
{
	a *= a;
	b *= b;
	return a / (a + b);
}

void Integrator::DrawPoint(const Float3& p, const Spectrum& c)
{
	Float2 pScreen(Inverse(m_camera->m_screenToWorld).TransformPoint(p));
	m_buffers[0]->DrawCircle(pScreen.x, pScreen.y, 0.8, c);
}

void Integrator::DrawLine(const Float3& p, const Float3& q, const Spectrum& c)
{
	Float2 pScreen(Inverse(m_camera->m_screenToWorld).TransformPoint(p));
	Float2 qScreen(Inverse(m_camera->m_screenToWorld).TransformPoint(q));
	m_buffers[0]->DrawLine(pScreen, qScreen, c);
}

void Integrator::DrawBounds(const Bounds& bounds, const Spectrum& c)
{
	float xl = bounds.m_pMin.x, xr = bounds.m_pMax.x;
	float yl = bounds.m_pMin.y, yr = bounds.m_pMax.y;
	float zl = bounds.m_pMin.z, zr = bounds.m_pMax.z;
	DrawLine(Float3(xl, yl, zl), Float3(xr, yl, zl), c);
	DrawLine(Float3(xl, yr, zl), Float3(xr, yr, zl), c);
	DrawLine(Float3(xl, yl, zl), Float3(xl, yr, zl), c);
	DrawLine(Float3(xr, yl, zl), Float3(xr, yr, zl), c);

	DrawLine(Float3(xl, yl, zr), Float3(xr, yl, zr), c);
	DrawLine(Float3(xl, yr, zr), Float3(xr, yr, zr), c);
	DrawLine(Float3(xl, yl, zr), Float3(xl, yr, zr), c);
	DrawLine(Float3(xr, yl, zr), Float3(xr, yr, zr), c);

	DrawLine(Float3(xl, yl, zl), Float3(xl, yl, zr), c);
	DrawLine(Float3(xl, yr, zl), Float3(xl, yr, zr), c);
	DrawLine(Float3(xr, yl, zl), Float3(xr, yl, zr), c);
	DrawLine(Float3(xr, yr, zl), Float3(xr, yr, zr), c);
}

void Integrator::Save()
{
	m_buffers[0]->Save();
}

SampleIntegrator::~SampleIntegrator()
{
	if (m_renderThread->joinable()) {
		m_renderThread->join();
	}
}

void SampleIntegrator::Start()
{
	Setup();

	m_adaptiveTiles.clear();
	m_adaptiveTiles.push_back(std::pair<Framebuffer::Tile, bool>({ {0,0}, {m_buffers[0]->m_width, m_buffers[0]->m_height} }, false));

	m_tiles.clear();
	for (int j = 0; j < m_buffers[0]->m_height; j += tile_size) {
		for (int i = 0; i < m_buffers[0]->m_width; i += tile_size) {
			m_tiles.push_back({
				{i, j},
				{std::min(m_buffers[0]->m_width - i, tile_size), std::min(m_buffers[0]->m_height - j, tile_size)}
				});
		}
	}
	//std::reverse(m_tiles.begin(), m_tiles.end());

	// Initialize render status (start)
	m_rendering = true;
	m_timer.Start();
	// Add render thread
	/*
	m_renderThread = new std::thread(
		[this] {
			tbb::blocked_range<int> range(0, m_tiles.size());
			// Map : render tile
			auto map = [this](const tbb::blocked_range<int>& range) {
				for (int i = range.begin(); i < range.end(); ++i) {
					Framebuffer::Tile& tile = m_tiles[i];
					if (m_rendering) {
						RenderTile(tile, m_spp);
					}
				}
			};
			//map(range);
			tbb::parallel_for(range, map);

			// Initialize render status (stop)
			m_rendering = false;
			m_timer.Stop();
			m_buffers[0]->Save();
		}
	);
	*/

	m_renderThread = new std::thread(
		[this] {
			uint32_t step = 10;
			float thresholdTerminate = 0.0002;
			float thresholdSplit = 256 * thresholdTerminate;
			uint32_t A_block = m_buffers[0]->GetSize();
			uint32_t A_image = m_buffers[0]->GetSize();
			std::vector<bool> vis(A_block, true);
			for (uint32_t accSpp = 0; accSpp < m_spp; accSpp += step) {
				uint32_t spp = std::min(step, m_spp - accSpp);
				std::cout << accSpp << ' ' << step << ' ' << spp << std::endl;
				//m_buffers[1]->ClearOutputBuffer();
				// Map : render tile
				auto renderTilesMap = [&vis, step, accSpp, spp, this](const tbb::blocked_range<int>& range) {
					for (int idx = range.begin(); idx < range.end(); ++idx) {
						Framebuffer::Tile& tile = m_tiles[idx];
						if (!m_rendering) break;
						// Render tile
						IndependentSampler sampler;
						uint64_t s = (uint64_t)(tile.pos[1] * m_buffers[0]->m_width + tile.pos[0]) + uint64_t(m_buffers[0]->GetSize()) * accSpp;
						sampler.Setup(s);
						for (int j = 0; j < tile.res[1]; j++) {
							for (int i = 0; i < tile.res[0]; i++) {
								int x = i + tile.pos[0], y = j + tile.pos[1];
								int idx = y * m_buffers[0]->m_width + x;
								if (!vis[idx]) continue;
								for (uint32_t k = 0; k < spp; k++) {
									if (!m_rendering) break;
									Ray ray;
									m_camera->GenerateRay(Float2(x, y), sampler, ray);
									Spectrum radiance = Li(ray, sampler);
									m_buffers[0]->AddSample(x, y, radiance);
									if (k & 1) {
										m_buffers[1]->AddSample(x, y, radiance);
									}
								}
							}
						}
					}
				};
				tbb::blocked_range<int> range(0, m_tiles.size());
				//renderTilesMap(range);
				tbb::parallel_for(range, renderTilesMap);

				// Adaptive check
				std::atomic<uint32_t> _A_block = A_block;
				std::mutex mutex;
				float r = std::sqrt(float(A_block) / A_image);
				auto adaptiveMap = [&](const tbb::blocked_range<int>& range) {
					for (int idx = range.begin(); idx < range.end(); ++idx) {
						if (m_adaptiveTiles[idx].second) continue;
						Framebuffer::Tile& tile = m_adaptiveTiles[idx].first;
						int axis = tile.res[0] < tile.res[1];
						std::vector<float> metrics(tile.res[axis], 0.f);
						if (!m_rendering) break;
						float blockErrorSum = 0;
						for (int j = 0; j < tile.res[1]; j++) {
							for (int i = 0; i < tile.res[0]; i++) {
								if (!m_rendering) break;
								int x = i + tile.pos[0], y = j + tile.pos[1];
								sRGB I = m_buffers[0]->GetPixelSpectrum({ x,y });
								sRGB A = m_buffers[1]->GetPixelSpectrum({ x,y });
								float numerator = std::abs(I.r - A.r) + std::abs(I.g - A.g) + std::abs(I.b - A.b);
								float denominator = std::sqrt(0.0001f + I.r + I.g + I.b);
								float pixelError = numerator / denominator;
								if (isnan(pixelError)) {
									std::cout << "NAN: " << numerator << ' ' << denominator << std::endl;
								}
								blockErrorSum += pixelError;
								metrics[axis == 0 ? i : j] += pixelError;
							}
						}
						int N = tile.GetSize();
						float blockError = blockErrorSum * r / N;
						/*
						if (tile.pos[0] + tile.pos[1] == 0) {
							std::cout << idx << ' ' << blockError << ' ' << N << std::endl;
						}
						*/
						std::cout << idx << ' ' << blockError << std::endl;
						if (blockError < thresholdTerminate) {
							//std::cout << "Terminate : " << blockError << std::endl;
							m_adaptiveTiles[idx].second = true;
							_A_block -= tile.GetSize();
							for (int j = 0; j < tile.res[1]; j++) {
								for (int i = 0; i < tile.res[0]; i++) {
									if (!m_rendering) break;
									int x = i + tile.pos[0], y = j + tile.pos[1];
									int idx = y * m_buffers[0]->m_width + x;
									vis[idx] = false;
								}
							}
						}
						else if (blockError < thresholdSplit) {
							//std::cout << "Split : " << blockError << std::endl;
							m_adaptiveTiles[idx].second = true;
							float minDelta = std::abs(metrics[0] - (blockErrorSum - metrics[0]) / (tile.res[axis] - 1));
							float prefix = 0;
							int splitPos = 0;
							//std::cout << "axis: " << axis << std::endl;
							for (int i = 0; i + 1 < tile.res[axis]; i++) {								
								prefix += metrics[i];
								float delta = std::abs((prefix / (i + 1)) - ((blockErrorSum - prefix) / (tile.res[axis] - i - 1)));
								if (delta < minDelta) {
									splitPos = i;
									minDelta = delta;
								}
							}
							Framebuffer::Tile tile0, tile1;
							tile0.pos[0] = tile.pos[0], tile0.pos[1] = tile.pos[1];
							tile0.res[axis] = splitPos + 1, tile0.res[axis ^ 1] = tile.res[axis ^ 1];
							tile1.pos[axis] = tile.pos[axis] + splitPos + 1, tile1.pos[axis ^ 1] = tile.pos[axis ^ 1];
							tile1.res[axis] = tile.res[axis] - splitPos - 1, tile1.res[axis ^ 1] = tile.res[axis ^ 1];
							//std::cout << "splitPos: "<< splitPos << std::endl;
							//std::cout << "res: " << tile.res[axis] << std::endl;
							//std::cout << "tile0: " << tile0.pos[0] << ' ' << tile0.pos[1] << ' ' << tile0.res[0] << ' ' << tile0.res[1] << std::endl;
							//std::cout << "tile1: " << tile1.pos[0] << ' ' << tile1.pos[1] << ' ' << tile1.res[0] << ' ' << tile1.res[1] << std::endl;
							if (mutex.try_lock()) {   // only increase if currently not locked:
								m_adaptiveTiles.push_back(std::make_pair(tile0, false));
								m_adaptiveTiles.push_back(std::make_pair(tile1, false));
								mutex.unlock();
							}

							/*
							for (int j = 0; j < tile.res[1]; j++) {
								for (int i = 0; i < tile.res[0]; i++) {
									if (!m_rendering) break;
									int x = i + tile.pos[0], y = j + tile.pos[1];
									m_buffers[1]->ClearPixel({ x,y });
								}
							}
							*/
						}
						
						
						for (int j = 1; j + 1 < tile.res[1]; j++) {
							for (int i = 1; i + 1 < tile.res[0]; i++) {
								int x = i + tile.pos[0], y = j + tile.pos[1];
								m_buffers[0]->SetVal(x, y, Spectrum(1.f, 0, 0));
							}
						}						
					}
				};
				//std::cout << m_adaptiveTiles.size() << std::endl;
				range = tbb::blocked_range<int>(0, m_adaptiveTiles.size());
				m_buffers[0]->ClearDebugBuffer();
				//adaptiveMap(range);
				tbb::parallel_for(range, adaptiveMap);

				A_block = _A_block;
			}

			// Initialize render status (stop)
			m_rendering = false;
			m_timer.Stop();
			m_buffers[0]->Save();
		}
	);
}

void SampleIntegrator::Stop()
{
	m_rendering = false;
}

void SampleIntegrator::Wait()
{
	if (m_renderThread->joinable()) {
		m_renderThread->join();
	}
}

bool SampleIntegrator::IsRendering()
{
	return m_rendering;
}

void SampleIntegrator::RenderTile(const Framebuffer::Tile& tile)
{
	IndependentSampler sampler;
	unsigned long long s = (uint64_t)(tile.pos[1] * m_buffers[0]->m_width + tile.pos[0]) + uint64_t(m_buffers[0]->m_height * m_buffers[0]->m_height) * 0;
	sampler.Setup(s);

	for (int j = 0; j < tile.res[1]; j++) {
		for (int i = 0; i < tile.res[0]; i++) {
			for (uint32_t k = 0; k < m_spp; k++) {
				if (!m_rendering) {
					break;
				}
				int x = i + tile.pos[0], y = j + tile.pos[1];
				Ray ray;
				m_camera->GenerateRay(Float2(x, y), sampler, ray);
				Spectrum radiance = Li(ray, sampler);
				m_buffers[0]->AddSample(x, y, radiance);
			}
		}
	}
}

Spectrum SampleIntegrator::NormalCheck(Ray ray, Sampler& sampler)
{
	Spectrum radiance(0.f);
	HitRecord hitRec;
	bool hit = m_scene->Intersect(ray, hitRec);

	if (!hit) {
		return radiance;
	}

	Float2 st = hitRec.m_geoRec.m_st;
	//radiance = Spectrum(hitRec.m_geoRec.m_ng); return radiance;
	//radiance = Spectrum(st.x, st.y, 0); return radiance;

	float dotValue = Dot(-ray.d, hitRec.m_geoRec.m_ns);
	if (dotValue >= 0) {
		radiance = Spectrum(0.f, 0.f, 1.f) * dotValue;
	}
	else {
		radiance = Spectrum(1.f, 0.f, 0.f) * std::fabs(dotValue);
	}

	return radiance;
}