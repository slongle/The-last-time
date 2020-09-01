#include "director.h"

Spectrum DirectorIntegrator::Li(Ray ray, Sampler& sampler)
{
    Spectrum radiance(0.f);
    for (int i = 0; i < 1; i++) {
        HitRecord hitRec;
        bool hit = m_scene->Intersect(ray, hitRec);
        {
            auto light = m_scene->m_lights[0];
            LightRecord lightRec(ray.o);
            Spectrum Le = light->Sample(lightRec, Float2(0, 0));
            if (Dot(ray.d, lightRec.m_wi) > 1 - 1e-5) {
                radiance = Spectrum(0, 1, 0);                
            }
            //break;
        }
        if (!hit) {
            break;
        }
        
        //Float2 st = hitRec.m_geoRec.m_st;
        //radiance = Spectrum(st.x, st.y, 0);

        auto light = m_scene->m_lights[0];
        LightRecord lightRec(hitRec.m_geoRec.m_p);
        Spectrum Le = light->Sample(lightRec, Float2(0, 0));

        auto bsdf = hitRec.GetBSDF();
        MaterialRecord matRec(-ray.d, lightRec.m_wi, hitRec.m_geoRec.m_ns, hitRec.m_geoRec.m_st);
        Spectrum f = bsdf->Eval(matRec);

        radiance = f * Le;
        //radiance = f;
    }
    return radiance;
}

std::string DirectorIntegrator::ToString() const
{
    return fmt::format("Director(SVBRDF)\nspp : {0}\nmax bounce : {1}", m_spp, m_maxBounce);
}

void DirectorIntegrator::Debug(DebugRecord& debugRec)
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

void DirectorIntegrator::DebugRay(Ray ray, Sampler& sampler)
{
    Spectrum radiance(0.f);
    for (int i = 0; i < 1; i++) {
        HitRecord hitRec;
        bool hit = m_scene->Intersect(ray, hitRec);
        {
            auto light = m_scene->m_lights[0];
            LightRecord lightRec(ray.o);
            Spectrum Le = light->Sample(lightRec, Float2(0, 0));
            if (Dot(ray.d, lightRec.m_wi) > 1 - 1e-5) {
                radiance = Spectrum(0, 1, 0);
            }
            //break;
        }
        if (!hit) {
            break;
        }
        auto light = m_scene->m_lights[0];
        LightRecord lightRec(hitRec.m_geoRec.m_p);
        Spectrum Le = light->Sample(lightRec, Float2(0, 0));        

        auto bsdf = hitRec.GetBSDF();
        MaterialRecord matRec(-ray.d, lightRec.m_wi, hitRec.m_geoRec.m_ns, hitRec.m_geoRec.m_st);
        Spectrum f = bsdf->Eval(matRec);

        std::cout << f << std::endl;
        std::cout << Le << std::endl;
    }
}
