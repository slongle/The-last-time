#pragma once

#include "core/integrator.h"

class PathIntegrator : public SampleIntegrator {
public:
    PathIntegrator(
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
    Spectrum EvalLight(
        bool hit, 
        const Ray& ray, 
        const HitRecord& hitRec) const;
    Spectrum EvalPdfLight(
        bool hit, 
        const Ray& ray, 
        const HitRecord& hitRec, 
        LightRecord& lightRec) const;
    Spectrum SampleLight(
        LightRecord& lightRec, 
        Float2& s) const;    
    // Debug
    void Debug(DebugRecord& debugRec);    
    void DebugRay(Ray ray, Sampler& sampler);
private:
    // Options
    uint32_t m_maxBounce;    
};