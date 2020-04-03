#pragma once

#include "core/integrator.h"

#include <atomic>
#include <thread>
#include <mutex>

class PathIntegrator : public Integrator {
public:
    PathIntegrator(
        const std::shared_ptr<Scene>& scene,
        const std::shared_ptr<Camera>& camera,
        const std::shared_ptr<Framebuffer>& buffer,
        const uint32_t maxBounce,
        const uint32_t spp)
        : Integrator(scene, camera, buffer), m_maxBounce(maxBounce), m_spp(spp) {}

    Spectrum Li(Ray ray, Sampler& sampler);
    void Start();
    void Stop();
    void Wait();
    bool IsRendering();
private:
    void Setup();
    void ThreadWork();
    void RenderTile(const Framebuffer::Tile& tile);

    Spectrum NormalCheck(Ray ray, Sampler& sampler);

    // Muti-thread setting
    bool m_rendering;
    std::atomic<int> m_renderingNum;
    std::mutex m_tileMutex;
    std::vector<std::thread*> m_threads;
    std::vector<Framebuffer::Tile> m_tiles;

    // Options
    uint32_t m_maxBounce;
    uint32_t m_spp;
};