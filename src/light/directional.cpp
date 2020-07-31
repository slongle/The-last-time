#include "directional.h"

Spectrum DirectionalLight::Sample(LightRecord& lightRec, Float2& s) const
{
    lightRec.m_pdf = 1;
    lightRec.m_wi = m_direction;
    lightRec.m_shadowRay = Ray(lightRec.m_ref, m_direction);
    return m_irrandance;
}

Spectrum DirectionalLight::Eval(LightRecord& lightRec) const
{
    LOG(FATAL) << "No implementation.";
    return Spectrum();
}

float DirectionalLight::Pdf(LightRecord& lightRec) const
{
    LOG(FATAL) << "No implementation.";
    return 0.0f;
}

Spectrum DirectionalLight::EvalPdf(LightRecord& lightRec) const
{
    return Spectrum(0.f);
}

Spectrum DirectionalLight::SamplePhoton(Float2& s1, Float2& s2, Ray& ray) const
{
    LOG(FATAL) << "No implementation.";
    return Spectrum();
}
