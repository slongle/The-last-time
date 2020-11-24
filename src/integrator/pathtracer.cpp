#include "pathtracer.h"
#include "light/environment.h"

Spectrum PathIntegrator::Li(Ray ray, Sampler& sampler)
{
    Spectrum radiance(0.f);
    Spectrum throughput(1.f);
    float eta = 1.f;
    HitRecord hitRec;
    bool hit = m_scene->Intersect(ray, hitRec);
    for (uint32_t bounce = 0; bounce < m_maxBounce; bounce++) {
        // Eval direct light at first bounce
        if (bounce == 0) {
            radiance += throughput * EvalLight(hit, ray, hitRec);
        }
        // No hit
        if (!hit) {
            break;
        }

        if (bounce == 0 && hitRec.m_primitive->IsAreaLight()) {
            LightRecord lightRec(ray.o, hitRec.m_geoRec);
            radiance += throughput * hitRec.m_primitive->m_areaLight->Eval(lightRec);
        }

        auto& bsdf = hitRec.m_primitive->m_bsdf;

        if (!bsdf->IsDelta(hitRec.m_geoRec.m_st)) {
            LightRecord lightRec(hitRec.m_geoRec.m_p);
            Spectrum emission = SampleLight(lightRec, sampler.Next2D());
            if (!emission.IsBlack()) {

                /* Evaluate BSDF * cos(theta) and pdf */
                MaterialRecord matRec(-ray.d, lightRec.m_wi, hitRec.m_geoRec.m_ns, hitRec.m_geoRec.m_st);
                Spectrum bsdfVal = bsdf->EvalPdf(matRec);

                if (!bsdfVal.IsBlack()) {
                    /* Weight using the power heuristic */
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
            ray = Ray(hitRec.m_geoRec.m_p, matRec.ToWorld(matRec.m_wo));
            if (bsdfVal.IsBlack()) {
                break;
            }

            // Update throughput
            throughput *= bsdfVal;
            eta *= matRec.m_eta;
            hit = m_scene->Intersect(ray, hitRec);

            // Eval emission
            LightRecord lightRec;
            Spectrum emission = EvalPdfLight(hit, ray, hitRec, lightRec);
            if (!emission.IsBlack()) {
                // Heuristic
                float weight = bsdf->IsDelta(hitRec.m_geoRec.m_st) ?
                    1.f : PowerHeuristic(matRec.m_pdf, lightRec.m_pdf);
                radiance += throughput * emission * weight;
            }
        }

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

Spectrum PathIntegrator::EvalLight(bool hit, const Ray& ray, const HitRecord& hitRec) const
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

Spectrum PathIntegrator::EvalPdfLight(bool hit, const Ray& ray, const HitRecord& hitRec, LightRecord& lightRec) const
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

Spectrum PathIntegrator::SampleLight(LightRecord& lightRec, Float2& s) const
{
    uint32_t lightNum = m_scene->m_lights.size();
    if (lightNum == 0) {
        return Spectrum(0.f);
    }
    // Randomly pick an emitter
    uint32_t lightIdx = std::min(uint32_t(lightNum * s.x), lightNum - 1);
    float lightChoosePdf = 1.f / lightNum;
    s.x = s.x * lightNum - lightIdx;
    const auto& light = m_scene->m_lights[lightIdx];
    // Sample on light
    Spectrum emission = light->Sample(lightRec, s);
    // Occlude test
    if (lightRec.m_pdf != 0) {
        Spectrum throughput(1.f);
        bool occlude = m_scene->OccludeTransparent(lightRec.m_shadowRay, throughput);
        //bool occlude = m_scene->Occlude(lightRec.m_shadowRay);
        if (occlude) {
            return Spectrum(0.0f);
        }
        if (throughput.r != 1) {
            std::cout << throughput << std::endl;
        }
        lightRec.m_pdf *= lightChoosePdf;
        emission *= throughput / lightChoosePdf;
        return emission;
    }
    else {
        return Spectrum(0.0f);
    }
}

std::string PathIntegrator::ToString() const
{
    return fmt::format("Path Tracer\nspp : {0}\nmax bounce : {1}", m_spp, m_maxBounce);
}

void PathIntegrator::Debug(DebugRecord& debugRec)
{
    //return;
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
            DebugRay(ray, sampler);
        }
    }
}

void PathIntegrator::DebugRay(Ray ray, Sampler& sampler)
{
    Spectrum throughput(1.f);
    HitRecord hitRec;
    bool hit = m_scene->Intersect(ray, hitRec);
    for (uint32_t bounce = 0; bounce < m_maxBounce; bounce++) {
        if (!hit) {
            break;
        }

        auto& bsdf = hitRec.m_primitive->m_bsdf;

        if (!bsdf->IsDelta(hitRec.m_geoRec.m_st)) {
            LightRecord lightRec(hitRec.m_geoRec.m_p);
            Spectrum emission = m_scene->SampleLight(lightRec, sampler.Next2D(), sampler);
            if (!emission.IsBlack()) {
                /* Allocate a record for querying the BSDF */
                MaterialRecord matRec(-ray.d, lightRec.m_wi, hitRec.m_geoRec.m_ns, hitRec.m_geoRec.m_st);

                /* Evaluate BSDF * cos(theta) and pdf */
                Spectrum bsdfVal = bsdf->EvalPdf(matRec);

                if (!bsdfVal.IsBlack()) {
                    /* Weight using the power heuristic */
                    float weight = PowerHeuristic(lightRec.m_pdf, matRec.m_pdf);
                    Spectrum value = throughput * bsdfVal * emission * weight;

                    Float3 p = hitRec.m_geoRec.m_p;
                    Float3 q = lightRec.m_geoRec.m_p;
                    DrawLine(p, q, Spectrum(1, 1, 0));
                }
            }
        }

        // Sample BSDF
        {
            // Sample BSDF * |cos| / pdf
            MaterialRecord matRec(-ray.d, hitRec.m_geoRec.m_ns, hitRec.m_geoRec.m_st);
            Spectrum bsdfVal = bsdf->Sample(matRec, sampler.Next2D());
            ray = Ray(hitRec.m_geoRec.m_p, matRec.ToWorld(matRec.m_wo));
            if (bsdfVal.IsBlack()) {
                break;
            }

            // Update throughput
            throughput *= bsdfVal;

            LightRecord lightRec;
            Spectrum emission;
            hit = m_scene->Intersect(ray, hitRec);
            if (hit) {
                if (hitRec.m_primitive->IsAreaLight()) {
                    auto areaLight = hitRec.m_primitive->m_areaLight;
                    lightRec = LightRecord(ray.o, hitRec.m_geoRec);
                    emission = areaLight->EvalPdf(lightRec);
                    lightRec.m_pdf /= m_scene->m_lights.size();
                }
                Float3 p = ray.o;
                Float3 q = hitRec.m_geoRec.m_p;
                DrawLine(p, q, Spectrum(1, 1, 1));
            }
            else {
                // Environment Light
                if (m_scene->m_environmentLights.empty()) {
                    Float3 p = ray.o;
                    Float3 q = ray.o + ray.d * 100.f;
                    DrawLine(p, q, Spectrum(1, 0, 0));
                }
                else {
                    break;
                }
            }

            if (!emission.IsBlack()) {
                // Heuristic
                float weight = bsdf->IsDelta(hitRec.m_geoRec.m_st) ?
                    1.f : PowerHeuristic(matRec.m_pdf, lightRec.m_pdf);
                Spectrum contribution = throughput * emission * weight;
            }
        }

        if (bounce > 5) {
            float q = std::min(0.99f, MaxComponent(throughput));
            if (sampler.Next1D() > q) {
                break;
            }
            throughput /= q;
        }
    }
}

