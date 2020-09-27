#include "pppm.h"

void PPPMIntegrator::EmitPhoton(Sampler& sampler)
{    
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

Spectrum PPPMIntegrator::Li(Ray ray, Sampler& sampler)
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

void PPPMIntegrator::Setup()
{
    Integrator::Setup();
}

void PPPMIntegrator::Start()
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

    m_photons.clear();
    for (uint32_t i = 0; i < m_deltaPhotonNum; i++) {
        m_photons.push_back(i);
    }

    m_renderingNum = 0;
    m_rendering = true;    

    m_renderThreads.resize(std::thread::hardware_concurrency());
    //m_renderThreads.resize(1);
    m_timer.Start();
    m_controlThread = std::make_unique<std::thread>(&PPPMIntegrator::Render, this);
}

void PPPMIntegrator::Stop()
{
    m_rendering = false;
}

void PPPMIntegrator::Wait()
{
    if (m_controlThread->joinable()) {
        m_controlThread->join();
    }
    assert(m_renderingNum == 0);
}

bool PPPMIntegrator::IsRendering()
{
    return m_rendering;
}

std::string PPPMIntegrator::ToString() const
{
    return fmt::format("PPPM\nnmax bounce : {0}\niteration : {1}\n# photon : {2}\nradius : {3}",
        m_maxBounce, m_currentIteration, m_currentPhotonNum, m_currentRadius);
}

void PPPMIntegrator::Render()
{
    m_currentRadius = m_initialRadius;
    m_currentPhotonNum = 0;
    for (m_currentIteration = 0; m_currentIteration < m_maxIteration && m_rendering; m_currentIteration++) {                
        m_currentPhotonIndex = 0;
        m_currentTileIndex = 0;
        // Photon pass
        for (std::thread*& thread : m_renderThreads) {
            thread = new std::thread(&PPPMIntegrator::PhotonIteration, this);
        }
        SynchronizeThreads();        

        // Construct photon structure
        m_photonTree.Build();

        // Camera pass
        for (std::thread*& thread : m_renderThreads) {
            thread = new std::thread(&PPPMIntegrator::RenderIteration, this, 1, m_currentIteration);
        }
        SynchronizeThreads();
        
        // Update settings
        if (m_currentIteration + 1 != m_maxIteration) {
            m_photonTree.Clear();
        }
        m_currentPhotonNum += m_deltaPhotonNum;
        m_currentRadius = std::sqrt((m_currentIteration + m_alpha)/(m_currentIteration + 1)) * m_currentRadius;
    }
    m_rendering = false;
    m_timer.Stop();
    m_buffer->Save();
}

void PPPMIntegrator::PhotonIteration()
{
    ++m_renderingNum;

    while (m_rendering) {
        uint32_t photonIndex;
        if (m_currentPhotonIndex < m_photons.size()) {
            photonIndex = m_photons[m_currentPhotonIndex++];
        }
        else {
            break;
        }

        if (m_rendering) {
            Sampler sampler;
            uint64_t seed = (uint64_t)m_currentIteration * m_deltaPhotonNum + photonIndex;
            sampler.Setup(seed);
            EmitPhoton(sampler);
        }
    }

    m_renderingNum--;
}

void PPPMIntegrator::RenderIteration(const uint32_t& spp, const uint32_t& iteration)
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
}

void PPPMIntegrator::RenderTile(
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

void PPPMIntegrator::SynchronizeThreads()
{
    for (std::thread*& thread : m_renderThreads) {
        thread->join();
        delete thread;
    }
    assert(m_renderingNum == 0);
}

void PPPMIntegrator::Debug(DebugRecord& debugRec)
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
        for (const auto& photon : m_photonTree.m_photons) {
            DrawPoint(photon.m_position, Spectrum(1, 0, 0) * photon.m_flux.y());
        }
    }
}

void PPPMIntegrator::DebugRay(Ray ray, Sampler& sampler)
{
    Spectrum radiance(0.f);
    Spectrum throughput(1.f);
    float eta = 1.f;
    HitRecord hitRec;
    for (uint32_t bounce = 0; bounce < m_maxBounce; bounce++) {
        bool hit = m_scene->Intersect(ray, hitRec);
        radiance += throughput * m_scene->EvalLight(hit, ray, hitRec);

        // Draw ray
        if (bounce != 0) {
            DrawLine(ray.o, hitRec.m_geoRec.m_p, Spectrum(1, 1, 1));
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
}
