#include "pathtracer.h"

Spectrum PathIntegrator::Li(Ray ray, Sampler& sampler)
{
    Spectrum radiance(0.f);
    Spectrum throughput(1.f);
    bool specular = false;
    for (uint32_t bounce = 0; bounce < m_maxBounce; bounce++) {
        HitRecord hitRec;
        bool hit = m_scene->Intersect(ray, hitRec);

        if (!hit) {
            if (bounce == 0 || specular) {
                // Environment Light
            }
            break;
        }

        if ((bounce == 0 || specular) && hitRec.m_primitive->IsAreaLight()) {
            LightRecord lightRec(ray.o, hitRec.m_geoRec);
            radiance += throughput * hitRec.m_primitive->m_areaLight->Eval(lightRec);
        }

        auto& bsdf = hitRec.m_primitive->m_bsdf;
        MaterialRecord matRec(-ray.d, hitRec.m_geoRec.m_ns, hitRec.m_geoRec.m_st);
        specular = bsdf->IsDelta(matRec);

        // NEE using MIS            
        if (!specular) {
            // Light Sampling
            {
                // Choose one of lights
                uint32_t lightNum = m_scene->m_lights.size();
                uint32_t lightIdx = std::min(uint32_t(lightNum * sampler.Next1D()), lightNum - 1);
                const auto& light = m_scene->m_lights[lightIdx];
                // Sample light
                LightRecord lightRec(hitRec.m_geoRec.m_p);
                Spectrum emission = light->Sample(lightRec, sampler.Next2D()) * lightNum;
                if (!m_scene->Occlude(lightRec.m_shadowRay)) {
                    matRec.m_wo = matRec.ToLocal(lightRec.m_wi);
                    Spectrum f = bsdf->EvalPdf(matRec);
                    // Balance Heuristic
                    float weight = lightRec.m_pdf / (lightRec.m_pdf + matRec.m_pdf);
                    radiance += emission * throughput * f * weight;
                }
            }
            // BSDF Sampling
            {
                Spectrum f = bsdf->Sample(matRec, sampler.Next2D());
                Ray shadowRay(hitRec.m_geoRec.m_p, matRec.ToWorld(matRec.m_wo));
                HitRecord lightHitRecord;
                if (m_scene->Intersect(shadowRay, lightHitRecord)) {
                    if (lightHitRecord.m_primitive->IsAreaLight()) {
                        auto areaLight = lightHitRecord.m_primitive->m_areaLight;
                        LightRecord lightRec(hitRec.m_geoRec.m_p, lightHitRecord.m_geoRec);
                        Spectrum emission = areaLight->EvalPdf(lightRec);
                        // Balance Heuristic
                        float weight = matRec.m_pdf / (lightRec.m_pdf + matRec.m_pdf);
                        radiance += emission * throughput * f * weight;
                    }
                }
                else {
                    // Environment Light
                }
            }
        }

        throughput *= bsdf->Sample(matRec, sampler.Next2D());
        ray = Ray(hitRec.m_geoRec.m_p, matRec.ToWorld(matRec.m_wo));

        if (bounce > 3) {
            float q = std::min(0.99f, MaxComponent(throughput));
            if (sampler.Next1D() > q) {
                break;
            }
            throughput /= q;
        }
    }
    return radiance;
}

void PathIntegrator::Start()
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

    m_renderingNum = 0;
    m_rendering = true;

    m_threads.resize(std::thread::hardware_concurrency());
    //m_threads.resize(1);
    m_timer.Start();
    for (std::thread*& thread : m_threads) {
        thread = new std::thread(&PathIntegrator::ThreadWork, this);
    }
}

void PathIntegrator::Stop()
{
    m_rendering = false;
}

void PathIntegrator::Wait()
{
    for (std::thread*& thread : m_threads) {
        thread->join();
        delete thread;
    }
    assert(m_renderingNum == 0);
}

bool PathIntegrator::IsRendering()
{
    return m_rendering;
}

void PathIntegrator::Setup()
{
    Integrator::Setup();
}

void PathIntegrator::ThreadWork()
{
    int threadIdx = ++m_renderingNum;
    //std::cout << threadIdx << std::endl;

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

void PathIntegrator::RenderTile(const Framebuffer::Tile& tile)
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

Spectrum PathIntegrator::NormalCheck(Ray ray, Sampler& sampler)
{
    Spectrum radiance(0.f);
    HitRecord hitRec;
    bool hit = m_scene->Intersect(ray, hitRec);

    if (!hit) {
        return radiance;
    }

    hitRec.m_primitive->m_shape->SetGeometryRecord(hitRec.m_geoRec);

    Float2 st = hitRec.m_geoRec.m_st;
    radiance = Spectrum(st.x, st.y, 0); return radiance;
    //radiance = Spectrum(hitRec.m_geoRec.m_ng); return radiance;

    float dotValue = Dot(-ray.d, hitRec.m_geoRec.m_ns);
    if (dotValue >= 0) {
        radiance = Spectrum(0.f, 0.f, 1.f) * dotValue;
    }
    else {
        radiance = Spectrum(1.f, 0.f, 0.f) * std::fabs(dotValue);
    }

    return radiance;
}

