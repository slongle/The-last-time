#include "vppm.h"

#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>
#include <tbb/concurrent_vector.h>

void VPPMIntegrator::Start()
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

    // Initialize render status (start)
    m_rendering = true;
    m_timer.Start();
    // Add render thread
    m_renderThread = new std::thread(
        [this] {
            // Photon pass            
            auto photonPass = [this](const tbb::blocked_range<int>& range) {
                for (int i = range.begin(); i < range.end(); ++i) {    
                    if (m_rendering) {                        
                        EmitPhoton(i);
                    }
                }
            };

            // Camera pass            
            auto cameraPass = [this](const tbb::blocked_range<int>& range) {
                for (int i = range.begin(); i < range.end(); ++i) {
                    Framebuffer::Tile& tile = m_tiles[i];
                    if (m_rendering) {
                        RenderTile(tile);
                    }
                }
            };

            // Iteration
            m_currentRadius = m_initialRadius;
            m_currentPhotonNum = 0;
            for (m_currentIteration = 0; m_currentIteration < m_maxIteration && m_rendering; m_currentIteration++) {            
                if (!m_rendering) {
                    break;
                }                
                // Photon pass
                tbb::blocked_range<int> photonRange(0, m_deltaPhotonNum);
                tbb::parallel_for(photonRange, photonPass);
                //photonPass(photonRange);

                // Construct photon structure
                m_photonTree.Build();

                // Camera pass
                tbb::blocked_range<int> tileRange(0, m_tiles.size());
                tbb::parallel_for(tileRange, cameraPass);
                //cameraPass(tileRange);

                // Update settings
                if (m_currentIteration + 1 != m_maxIteration) {
                    m_photonTree.Clear();
                }
                m_currentPhotonNum += m_deltaPhotonNum;
                m_currentRadius = std::sqrt((m_currentIteration + m_alpha) / (m_currentIteration + 1)) * 
                    m_currentRadius;
            }

            // Initialize render status (stop)
            m_rendering = false;
            m_timer.Stop();
            m_buffer->Save();
        }
    );
}

std::string VPPMIntegrator::ToString() const
{
    return fmt::format("VPPM\nnmax bounce : {0}\niteration : {1}\n# photon : {2}\nradius : {3}",
        m_maxBounce, m_currentIteration, m_currentPhotonNum, m_currentRadius);
}

void VPPMIntegrator::RenderTile(const Framebuffer::Tile& tile)
{
    Sampler sampler;
    uint64_t s = (tile.pos[1] * m_buffer->m_width + tile.pos[0]) +
        m_currentIteration * (m_buffer->m_width * m_buffer->m_height);
    sampler.Setup(s);

    for (int j = 0; j < tile.res[1]; j++) {
        for (int i = 0; i < tile.res[0]; i++) {
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

void VPPMIntegrator::EmitPhoton(const uint32_t& photonIndex)
{
    if (!m_rendering) {
        return;
    }
    // Initialize sampler
    Sampler sampler;
    uint64_t seed = (uint64_t)m_currentIteration * m_deltaPhotonNum + photonIndex;
    sampler.Setup(seed);
    // Randomly pick an emitter
    uint32_t lightNum = m_scene->m_lights.size();
    uint32_t lightIdx = std::min(uint32_t(lightNum * sampler.Next1D()), lightNum - 1);
    const auto& light = m_scene->m_lights[lightIdx];
    // Sample photon
    Ray ray;
    Spectrum flux = light->SamplePhoton(sampler.Next2D(), sampler.Next2D(), ray);
    flux *= lightNum;
    // Trace photon
    for (uint32_t bounce = 0; bounce < m_maxBounce; bounce++) {
        // Intersect test
        HitRecord hitRec;
        bool hit = m_scene->Intersect(ray, hitRec);
        //DrawLine(ray.o, hitRec.m_geoRec.m_p, Spectrum(1, 0, 0));
        if (!hit) {
            break;
        }
        auto& bsdf = hitRec.m_primitive->m_bsdf;
        bool isDelta = bsdf->IsDelta(hitRec.m_geoRec.m_st);
        // Store photon
        Photon photon(hitRec.m_geoRec.m_p, -ray.d, flux);
        if (!isDelta) {
            m_photonTree.Add(photon);
        }
        // Scatter photon
        MaterialRecord matRec(-ray.d, hitRec.m_geoRec.m_ns, hitRec.m_geoRec.m_st, Importance);
        Spectrum bsdfVal = bsdf->Sample(matRec, sampler.Next2D());
        flux *= bsdfVal;
        ray = Ray(hitRec.m_geoRec.m_p, matRec.ToWorld(matRec.m_wo));
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

Spectrum VPPMIntegrator::Li(Ray ray, Sampler& sampler)
{
    Spectrum radiance(0.f);
    Spectrum throughput(1.f);
    float eta = 1.f;
    HitRecord hitRec;
    for (uint32_t bounce = 0; bounce < m_maxBounce; bounce++) {
        bool hit = m_scene->Intersect(ray, hitRec);
        radiance += throughput * m_scene->EvalLight(hit, ray, hitRec);

        if (!hit) {
            break;
        }

        // Get basf
        auto& bsdf = hitRec.m_primitive->m_bsdf;
        bool isDelta = bsdf->IsDelta(hitRec.m_geoRec.m_st);

        // Estimate Li
        if (!isDelta) {
            std::vector<Photon> gatheredPhotons;
            m_photonTree.Query(hitRec.m_geoRec.m_p, m_currentRadius, gatheredPhotons);
            Spectrum sum(0.f);
            for (const auto& photon : gatheredPhotons) {
                MaterialRecord matRec(-ray.d, photon.m_direction, hitRec.m_geoRec.m_ns, hitRec.m_geoRec.m_st);
                Spectrum bsdfVal = bsdf->Eval(matRec);
                sum += bsdfVal / Frame::AbsCosTheta(matRec.m_wo) * photon.m_flux;
            }
            float area = M_PI * m_currentRadius * m_currentRadius;
            radiance += throughput * sum / (area * m_deltaPhotonNum);
            break;
        }

        // Sample BSDF            
        MaterialRecord matRec(-ray.d, hitRec.m_geoRec.m_ns, hitRec.m_geoRec.m_st);
        Spectrum bsdfVal = bsdf->Sample(matRec, sampler.Next2D());
        ray = Ray(hitRec.m_geoRec.m_p, matRec.ToWorld(matRec.m_wo));
        if (bsdfVal.IsBlack()) {
            break;
        }
        // Update throughput
        throughput *= bsdfVal;

        // Russian roulette
        if (bounce > 5) {
            float q = std::min(0.99f, MaxComponent(throughput));
            if (sampler.Next1D() > q) {
                break;
            }
            throughput /= q;
        }
    }

    return radiance;
}

