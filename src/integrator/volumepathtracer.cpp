#include "volumepathtracer.h"
#include "light/environment.h"

Spectrum VolumePathIntegrator::Li(Ray ray, Sampler& sampler)
{
    Spectrum radiance(0.f);
    Spectrum throughput(1.f);
    float eta = 1.f;
    HitRecord hitRec;
    bool hit = m_scene->Intersect(ray, hitRec);
    for (uint32_t bounce = 0; bounce < m_maxBounce; bounce++) {
        // Sample medium
        MediumRecord mediumRec;
        if (hit && ray.m_medium) {
            throughput *= ray.m_medium->Sample(ray, mediumRec, sampler);
        }
        if (throughput.IsBlack()) {
            break;
        }
        // Medium internal
        if (mediumRec.m_internal) {
            if (!hit) {
                break;
            }

            auto& phase = ray.m_medium->m_phaseFunction;

            // Sample light
            {
                LightRecord lightRec(mediumRec.m_p);
                Spectrum emission = m_scene->SampleLight(lightRec, sampler.Next2D(), ray.m_medium);
                if (!emission.IsBlack()) {
                    // Allocate a record for querying the phase function
                    PhaseFunctionRecord phaseRec(-ray.d, lightRec.m_wi);
                    // Evaluate phase and pdf
                    Spectrum phaseVal = phase->EvalPdf(phaseRec);
                    if (!phaseVal.IsBlack()) {
                        // Weight using the power heuristic
                        float weight = PowerHeuristic(lightRec.m_pdf, phaseRec.m_pdf);
                        radiance += throughput * phaseVal * emission * weight;
                    }
                }
            }

            // Sample phase function
            {
                // Sample phase / pdf
                PhaseFunctionRecord phaseRec(-ray.d);
                Spectrum phaseVal = phase->Sample(phaseRec, sampler.Next2D());
                auto medium = ray.m_medium;
                ray = Ray(mediumRec.m_p, phaseRec.m_wo, ray.m_medium);
                // Update throughput
                throughput *= phaseVal;
                // Eval light                
                Spectrum transmittance;
                hit = m_scene->IntersectTr(ray, hitRec, transmittance);
                ray = Ray(mediumRec.m_p, phaseRec.m_wo, medium);
                LightRecord lightRec;
                Spectrum emission = m_scene->EvalPdfLight(hit, ray, hitRec, lightRec);
                emission *= transmittance;
                if (!emission.IsBlack()) {
                    // Weight using the power heuristic
                    float weight = PowerHeuristic(phaseRec.m_pdf, lightRec.m_pdf);
                    radiance += throughput * emission * weight;
                }

                ray = Ray(mediumRec.m_p, phaseRec.m_wo, medium);
                hit = m_scene->Intersect(ray, hitRec);
            }
        }
        // Medium external
        else {
            // Eval direct light at first bounce
            if (bounce == 0) {
                radiance += throughput * m_scene->EvalLight(hit, ray, hitRec);
            }
            // No hit
            if (!hit) {
                break;
            }

            auto& bsdf = hitRec.m_primitive->m_bsdf;
            // Hit medium interface, not surface
            if (!bsdf) {
                auto medium = hitRec.GetMedium(ray.d);
                ray = Ray(hitRec.m_geoRec.m_p, ray.d, medium);
                hit = m_scene->Intersect(ray, hitRec);

                bounce--;
                continue;
            }

            // Sample light            
            if (!bsdf->IsDelta(hitRec.m_geoRec.m_st)) {
                LightRecord lightRec(hitRec.m_geoRec.m_p);
                Spectrum emission = m_scene->SampleLight(lightRec, sampler.Next2D());
                if (!emission.IsBlack()) {
                    // Allocate a record for querying the BSDF
                    MaterialRecord matRec(-ray.d, lightRec.m_wi, hitRec.m_geoRec.m_ns, hitRec.m_geoRec.m_st);
                    // Evaluate BSDF * cos(theta) and pdf
                    Spectrum bsdfVal = bsdf->EvalPdf(matRec);
                    if (!bsdfVal.IsBlack()) {
                        // Weight using the power heuristic
                        float weight = PowerHeuristic(lightRec.m_pdf, matRec.m_pdf);
                        radiance += throughput * bsdfVal * emission * weight;
                    }
                }
            }

            // Sample BSDF
            {
                // Sample BSDF * |cos| / pdf
                MaterialRecord matRec(-ray.d, hitRec.m_geoRec.m_ns, hitRec.m_geoRec.m_st);
                Spectrum bsdfVal = bsdf->Sample(matRec, sampler.Next2D());
                /// Get medium
                Float3 newDirection = matRec.ToWorld(matRec.m_wo);
                auto medium = hitRec.GetMedium(newDirection);
                Float3 rayO = hitRec.m_geoRec.m_p;
                ray = Ray(rayO, newDirection, medium);
                if (bsdfVal.IsBlack()) {
                    break;
                }
                // Update throughput
                throughput *= bsdfVal;
                eta *= matRec.m_eta;
                // Eval light                
                Spectrum transmittance;
                hit = m_scene->IntersectTr(ray, hitRec, transmittance);
                LightRecord lightRec;
                ray = Ray(rayO, newDirection, medium);
                Spectrum emission = m_scene->EvalPdfLight(hit, ray, hitRec, lightRec);
                emission *= transmittance;
                if (!emission.IsBlack()) {
                    // Weight using the power heuristic
                    float weight = bsdf->IsDelta(hitRec.m_geoRec.m_st) ?
                        1.f : PowerHeuristic(matRec.m_pdf, lightRec.m_pdf);
                    radiance += throughput * emission * weight;
                }
                ray = Ray(rayO, newDirection, medium);
                hit = m_scene->Intersect(ray, hitRec);
            }
        }
        // Russian roulette
        if (bounce > 5) {
            float q = std::min(0.99f, MaxComponent(throughput * eta * eta));
            if (sampler.Next1D() > q) {
                break;
            }
            throughput /= q;
        }
    }
    return radiance;
}

void VolumePathIntegrator::Start()
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

    m_renderingNum = 0;
    m_rendering = true;

    m_threads.resize(std::thread::hardware_concurrency());
    //m_threads.resize(1);
    m_timer.Start();
    for (std::thread*& thread : m_threads) {
        thread = new std::thread(&VolumePathIntegrator::ThreadWork, this);
    }
}

void VolumePathIntegrator::Stop()
{
    m_rendering = false;
}

void VolumePathIntegrator::Wait()
{
    for (std::thread*& thread : m_threads) {
        thread->join();
        delete thread;
    }
    m_threads.clear();
    assert(m_renderingNum == 0);
}

bool VolumePathIntegrator::IsRendering()
{
    return m_rendering;
}

std::string VolumePathIntegrator::ToString() const
{
    return fmt::format("Volume Path Tracer\nspp : {0}\nmax bounce : {1}", m_spp, m_maxBounce);
}

void VolumePathIntegrator::Debug(DebugRecord& debugRec)
{
    //return;
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
            DebugRay(ray, sampler);
        }
    }
}

void VolumePathIntegrator::Setup()
{
    Integrator::Setup();
}

void VolumePathIntegrator::ThreadWork()
{
    int threadIdx = ++m_renderingNum;

    while (m_rendering) {
        Framebuffer::Tile tile;
        m_tileMutex.lock();
        if (!m_tiles.empty()) {
            tile = m_tiles.back();
            m_tiles.pop_back();

            m_tileMutex.unlock();
        }
        else {
            m_tileMutex.unlock();
            break;
        }

        if (m_rendering) {
            RenderTile(tile);
        }
    }

    m_renderingNum--;
    if (m_renderingNum == 0) {
        m_rendering = false;
        m_timer.Stop();
        m_buffer->Save();
    }
}

void VolumePathIntegrator::RenderTile(const Framebuffer::Tile& tile)
{
    Sampler sampler;
    unsigned int s = tile.pos[1] * m_buffer->m_width + tile.pos[0];
    sampler.Setup(s);

    for (int j = 0; j < tile.res[1]; j++) {
        for (int i = 0; i < tile.res[0]; i++) {
            for (uint32_t k = 0; k < m_spp; k++) {
                int x = i + tile.pos[0], y = j + tile.pos[1];
                Ray ray;
                m_camera->GenerateRay(Float2(x, y), sampler, ray);
                Spectrum radiance = Li(ray, sampler);
                m_buffer->AddSample(x, y, radiance);
            }
        }
    }
}

Spectrum VolumePathIntegrator::NormalCheck(Ray ray, Sampler& sampler)
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

void VolumePathIntegrator::DebugRay(Ray ray, Sampler& sampler) {
    Spectrum radiance(0.f);
    Spectrum throughput(1.f);
    float eta = 1.f;
    HitRecord hitRec;
    bool hit = m_scene->Intersect(ray, hitRec);
    for (uint32_t bounce = 0; bounce < m_maxBounce; bounce++) {
        // Sample medium
        MediumRecord mediumRec;
        if (hit && ray.m_medium) {
            throughput *= ray.m_medium->Sample(ray, mediumRec, sampler);
        }
        if (throughput.IsBlack()) {
            break;
        }
        if (hit) {
            Float3 p = ray.o;
            Float3 q = hitRec.m_geoRec.m_p;
            DrawLine(p, q, Spectrum(1, 1, 0));
        }
        // Medium internal
        if (mediumRec.m_internal) {
            if (!hit) {
                break;
            }

            auto& phase = ray.m_medium->m_phaseFunction;

            // Sample light
            {
                LightRecord lightRec(mediumRec.m_p);
                Spectrum emission = m_scene->SampleLight(lightRec, sampler.Next2D(), ray.m_medium);
                if (!emission.IsBlack()) {
                    // Allocate a record for querying the phase function
                    PhaseFunctionRecord phaseRec(-ray.d, lightRec.m_wi);
                    // Evaluate phase and pdf
                    Spectrum phaseVal = phase->EvalPdf(phaseRec);
                    if (!phaseVal.IsBlack()) {
                        // Weight using the power heuristic
                        float weight = PowerHeuristic(lightRec.m_pdf, phaseRec.m_pdf);
                        radiance += throughput * phaseVal * emission * weight;
                    }
                }
            }

            // Sample phase function
            {
                // Sample phase / pdf
                PhaseFunctionRecord phaseRec(-ray.d);
                Spectrum phaseVal = phase->Sample(phaseRec, sampler.Next2D());
                auto medium = ray.m_medium;
                ray = Ray(mediumRec.m_p, phaseRec.m_wo, ray.m_medium);
                // Update throughput
                throughput *= phaseVal;
                // Eval light                
                Spectrum transmittance;
                hit = m_scene->IntersectTr(ray, hitRec, transmittance);
                ray = Ray(mediumRec.m_p, phaseRec.m_wo, medium);
                LightRecord lightRec;
                Spectrum emission = m_scene->EvalPdfLight(hit, ray, hitRec, lightRec);
                emission *= transmittance;
                if (!emission.IsBlack()) {
                    // Weight using the power heuristic
                    float weight = PowerHeuristic(phaseRec.m_pdf, lightRec.m_pdf);
                    radiance += throughput * emission * weight;
                }

                ray = Ray(mediumRec.m_p, phaseRec.m_wo, medium);
                hit = m_scene->Intersect(ray, hitRec);
            }
        }
        // Medium external
        else {
            // Eval direct light at first bounce
            if (bounce == 0) {
                radiance += throughput * m_scene->EvalLight(hit, ray, hitRec);
            }
            // No hit
            if (!hit) {
                break;
            }

            auto& bsdf = hitRec.m_primitive->m_bsdf;
            // Hit medium interface, not surface
            if (!bsdf) {
                auto medium = hitRec.GetMedium(ray.d);
                ray = Ray(hitRec.m_geoRec.m_p, ray.d, medium);
                hit = m_scene->Intersect(ray, hitRec);

                bounce--;
                continue;
            }

            // Sample light            
            if (!bsdf->IsDelta(hitRec.m_geoRec.m_st)) {
                LightRecord lightRec(hitRec.m_geoRec.m_p);
                Spectrum emission = m_scene->SampleLight(lightRec, sampler.Next2D());
                if (!emission.IsBlack()) {
                    // Allocate a record for querying the BSDF
                    MaterialRecord matRec(-ray.d, lightRec.m_wi, hitRec.m_geoRec.m_ns, hitRec.m_geoRec.m_st);
                    // Evaluate BSDF * cos(theta) and pdf
                    Spectrum bsdfVal = bsdf->EvalPdf(matRec);
                    if (!bsdfVal.IsBlack()) {
                        // Weight using the power heuristic
                        float weight = PowerHeuristic(lightRec.m_pdf, matRec.m_pdf);
                        radiance += throughput * bsdfVal * emission * weight;
                    }
                }
            }

            // Sample BSDF
            {
                // Sample BSDF * |cos| / pdf
                MaterialRecord matRec(-ray.d, hitRec.m_geoRec.m_ns, hitRec.m_geoRec.m_st);
                Spectrum bsdfVal = bsdf->Sample(matRec, sampler.Next2D());
                /// Get medium
                Float3 newDirection = matRec.ToWorld(matRec.m_wo);
                auto medium = hitRec.GetMedium(newDirection);
                Float3 rayO = hitRec.m_geoRec.m_p;
                ray = Ray(rayO, newDirection, medium);
                if (bsdfVal.IsBlack()) {
                    break;
                }
                // Update throughput
                throughput *= bsdfVal;
                eta *= matRec.m_eta;
                // Eval light                
                Spectrum transmittance;
                hit = m_scene->IntersectTr(ray, hitRec, transmittance);
                LightRecord lightRec;
                ray = Ray(rayO, newDirection, medium);
                Spectrum emission = m_scene->EvalPdfLight(hit, ray, hitRec, lightRec);
                emission *= transmittance;
                if (!emission.IsBlack()) {
                    // Weight using the power heuristic
                    float weight = bsdf->IsDelta(hitRec.m_geoRec.m_st) ?
                        1.f : PowerHeuristic(matRec.m_pdf, lightRec.m_pdf);
                    radiance += throughput * emission * weight;
                }
                ray = Ray(rayO, newDirection, medium);
                hit = m_scene->Intersect(ray, hitRec);
            }
        }
        // Russian roulette
        if (bounce > 5) {
            float q = std::min(0.99f, MaxComponent(throughput * eta * eta));
            if (sampler.Next1D() > q) {
                break;
            }
            throughput /= q;
        }
    }
}

float VolumePathIntegrator::PowerHeuristic(float a, float b) const
{
    a *= a;
    b *= b;
    return a / (a + b);
}

