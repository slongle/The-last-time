#pragma once

#include "core/integrator.h"

#include <atomic>
#include <thread>
#include <mutex>

class SDTree {
public:
    SDTree(Bounds bounds) :m_bounds(bounds) {}
    Bounds m_bounds;
};

class PathGuiderIntegrator : public Integrator {
public:
    PathGuiderIntegrator(
        const std::shared_ptr<Scene>& scene,
        const std::shared_ptr<Camera>& camera,
        const std::shared_ptr<Framebuffer>& buffer,
        const uint32_t maxBounce,
        const uint32_t initSpp,
        const uint32_t maxIteration)
        : Integrator(scene, camera, buffer), m_maxBounce(maxBounce), m_initSpp(initSpp), m_maxIteration(maxIteration) {}

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
    void RenderIteration(const uint32_t& spp, const uint32_t& iteration);
    void RenderTile(const Framebuffer::Tile& tile, const uint32_t& spp, const uint32_t& iteration);
    void SynchronizeThreads();
    // Misc
    float PowerHeuristic(float a, float b) const;
    // Debug
    void DebugRay(Ray ray, Sampler& sampler);
private:
    // Muti-thread setting
    bool m_rendering;
    std::atomic<int> m_renderingNum;
    std::vector<std::thread*> m_threads;
    std::vector<Framebuffer::Tile> m_tiles;
    std::atomic<uint32_t> m_currentTileIndex;
    std::unique_ptr<std::thread> m_mainThread;

    // Options
    uint32_t m_maxBounce;
    uint32_t m_maxIteration;
    uint32_t m_initSpp;

    // State
    uint32_t m_currentSpp;
};