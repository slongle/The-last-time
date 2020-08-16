#pragma once

#include "core/primitive.h"

class PointLight :public Light {
public:
    PointLight(const Float3& position, const Spectrum& intensity, const MediumInterface& mi)
        :Light(mi), m_intensity(intensity), m_position(position) {}

    Spectrum Sample(LightRecord& lightRec, Float2& s) const;
    Spectrum Eval(LightRecord& lightRec) const;
    float Pdf(LightRecord& lightRec) const;
    Spectrum EvalPdf(LightRecord& lightRec) const;
    Spectrum SamplePhoton(Float2& s1, Float2& s2, Ray& ray) const;
private:
    Spectrum m_intensity;
    Float3 m_position;
};