#pragma once

#include "core/integrator.h"

#include <atomic>
#include <thread>
#include <mutex>

class VolumePathIntegrator : public Integrator {
public:
    VolumePathIntegrator(
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
    std::string ToString() const;
    // Debug
    void Debug(DebugRecord& debugRec);
private:
    void Setup();
    void ThreadWork();
    void RenderTile(const Framebuffer::Tile& tile);
    // Misc
    float PowerHeuristic(float a, float b) const;
    // Debug
    Spectrum NormalCheck(Ray ray, Sampler& sampler);
    void DebugRay(Ray ray, Sampler& sampler);
private:
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