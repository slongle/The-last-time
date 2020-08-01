#pragma once

#include "core/integrator.h"

class VolumePathIntegrator : public SampleIntegrator {
public:
    VolumePathIntegrator(
        const std::shared_ptr<Scene>& scene,
        const std::shared_ptr<Camera>& camera,
        const std::shared_ptr<Framebuffer>& buffer,
        const uint32_t maxBounce,
        const uint32_t spp)
        : SampleIntegrator(scene, camera, buffer, spp), m_maxBounce(maxBounce) {}

    Spectrum Li(Ray ray, Sampler& sampler);
    std::string ToString() const;    
private:
    void Setup() { Integrator::Setup(); }
    // Debug
    void Debug(DebugRecord& debugRec);
    void DebugRay(Ray ray, Sampler& sampler);
private:
    // Options
    uint32_t m_maxBounce;    
};