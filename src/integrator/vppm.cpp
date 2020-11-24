#include "vppm.h"

#include "sampler/independent.h"

#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>
#include <tbb/concurrent_vector.h>

void VPPMIntegrator::Save()
{
    std::string suffix =
        fmt::format("{0}iteration_{1}_{2}_{3}_{4}",
            FormatNumber(m_currentIteration),
            FormatNumber(m_deltaPhotonNum),
            m_initialRadius,
            m_alpha,
            m_timer.ToString());
    m_buffers[0]->Save(suffix);
}

std::string VPPMIntegrator::ToString() const
{
    return fmt::format("VPPM\nnmax bounce : {0}\niteration : {1}\n# photon : {2}\nradius : {3}",
        m_maxBounce, m_currentIteration, m_currentPhotonNum, m_currentRadius);
}

void VPPMIntegrator::Start()
{
    Setup();

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
#ifdef _DEBUG
                //photonPass(photonRange);
                tbb::parallel_for(photonRange, photonPass);
#else
                //photonPass(photonRange);
                tbb::parallel_for(photonRange, photonPass);
#endif // _DEBUG


                // Construct photon structure
                m_photonMedium.Build(m_currentRadius);
                m_photonPlane.Build();

                // Camera pass
                tbb::blocked_range<int> tileRange(0, m_tiles.size());
#ifdef _DEBUG
                //cameraPass(tileRange);
                tbb::parallel_for(tileRange, cameraPass);
#else
                tbb::parallel_for(tileRange, cameraPass);
#endif // _DEBUG


                // Update settings
                if (m_currentIteration + 1 != m_maxIteration) {
                    m_photonMedium.Clear();
                    m_photonPlane.Clear();
                }
                m_currentPhotonNum += m_deltaPhotonNum;
                m_currentRadius = std::sqrt((m_currentIteration + m_alpha) / (m_currentIteration + 1)) *
                    m_currentRadius;
            }

            // Initialize render status (stop)
            m_rendering = false;
            m_timer.Stop();
            //m_buffers[0]->Save();
        }
    );
}

void VPPMIntegrator::RenderTile(const Framebuffer::Tile& tile)
{
    IndependentSampler sampler;
    uint64_t s = (tile.pos[1] * m_buffers[0]->m_width + tile.pos[0]) +
        m_currentIteration * (m_buffers[0]->m_width * m_buffers[0]->m_height);
    sampler.Setup(s);

    for (int j = 0; j < tile.res[1]; j++) {
        for (int i = 0; i < tile.res[0]; i++) {
            for (int k = 0; k < 16; k++) {
                if (!m_rendering) {
                    break;
                }
                int x = i + tile.pos[0], y = j + tile.pos[1];
                Ray ray;
                m_camera->GenerateRay(Float2(x, y), sampler, ray);
#ifdef _DEBUG
                Spectrum radiance = Li(ray, sampler);
                //Spectrum radiance = LiDebug(ray, sampler);
#else
                Spectrum radiance = Li(ray, sampler);
#endif // _DEBUG
                m_buffers[0]->AddSample(x, y, radiance);
            }
        }
    }
}

void VPPMIntegrator::EmitPhoton(const uint32_t& photonIndex)
{
    if (!m_rendering) {
        return;
    }
    // Initialize sampler
    IndependentSampler sampler;
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
    for (int bounce = 0; bounce < m_maxBounce; bounce++) {
        // Intersect test
        HitRecord hitRec;
        bool hit = m_scene->Intersect(ray, hitRec);
        if (!hit) {
            break;
        }
        // Sample medium
        MediumRecord mediumRec;
        if (ray.m_medium) {
            flux *= ray.m_medium->Sample(ray, mediumRec, sampler);
        }
        // Store photon
        auto& bsdf = hitRec.m_primitive->m_bsdf;
        if (mediumRec.m_internal) {
            Photon photon(mediumRec.m_p, -ray.d, flux);
            m_photonMedium.Add(photon);
        }
        else if (bsdf && !bsdf->IsDelta(hitRec.m_geoRec.m_st)) {
            Photon photon(hitRec.m_geoRec.m_p, -ray.d, flux);
            m_photonPlane.Add(photon);
        }
        // Hit medium bound
        if (!mediumRec.m_internal && !bsdf) {
            auto medium = hitRec.GetMedium(ray.d);
            ray = Ray(hitRec.m_geoRec.m_p, ray.d, medium);

            bounce--;
            continue;
        }
        // Scatter photon
        if (mediumRec.m_internal) {
            // Sample phase function
            auto& phase = ray.m_medium->m_phaseFunction;
            PhaseFunctionRecord phaseRec(-ray.d);
            Spectrum phaseVal = phase->Sample(phaseRec, sampler.Next2D());
            flux *= phaseVal;
            ray = Ray(mediumRec.m_p, phaseRec.m_wo, ray.m_medium);
        }
        else {
            // Sample BSDF
            MaterialRecord matRec(-ray.d, hitRec.m_geoRec.m_ns, hitRec.m_geoRec.m_st, Importance);
            Spectrum bsdfVal = bsdf->Sample(matRec, sampler.Next2D());
            flux *= bsdfVal;
            Float3 dir = matRec.ToWorld(matRec.m_wo);
            ray = Ray(hitRec.m_geoRec.m_p, dir, hitRec.GetMedium(dir));
        }
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
    for (int bounce = 0; bounce < m_maxBounce; bounce++) {
        bool hit = m_scene->Intersect(ray, hitRec);
        radiance += throughput * m_scene->EvalLight(hit, ray, hitRec);
        // No hit
        if (!hit) {
            break;
        }
        // Sample medium
        MediumRecord mediumRec;
        Spectrum mediumTr(1.f);
        if (ray.m_medium) {
            mediumTr = ray.m_medium->Sample(ray, mediumRec, sampler);
        }
        // Estimate radiance
        auto& bsdf = hitRec.m_primitive->m_bsdf;
        auto& phase = ray.m_medium->m_phaseFunction;
        if (mediumRec.m_internal) {
            //radiance += throughput * EstimateMediumBeam3D(ray, mediumRec, phase, sampler);
            radiance += mediumTr * throughput * EstimateMediumPoint3D(ray, mediumRec, phase);            
            break;
        }
        else if (bsdf && !bsdf->IsDelta(hitRec.m_geoRec.m_st)) {
            radiance += mediumTr * throughput * EstimatePlane(ray, hitRec, bsdf);            
            break;
        }
        throughput *= mediumTr;
        // Hit medium bound
        if (!bsdf) {
            auto medium = hitRec.GetMedium(ray.d);
            ray = Ray(hitRec.m_geoRec.m_p, ray.d, medium);

            bounce--;
            continue;
        }
        // Scatter 
        if (mediumRec.m_internal) {
            // Sample phase function
            PhaseFunctionRecord phaseRec(-ray.d);
            Spectrum phaseVal = phase->Sample(phaseRec, sampler.Next2D());
            ray = Ray(mediumRec.m_p, phaseRec.m_wo, ray.m_medium);
            throughput *= phaseVal;
        }
        else {
            //Sample BSDF
            MaterialRecord matRec(-ray.d, hitRec.m_geoRec.m_ns, hitRec.m_geoRec.m_st);
            Spectrum bsdfVal = bsdf->Sample(matRec, sampler.Next2D());
            if (bsdfVal.IsBlack()) {
                break;
            }
            Float3 dir = matRec.ToWorld(matRec.m_wo);
            ray = Ray(hitRec.m_geoRec.m_p, dir, hitRec.GetMedium(dir));
            throughput *= bsdfVal;
        }

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

Spectrum VPPMIntegrator::EstimateMediumBeam3D(
    const Ray& ray, 
    const MediumRecord& mediumRec, 
    const std::shared_ptr<PhaseFunction>& phase,
    Sampler& sampler)
{
    Ray beam(ray.o, ray.d, 0, mediumRec.m_t);
    std::vector<std::pair<const Photon*, float>> gatheredPhotons;
    m_photonMedium.QueryBeam(beam, m_currentRadius, gatheredPhotons);
    Spectrum sum(0.f);
    for (const auto& p : gatheredPhotons) {
        const auto& photon = p.first;
        const auto& t = p.second;
        // Estimate Tr
        Spectrum Tr = ray.m_medium->Transmittance(Ray(ray.o, ray.d, 0, t), sampler);
        // Evaluate phase function
        PhaseFunctionRecord phaseRec(-ray.d, photon->m_direction);
        Spectrum phaseVal = phase->EvalPdf(phaseRec);
        // Add contribution
        sum += Tr * phaseVal * photon->m_flux;
    }
    float num = gatheredPhotons.empty() ? 1 : gatheredPhotons.size();
    float vol = 4.f / 3.f * M_PI * m_currentRadius * m_currentRadius * m_currentRadius;
    Spectrum radiance = sum / (vol * m_deltaPhotonNum * 25);
    return radiance;
}

Spectrum VPPMIntegrator::EstimateMediumPoint3D(
    const Ray& ray,
    const MediumRecord& mediumRec,
    const std::shared_ptr<PhaseFunction>& phase)
{
    std::vector<const Photon*> gatheredPhotons;
    m_photonMedium.Query(mediumRec.m_p, m_currentRadius, gatheredPhotons);
    Spectrum sum(0.f);
    for (const auto& photon : gatheredPhotons) {
        PhaseFunctionRecord phaseRec(-ray.d, photon->m_direction);
        Spectrum phaseVal = phase->EvalPdf(phaseRec);
        sum += phaseVal * photon->m_flux;
    }
    float vol = 4.f / 3.f * M_PI * m_currentRadius * m_currentRadius * m_currentRadius;
    Spectrum radiance = sum / (vol * m_deltaPhotonNum);
    return radiance;
}

Spectrum VPPMIntegrator::EstimatePlane(
    const Ray& ray,
    const HitRecord& hitRec,
    const std::shared_ptr<BSDF>& bsdf)
{
    std::vector<const Photon*> gatheredPhotons;
    m_photonPlane.Query(hitRec.m_geoRec.m_p, m_currentRadius, gatheredPhotons);
    Spectrum sum(0.f);
    for (const auto& photon : gatheredPhotons) {
        MaterialRecord matRec(-ray.d, photon->m_direction, hitRec.m_geoRec.m_ns, hitRec.m_geoRec.m_st);
        Spectrum bsdfVal = bsdf->Eval(matRec);
        sum += bsdfVal / Frame::AbsCosTheta(matRec.m_wo) * photon->m_flux;
    }
    float area = M_PI * m_currentRadius * m_currentRadius;
    Spectrum radiance = sum / (area * m_deltaPhotonNum);
    return radiance;
}

void VPPMIntegrator::Debug(DebugRecord& debugRec)
{
    if (debugRec.m_debugRay) {
        Float2 pos = debugRec.m_rasterPosition;
        if (pos.x >= 0 && pos.x < m_buffers[0]->m_width &&
            pos.y >= 0 && pos.y < m_buffers[0]->m_height)
        {
            int x = pos.x, y = m_buffers[0]->m_height - pos.y;
            Sampler sampler;
            unsigned int s = y * m_buffers[0]->m_width + x;
            sampler.Setup(s);
            Ray ray;
            m_camera->GenerateRay(Float2(x, y), sampler, ray);
            LiDebug(ray, sampler);
        }
    }
}

Spectrum VPPMIntegrator::LiDebug(Ray ray, Sampler& sampler)
{
    Spectrum radiance(0.f);
    Spectrum throughput(1.f);
    float eta = 1.f;
    HitRecord hitRec;
    for (int bounce = 0; bounce < m_maxBounce; bounce++) {
        bool hit = m_scene->Intersect(ray, hitRec);
        // No hit
        if (!hit) {
            break;
        }
        auto& bsdf = hitRec.m_primitive->m_bsdf;
        // Sample medium
        MediumRecord mediumRec;
        if (ray.m_medium) {
            throughput *= ray.m_medium->Sample(ray, mediumRec, sampler);
        }
        if (!mediumRec.m_internal) {
            // Estimate radiance
            std::vector<Photon> gatheredPhotons;
            m_photonMedium.Query(mediumRec.m_p, m_currentRadius, gatheredPhotons);
            Spectrum sum(0.f);
            for (const auto& photon : gatheredPhotons) {
                sum += photon.m_flux;
            }
            float vol = 4.f / 3.f * M_PI * m_currentRadius * m_currentRadius * m_currentRadius;
            radiance += sum / (vol * m_deltaPhotonNum);
        }
        else {
            // Hit medium bound
            if (!bsdf) {
                auto medium = hitRec.GetMedium(ray.d);
                ray = Ray(hitRec.m_geoRec.m_p, ray.d, medium);

                bounce--;
                continue;
            }
            // Estimate radiance
            std::vector<Photon> gatheredPhotons;
            m_photonPlane.Query(hitRec.m_geoRec.m_p, m_currentRadius, gatheredPhotons);
            Spectrum sum(0.f);
            for (const auto& photon : gatheredPhotons) {
                sum += photon.m_flux;
            }
            float area = M_PI * m_currentRadius * m_currentRadius;
            radiance += sum / (area * m_deltaPhotonNum);
        }


        break;
    }
    return radiance;
}


