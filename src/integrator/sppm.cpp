#include "sppm.h"

#include "sampler/independent.h"

#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>
#include <tbb/concurrent_vector.h>

void SPPMIntegrator::Save()
{
    std::string suffix =
        fmt::format("{0}iteration_{1}_{2}_{3}_{4}",
            FormatNumber(m_currentIteration),
            FormatNumber(m_deltaPhotonNum),
            m_initialRadius,
            m_alpha,
            m_timer.ToString());
    m_buffer->Save(suffix);
}

std::string SPPMIntegrator::ToString() const
{
    return fmt::format("SPPM\niteration : {1}\n# photon : {2}",
        m_currentIteration, m_currentPhotonNum);
}

void SPPMIntegrator::InitializeGatherPoints()
{
    for (int j = 0; j < m_buffer->m_height; j += tile_size) {
        for (int i = 0; i < m_buffer->m_width; i += tile_size) {
            m_gatherBlocks.push_back(std::vector<GatherPoint>());
            std::vector<GatherPoint>& points = m_gatherBlocks.back();
            points.resize(std::min(m_buffer->m_width - i, tile_size) *
                std::min(m_buffer->m_height - j, tile_size));
            int index = 0;
            for (int xoffset = 0; xoffset < tile_size; xoffset++) {
                if (i + xoffset < m_buffer->m_width) {
                    break;
                }
                for (int yoffset = 0; yoffset < tile_size; yoffset++) {
                    if (j + yoffset < m_buffer->m_height) {
                        break;
                    }
                    points[index++].m_pos = Int2(i + xoffset, j + yoffset);
                }
            }
        }
    }
}

void SPPMIntegrator::PhotonPass(int index)
{
    // Initialize sampler
    IndependentSampler sampler;
    uint64_t seed = (uint64_t)m_currentIteration * m_deltaPhotonNum + index;
    sampler.Setup(seed);

    // Randomly pick an emitter
    uint32_t lightNum = m_scene->m_lights.size();
    uint32_t lightIdx = std::min(uint32_t(lightNum * sampler.Next1D()), lightNum - 1);
    const auto& light = m_scene->m_lights[lightIdx];

    // Sample photon
    Ray ray;
    Spectrum flux = light->SamplePhoton(sampler.Next2D(), sampler.Next2D(), ray);

    // Trace photon
    for (int bounce = 0; bounce < m_maxBounce; bounce++) {
        // Intersect test
        HitRecord hitRec;
        bool hit = m_scene->Intersect(ray, hitRec);
        if (!hit) {
            break;
        }

        // Store photon
        auto& bsdf = hitRec.m_primitive->m_bsdf;
        if (bsdf && !bsdf->IsDelta(hitRec.m_geoRec.m_st)) {
            Photon photon(hitRec.m_geoRec.m_p, -ray.d, flux);
            m_photonPlane.Add(photon);
        }

        // Hit medium bound
        if (!bsdf) {
            ray = Ray(hitRec.m_geoRec.m_p, ray.d);
            bounce--;
            continue;
        }

        // Scatter photon
        /// Sample BSDF
        MaterialRecord matRec(-ray.d, hitRec.m_geoRec.m_ns, hitRec.m_geoRec.m_st, Importance);
        flux *= bsdf->Sample(matRec, sampler.Next2D());
        Float3 dir = matRec.ToWorld(matRec.m_wo);
        ray = Ray(hitRec.m_geoRec.m_p, dir, hitRec.GetMedium(dir));

        // Russian roulette
        if (bounce > 5) {
            float q = std::min(0.99f, MaxComponent(flux));
            if (sampler.Next1D() > q) {
                break;
            }
            flux /= q;
        }
    }
}

void SPPMIntegrator::BuildPhotonMap()
{
    m_photonPlane.Build();
}

void SPPMIntegrator::CameraPass(int index)
{
    std::vector<GatherPoint>& block = m_gatherBlocks[index];
    // Initialize sampler
    IndependentSampler sampler;
    uint64_t seed = (block[0].m_pos.y * m_buffer->m_width + block[0].m_pos.x) +
        m_currentIteration * (m_buffer->m_width * m_buffer->m_height);
    sampler.Setup(seed);

    // Trace ray
    for (GatherPoint& gp : block) {
        // Generate ray
        Ray ray;
        m_camera->GenerateRay(gp.m_pos, sampler, ray);

        // Estimate radiance
        Spectrum throughput(1.f);
        float eta = 1.f;
        for (int bounce = 0;; bounce++) {
            bool hit = m_scene->Intersect(ray, gp.m_hitRec);
            gp.m_emission += throughput * m_scene->EvalLight(hit, ray, gp.m_hitRec);
            
            // No hit
            if (!hit) {
                break;
            }

            // Estimate radiance
            auto& bsdf = gp.m_hitRec.m_primitive->m_bsdf;
            if (bsdf && !bsdf->IsDelta(gp.m_hitRec.m_geoRec.m_st)) {
                gp.m_throughphut = throughput;
                break;
            }

            // Hit medium bound
            if (!bsdf) {
                ray = Ray(gp.m_hitRec.m_geoRec.m_p, ray.d);
                bounce--;
                continue;
            }

            // Scatter 
            ///Sample BSDF
            MaterialRecord matRec(-ray.d, gp.m_hitRec.m_geoRec.m_ns, gp.m_hitRec.m_geoRec.m_st);
            Spectrum bsdfVal = bsdf->Sample(matRec, sampler.Next2D());
            if (bsdfVal.IsBlack()) {
                break;
            }
            Float3 dir = matRec.ToWorld(matRec.m_wo);
            ray = Ray(gp.m_hitRec.m_geoRec.m_p, dir, gp.m_hitRec.GetMedium(dir));
            throughput *= bsdfVal;
        }
    }
}

void SPPMIntegrator::Update()
{
    auto job = [this](const tbb::blocked_range<int>& range) {
        for (int i = range.begin(); i < range.end(); ++i) {
            if (!m_rendering) {
                break;
            }

            for (GatherPoint& gp : m_gatherBlocks[i]) {
                Spectrum flux = gp.m_throughphut * EstimatePlane(gp.m_hitRec, gp.m_radius);
            }
        }
    };
    tbb::blocked_range<int> tileRange(0, m_gatherBlocks.size());
    tbb::parallel_for(tileRange, job);


    m_photonPlane.Clear();
    m_currentPhotonNum += m_deltaPhotonNum;
}

Spectrum SPPMIntegrator::EstimatePlane(const HitRecord& hitRec, float radius)
{
    std::shared_ptr<BSDF> bsdf = hitRec.GetBSDF();
    std::vector<const Photon*> gatheredPhotons;
    m_photonPlane.Query(hitRec.m_geoRec.m_p, radius, gatheredPhotons);
    Spectrum sum(0.f);
    for (const auto& photon : gatheredPhotons) {
        MaterialRecord matRec(hitRec.m_wi, photon->m_direction, hitRec.m_geoRec.m_ns, hitRec.m_geoRec.m_st);
        Spectrum bsdfVal = bsdf->Eval(matRec);
        sum += bsdfVal / Frame::AbsCosTheta(matRec.m_wo) * photon->m_flux;
    }
    float area = M_PI * radius * radius;
    Spectrum radiance = sum / (area * m_deltaPhotonNum);
    return radiance;
}

void SPPMIntegrator::Start()
{
    Setup();

    // Initialize gather points
    InitializeGatherPoints();

    // Initialize render status (start)
    m_rendering = true;
    m_timer.Start();
    // Add render thread
    m_renderThread = new std::thread(
        [this] {
            auto RunPhotonPass = [this]() {
                auto job = [this](const tbb::blocked_range<int>& range) {
                    for (int i = range.begin(); i < range.end(); ++i) {
                        if (m_rendering) {
                            PhotonPass(i);
                        }
                    }
                };
                tbb::blocked_range<int> photonRange(0, m_deltaPhotonNum);
                tbb::parallel_for(photonRange, job);
            };

            auto RunCameraPass = [this]() {
                auto job = [this](const tbb::blocked_range<int>& range) {
                    for (int i = range.begin(); i < range.end(); ++i) {
                        if (m_rendering) {
                            CameraPass(i);
                        }
                    }
                };
                tbb::blocked_range<int> tileRange(0, m_gatherBlocks.size());
                tbb::parallel_for(tileRange, job);
            };


            // Iteration
            m_currentPhotonNum = 0;
            for (m_currentIteration = 0; m_currentIteration < m_maxIteration && m_rendering; m_currentIteration++) {
                if (!m_rendering) {
                    break;
                }
                // Photon pass
                RunPhotonPass();

                // Construct photon structure
                BuildPhotonMap();

                // Camera pass
                RunCameraPass();

                // Update
                Update();                
            }

            // Initialize render status (stop)
            m_rendering = false;
            m_timer.Stop();
        }
    );
}


void SPPMIntegrator::Debug(DebugRecord& debugRec)
{
    if (debugRec.m_debugRay) {
        Float2 pos = debugRec.m_rasterPosition;
        if (pos.x >= 0 && pos.x < m_buffer->m_width &&
            pos.y >= 0 && pos.y < m_buffer->m_height)
        {
            int x = pos.x, y = m_buffer->m_height - pos.y;
            Sampler sampler;
            unsigned int s = y * m_buffer->m_width + x;
            sampler.Setup(s);
            Ray ray;
            m_camera->GenerateRay(Float2(x, y), sampler, ray);
            LiDebug(ray, sampler);
        }
    }
}

Spectrum SPPMIntegrator::LiDebug(Ray ray, Sampler& sampler)
{
    return Spectrum(0.f);
}