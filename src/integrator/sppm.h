#pragma once

#include "core/integrator.h"
#include "photon.h"

#include <atomic>
#include <thread>
#include <mutex>

class SPPMIntegrator : public Integrator {
public:
    SPPMIntegrator(
        const std::shared_ptr<Scene>& scene,
        const std::shared_ptr<Camera>& camera,
        const std::shared_ptr<Framebuffer>& buffer,
        const uint32_t maxBounce,
        const uint32_t maxIteration,
        const uint32_t deltaPhotonNum,
        const float initialRadius,
        const float alpha)
        : Integrator(scene, camera, buffer),
        m_maxBounce(maxBounce), m_maxIteration(maxIteration),
        m_deltaPhotonNum(deltaPhotonNum), m_initialRadius(initialRadius), m_alpha(alpha) {}

    Spectrum Li(Ray ray, Sampler& sampler);
    void Start();
    void Stop();
    void Wait();
    bool IsRendering();
    std::string ToString() const;
    // Debug
    void Debug(DebugRecord& debugRec);
private:

    void Setup();
    void Render();
    void PhotonIteration();
    void EmitPhoton(Sampler& sampler);
    void RenderIteration(const uint32_t& spp, const uint32_t& iteration);
    void RenderTile(const Framebuffer::Tile& tile, const uint32_t& spp, const uint32_t& iteration);
    void SynchronizeThreads();
    // Debug
    void DebugRay(Ray ray, Sampler& sampler);
private:
    // Muti-thread setting
    bool m_rendering;
    std::atomic<int> m_renderingNum;
    std::vector<std::thread*> m_renderThreads;
    std::unique_ptr<std::thread> m_controlThread;
    std::vector<Framebuffer::Tile> m_tiles;
    std::atomic<uint32_t> m_currentTileIndex;
    std::vector<uint32_t> m_photons;
    std::atomic<uint32_t> m_currentPhotonIndex;

    // Options
    uint32_t m_maxBounce;
    uint32_t m_maxIteration;
    uint32_t m_deltaPhotonNum;
    float m_initialRadius;
    float m_alpha;

    // State
    uint32_t m_currentPhotonNum;
    uint32_t m_currentIteration;
    float m_currentRadius;

    // 
    KDTree m_photonTree;
};