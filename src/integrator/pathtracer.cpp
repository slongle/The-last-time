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
    std::reverse(m_tiles.begin(), m_tiles.end());

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
    m_threads.clear();
    assert(m_renderingNum == 0);
}

bool PathIntegrator::IsRendering()
{
    return m_rendering;
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

void PathIntegrator::Setup()
{
    Integrator::Setup();
}

void PathIntegrator::ThreadWork()
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
                //Spectrum radiance = NormalCheck(ray, sampler);
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

