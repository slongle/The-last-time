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
	m_buffer->DrawCircle(pScreen.x, pScreen.y, 0.8, c);
}

void Integrator::DrawLine(const Float3& p, const Float3& q, const Spectrum& c)
{
	Float2 pScreen(Inverse(m_camera->m_screenToWorld).TransformPoint(p));
	Float2 qScreen(Inverse(m_camera->m_screenToWorld).TransformPoint(q));
	m_buffer->DrawLine(pScreen, qScreen, c);
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
	m_buffer->Save();
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

	m_tiles.clear();
	for (int j = 0; j < m_buffer->m_height; j += tile_size) {
		for (int i = 0; i < m_buffer->m_width; i += tile_size) {
			m_tiles.push_back({
				{i, j},
				{std::min(m_buffer->m_width - i, tile_size), std::min(m_buffer->m_height - j, tile_size)}
				});
		}
	}
	//std::reverse(m_tiles.begin(), m_tiles.end());
	m_samplers.resize(m_tiles.size());
	for (int i = 0; i < m_samplers.size(); i++) {
		uint64_t s = m_tiles[i].pos[1] * m_buffer->m_width + m_tiles[i].pos[0];
		m_samplers[i].Setup(s);
	}

	// Initialize render status (start)
	m_rendering = true;
	m_timer.Start();
	// Add render thread
	m_renderThread = new std::thread(
		[this] {
			tbb::blocked_range<int> range(0, m_tiles.size());
			// Map : render tile
			int step = 50;
			auto map = [step, this](const tbb::blocked_range<int>& range) {
				for (int i = range.begin(); i < range.end(); ++i) {
					Framebuffer::Tile& tile = m_tiles[i];
					if (m_rendering) {
						RenderTile(tile, step, m_samplers[i]);
					}
				}
			};
			for (m_accSpp = 0; m_accSpp < m_spp; m_accSpp += step) {
				if (!m_rendering) break;
				//map(range);
				tbb::parallel_for(range, map);
				m_buffer->Save(fmt::format("{}", m_accSpp));
			}

			// Initialize render status (stop)
			m_rendering = false;
			m_timer.Stop();
			m_buffer->Save();
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

void SampleIntegrator::RenderTile(const Framebuffer::Tile& tile, int spp, IndependentSampler& sampler)
{	
	for (int j = 0; j < tile.res[1]; j++) {
		for (int i = 0; i < tile.res[0]; i++) {
			for (uint32_t k = 0; k < spp; k++) {
				if (!m_rendering) {
					break;
				}
				int x = i + tile.pos[0], y = j + tile.pos[1];
				Ray ray;
				m_camera->GenerateRay(Float2(x, y), sampler, ray);
				Spectrum radiance = Li(ray, sampler);
				m_buffer->AddSample(x, y, radiance);
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