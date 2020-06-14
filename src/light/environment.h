#pragma once

#include "core/primitive.h"

class EnvironmentLight : public Light {    
public:
    EnvironmentLight(const std::shared_ptr<Texture<Spectrum>>& texture, const float& radius, const float& scale);

    Spectrum Eval(const Ray& ray) const;    
    Spectrum Sample(LightRecord& lightRec, Float2& s) const;    
    Spectrum Eval(LightRecord& lightRec) const { return Spectrum(0.f); }
    float Pdf(LightRecord& lightRec) const { return 0.f; }
    Spectrum EvalPdf(LightRecord& lightRec) const;
    Spectrum SamplePhoton(Float2& s1, Float2& s2, Ray& ray) const;
    // Test
    void TestSampling(const std::string& filename, const uint32_t& sampleNum) const;

private:
    Float3 GetIntersectPoint(const Ray& ray) const;

    std::shared_ptr<Texture<Spectrum>> m_texture;
    std::shared_ptr<Distribution2D> m_distribution;
    float m_radius, m_scale;
};