#pragma once

#include "core/primitive.h"

class EnvironmentLight : public Light {    
public:
    EnvironmentLight(const std::shared_ptr<Texture<Spectrum>>& texture);

    Spectrum Eval(const Ray& ray) const;    
    Spectrum Sample(LightRecord& lightRec, Float2& s) const;    
    Spectrum Eval(LightRecord& lightRec) const { return Spectrum(0.f); }    
    float Pdf(LightRecord& lightRec) const { return 0.f; }    
    Spectrum EvalPdf(LightRecord& lightRec) const;

private:
    std::shared_ptr<Texture<Spectrum>> m_texture;
    std::shared_ptr<Distribution2D> m_distribution;
    float m_radius = 10000;
};