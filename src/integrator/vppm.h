#pragma once

#include "core/integrator.h"

#include "photon.h"

#include <atomic>
#include <thread>
#include <mutex>

class VPPMIntegrator : public Integrator {
public:
    VPPMIntegrator(
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

    void Start();
    void Stop() { m_rendering = false; }
    void Wait() {
        if (m_renderThread->joinable()) {
            m_renderThread->join();
        }
    }
    bool IsRendering() { return m_rendering; }
    void Save();
    std::string ToString() const;
private:
    void RenderTile(const Framebuffer::Tile& tile);
    void EmitPhoton(const uint32_t& photonIndex);
    Spectrum Li(Ray ray, Sampler& sampler);
    Spectrum EstimateMediumPoint3D(
        const Ray& ray,
        const MediumRecord& mediumRec,
        const std::shared_ptr<PhaseFunction>& phase);
    Spectrum EstimatePlane(
        const Ray& ray,
        const HitRecord& hitRec,
        const std::shared_ptr<BSDF>& bsdf);
private:
    void Debug(DebugRecord& debugRec);
    Spectrum LiDebug(Ray ray, Sampler& sampler);
private:
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

    // Photons
    KDTree m_photonTree;

    // Muti-thread setting
    std::atomic<bool> m_rendering;
    std::vector<Framebuffer::Tile> m_tiles;
    std::thread* m_renderThread;
};