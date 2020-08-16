#include "point.h"

Spectrum PointLight::Sample(LightRecord& lightRec, Float2& s) const
{
    lightRec.m_pdf = 1;
    Float3 dir = m_position - lightRec.m_ref;
    float dist = Length(dir);
    lightRec.m_wi = dir / dist;
    lightRec.m_shadowRay = Ray(lightRec.m_ref, lightRec.m_wi, Ray::epsilon, dist * (1 - Ray::shadowEpsilon));
    return m_intensity / (dist * dist);
}

Spectrum PointLight::Eval(LightRecord& lightRec) const
{
    LOG(FATAL) << "No implementation.";
    return Spectrum(0.f);
}

float PointLight::Pdf(LightRecord& lightRec) const
{
    LOG(FATAL) << "No implementation.";
    return 0.0f;
}

Spectrum PointLight::EvalPdf(LightRecord& lightRec) const
{
    return Spectrum(0.f);
}

Spectrum PointLight::SamplePhoton(Float2& s1, Float2& s2, Ray& ray) const
{
    Float3 dir = SampleUniformSphere(s1);
    float dirPdf = PdfUniformSphere(dir);

    ray = Ray(m_position, dir, this->m_mediumInterface.m_outsideMedium);

    return m_intensity / dirPdf;
}
