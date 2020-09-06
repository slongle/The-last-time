#include "volumepathtracer.h"
#include "light/environment.h"

Spectrum VolumePathIntegrator::Li(Ray ray, Sampler& sampler)
{
    Spectrum radiance(0.f);
    Spectrum throughput(1.f);
    float eta = 1.f;
    HitRecord hitRec;
    bool hit = m_scene->Intersect(ray, hitRec);
    for (int bounce = 0; bounce < m_maxBounce; bounce++) {
        if (hit) {
            //return Spectrum(0.5f);
        }
        else {
            //return Spectrum(0.f);
        }
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

            if (!mediumRec.m_Le.IsBlack()) {
                radiance += throughput * mediumRec.m_Le;
                break;
            }

            auto medium = ray.m_medium;
            auto& phase = medium->m_phaseFunction;
            // Sample light
            {
                LightRecord lightRec(mediumRec.m_p);
                Spectrum emission = SampleLight(lightRec, sampler, medium);
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
                ray = Ray(mediumRec.m_p, phaseRec.m_wo, medium);
                // Update throughput
                throughput *= phaseVal;
                // Eval light                
                Spectrum transmittance;
                hit = m_scene->IntersectTr(ray, hitRec, transmittance, sampler);
                ray = Ray(mediumRec.m_p, phaseRec.m_wo, medium);
                LightRecord lightRec;
                Spectrum emission = EvalPdfLight(hit, ray, hitRec, lightRec);
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
                radiance += throughput * EvalLight(hit, ray, hitRec);
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
                Spectrum emission = m_scene->SampleLight(lightRec, sampler.Next2D(), sampler);
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
                hit = m_scene->IntersectTr(ray, hitRec, transmittance, sampler);
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

std::string VolumePathIntegrator::ToString() const
{
    return fmt::format("Volume Path Tracer\nspp : {0}\nmax bounce : {1}", m_spp, m_maxBounce);
}

Spectrum VolumePathIntegrator::EvalLight(bool hit, const Ray& ray, const HitRecord& hitRec) const
{
    Spectrum emission(0.f);
    if (hit) {
        if (hitRec.m_primitive->IsAreaLight()) {
            LightRecord lightRec(ray.o, hitRec.m_geoRec);
            emission = hitRec.m_primitive->m_areaLight->Eval(lightRec);
        }
    }
    else {
        // Environment Light
        for (const auto& envLight : m_scene->m_environmentLights) {
            emission += envLight->Eval(ray);
        }
    }
    return emission;
}

Spectrum VolumePathIntegrator::EvalPdfLight(bool hit, const Ray& ray, const HitRecord& hitRec, LightRecord& lightRec) const
{
    uint32_t lightNum = m_scene->m_lights.size();
    float chooseLightPdf = 1.f / lightNum;
    Spectrum emission(0.f);
    if (hit) {
        if (hitRec.m_primitive->IsAreaLight()) {
            auto areaLight = hitRec.m_primitive->m_areaLight;
            lightRec = LightRecord(ray.o, hitRec.m_geoRec);
            emission = areaLight->EvalPdf(lightRec);
            lightRec.m_pdf *= chooseLightPdf;
        }
    }
    else {
        // Environment Light
        for (const auto& envLight : m_scene->m_environmentLights) {
            lightRec = LightRecord(ray);
            emission = envLight->EvalPdf(lightRec);
            lightRec.m_pdf *= chooseLightPdf;
        }
    }
    return emission;
}

Spectrum VolumePathIntegrator::SampleLight(
    LightRecord& lightRec, 
    Sampler& sampler,
    const std::shared_ptr<Medium>& medium) const
{
    uint32_t lightNum = m_scene->m_lights.size();
    if (lightNum == 0) {
        return Spectrum(0.f);
    }
    // Randomly pick an emitter
    uint32_t lightIdx = std::min(uint32_t(lightNum * sampler.Next1D()), lightNum - 1);
    float lightChoosePdf = 1.f / lightNum;
    const auto& light = m_scene->m_lights[lightIdx];
    // Sample on light
    Spectrum emission = light->Sample(lightRec, sampler.Next2D());
    // Occlude test
    if (lightRec.m_pdf != 0) {
        // Handle medium
        Spectrum throughput(1.f);
        bool occlude;
        {
            HitRecord hitRec;
            Ray ray = lightRec.m_shadowRay;
            float tMax = ray.tMax;
            bool hit = m_scene->Intersect(ray, hitRec);
            if (!hit) {
                // Light inside the medium
                throughput *= medium->Transmittance(ray, sampler);
                occlude = false;
            }
            else {
                // Light outside the medium
                throughput *= medium->Transmittance(ray, sampler);
                ray = Ray(ray(ray.tMax), ray.d, Ray::epsilon, tMax - ray.tMax);
                occlude = m_scene->Occlude(ray);
            }
        }
        // Update pdf and Le
        if (occlude) {
            return Spectrum(0.0f);
        }        
        lightRec.m_pdf *= lightChoosePdf;
        emission *= throughput / lightChoosePdf;
        return emission;
    }
    else {
        return Spectrum(0.0f);
    }
}

void VolumePathIntegrator::Debug(DebugRecord& debugRec)
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
            DebugRay(ray, sampler);
        }
    }
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
                Spectrum emission = m_scene->SampleLight(lightRec, sampler.Next2D(), sampler, ray.m_medium);
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
                hit = m_scene->IntersectTr(ray, hitRec, transmittance, sampler);
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
                Spectrum emission = m_scene->SampleLight(lightRec, sampler.Next2D(), sampler);
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
                hit = m_scene->IntersectTr(ray, hitRec, transmittance, sampler);
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