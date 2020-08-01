#pragma once

#include "core/primitive.h"

class DirectionalLight :public Light {
public:
    DirectionalLight(const Spectrum& irrandance, const Float3& direction)
        :Light(MediumInterface(nullptr, nullptr)), m_irrandance(irrandance), m_direction(Normalize(direction)) {}

    Spectrum Sample(LightRecord& lightRec, Float2& s) const;
    Spectrum Eval(LightRecord& lightRec) const;
    float Pdf(LightRecord& lightRec) const;
    Spectrum EvalPdf(LightRecord& lightRec) const;
    Spectrum SamplePhoton(Float2& s1, Float2& s2, Ray& ray) const;
private:
    Spectrum m_irrandance;
    Float3 m_direction;

};