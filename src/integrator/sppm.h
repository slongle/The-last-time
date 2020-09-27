#pragma once
#include "core/integrator.h"

#include "photon.h"

#include <atomic>
#include <thread>
#include <mutex>

class GatherPoint {
public:
    GatherPoint() :m_throughphut(0.f), m_sumEmission(0.f), m_emission(0.f), m_radius(0.f), m_num(0.f) {}

    HitRecord m_hitRec;
    Spectrum m_throughphut;
    Spectrum m_sumEmission;
    Spectrum m_emission;
    float m_radius;
    float m_num;
    Int2 m_pos;
};

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
    void InitializeGatherPoints();
    void PhotonPass(int index);
    void BuildPhotonMap();
    void CameraPass(int index);
    void Update();
    Spectrum EstimatePlane(
        const HitRecord& hitRec,
        float radius);

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

    // Photons
    KDTree m_photonPlane;

    // Muti-thread setting
    std::atomic<bool> m_rendering;
    std::vector<std::vector<GatherPoint>> m_gatherBlocks;
    std::thread* m_renderThread;
};