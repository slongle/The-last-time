#include "pathguider.h"
#include "light/environment.h"



Spectrum PathGuiderIntegrator::Li(Ray ray, Sampler& sampler)
{
    Spectrum radiance(0.f);
    Spectrum throughput(1.f);
    float eta = 1.f;
    HitRecord hitRec;
    bool hit = m_scene->Intersect(ray, hitRec);
    for (uint32_t bounce = 0; bounce < m_maxBounce; bounce++) {
        if (!hit) {
            // Environment Light
            if (bounce == 0 && m_scene->m_environmentLight) {
                radiance += throughput * m_scene->m_environmentLight->Eval(ray);
            }
            break;
        }

        if (bounce == 0 && hitRec.m_primitive->IsAreaLight()) {
            LightRecord lightRec(ray.o, hitRec.m_geoRec);
            radiance += throughput * hitRec.m_primitive->m_areaLight->Eval(lightRec);
        }

        auto& bsdf = hitRec.m_primitive->m_bsdf;

        if (!bsdf->IsDelta(hitRec.m_geoRec.m_st)) {
            LightRecord lightRec(hitRec.m_geoRec.m_p);
            Spectrum emission = m_scene->SampleLight(lightRec, sampler.Next2D());
            if (!emission.IsBlack()) {
                /* Allocate a record for querying the BSDF */
                MaterialRecord matRec(-ray.d, lightRec.m_wi, hitRec.m_geoRec.m_ns, hitRec.m_geoRec.m_st);

                /* Evaluate BSDF * cos(theta) and pdf */
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
            }
            else {
                // Environment Light
                if (m_scene->m_environmentLight) {
                    lightRec = LightRecord(ray);
                    emission = m_scene->m_environmentLight->EvalPdf(lightRec);
                    lightRec.m_pdf /= m_scene->m_lights.size();
                }
                else {
                    break;
                }
            }

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

void PathGuiderIntegrator::Setup()
{
    Integrator::Setup();
}

void PathGuiderIntegrator::Start()
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
    std::reverse(m_tiles.begin(), m_tiles.end());

    m_renderingNum = 0;
    m_rendering = true;

    m_currentSpp = 0;

    m_threads.resize(std::thread::hardware_concurrency());
    //m_threads.resize(1);
    m_timer.Start();
    m_mainThread = std::make_unique<std::thread>(&PathGuiderIntegrator::Render, this);    
}

void PathGuiderIntegrator::Stop()
{
    m_rendering = false;
}

void PathGuiderIntegrator::Wait()
{
    if (m_mainThread->joinable()) {
        m_mainThread->join();
    }
    assert(m_renderingNum == 0);
}

bool PathGuiderIntegrator::IsRendering()
{
    return m_rendering;
}

std::string PathGuiderIntegrator::ToString() const
{
    return fmt::format("Path Guider\nspp : {0}\nmax bounce : {1}", m_currentSpp, m_maxBounce);
}

void PathGuiderIntegrator::Render()
{
    uint32_t spp = m_initSpp;
    for (uint32_t i = 0; i < m_maxIteration && m_rendering; i++) {
        m_currentTileIndex = 0;
        for (std::thread*& thread : m_threads) {
            thread = new std::thread(&PathGuiderIntegrator::RenderIteration, this, spp, i);
        }
        SynchronizeThreads();        
        m_currentSpp += spp;
        spp *= 2;
    }
    m_rendering = false;
    m_timer.Stop();
    m_buffer->Save();
}

void PathGuiderIntegrator::RenderIteration(const uint32_t& spp, const uint32_t& iteration)
{
    ++m_renderingNum;

    while (m_rendering) {
        Framebuffer::Tile tile;
        if (m_currentTileIndex < m_tiles.size()) {
            tile = m_tiles[m_currentTileIndex++];
        }
        else {
            break;
        }

        if (m_rendering) {
            RenderTile(tile, spp, iteration);
        }
    }

    m_renderingNum--;
    if (m_renderingNum == 0) {
        
    }
}

void PathGuiderIntegrator::RenderTile(
    const Framebuffer::Tile& tile, 
    const uint32_t& spp, 
    const uint32_t& iteration)
{
    Sampler sampler;
    uint64_t s = (tile.pos[1] * m_buffer->m_width + tile.pos[0]) + 
        iteration * (m_buffer->m_width * m_buffer->m_height);
    sampler.Setup(s);

    for (int j = 0; j < tile.res[1]; j++) {
        for (int i = 0; i < tile.res[0]; i++) {
            for (uint32_t k = 0; k < spp; k++) {
                int x = i + tile.pos[0], y = j + tile.pos[1];
                Ray ray;
                m_camera->GenerateRay(Float2(x, y), sampler, ray);
                Spectrum radiance = Li(ray, sampler);
                m_buffer->AddSample(x, y, radiance);
            }
        }
    }
}

void PathGuiderIntegrator::SynchronizeThreads()
{
    for (std::thread*& thread : m_threads) {
        thread->join();
        delete thread;
    }
    assert(m_renderingNum == 0);
}

void PathGuiderIntegrator::Debug(DebugRecord& debugRec)
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
    if (debugRec.m_debugKDTree) {
        Bounds bounds = m_scene->m_bounds;
        DrawBounds(bounds, Spectrum(1, 0, 0));
    }
}

void PathGuiderIntegrator::DebugRay(Ray ray, Sampler& sampler)
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
            Spectrum emission = m_scene->SampleLight(lightRec, sampler.Next2D());
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
                if (m_scene->m_environmentLight) {
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

float PathGuiderIntegrator::PowerHeuristic(float a, float b) const
{
    a *= a;
    b *= b;
    return a / (a + b);
}

