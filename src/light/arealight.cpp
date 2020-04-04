#include "arealight.h"

Spectrum AreaLight::Sample(LightRecord& lightRec, Float2& s) const
{
    assert(m_shape);
    m_shape->Sample(lightRec.m_geoRec, s);
    Float3 dir = lightRec.m_geoRec.m_p - lightRec.m_ref;
    if (Dot(lightRec.m_geoRec.m_ns, -dir) > 0) {
        float dist = Length(dir);
        lightRec.m_wi = Normalize(dir);
        lightRec.m_pdf = lightRec.m_geoRec.m_pdf * dist * dist / Dot(-lightRec.m_wi, lightRec.m_geoRec.m_ns);
        lightRec.m_shadowRay = Ray(lightRec.m_ref, lightRec.m_wi, Ray::Epsilon, dist - Ray::Epsilon);
        return m_radiance / lightRec.m_pdf;
    }
    else {        
        return Spectrum(0.f);
    }
}

Spectrum AreaLight::Eval(LightRecord& lightRec) const
{
    assert(m_shape);
    Float3 dir = lightRec.m_geoRec.m_p - lightRec.m_ref;
    if (Dot(lightRec.m_geoRec.m_ns, -dir) > 0) {
        return m_radiance;
    }
    else {        
        return Spectrum(0.f);
    }
}

float AreaLight::Pdf(LightRecord& lightRec) const
{
    assert(m_shape);
    Float3 dir = lightRec.m_geoRec.m_p - lightRec.m_ref;
    m_shape->Pdf(lightRec.m_geoRec);
    if (Dot(lightRec.m_geoRec.m_ns, -dir) > 0) {
        float dist = Length(dir);
        lightRec.m_wi = Normalize(dir);
        lightRec.m_pdf = lightRec.m_geoRec.m_pdf * dist * dist / Dot(-lightRec.m_wi, lightRec.m_geoRec.m_ns);
        lightRec.m_shadowRay = Ray(lightRec.m_ref, lightRec.m_wi, Ray::Epsilon, dist - Ray::Epsilon);
        return lightRec.m_pdf;
    }
    else {
        return 0.f;
    }
}

Spectrum AreaLight::EvalPdf(LightRecord& lightRec) const
{
    assert(m_shape);
    Float3 dir = lightRec.m_geoRec.m_p - lightRec.m_ref;
    m_shape->Pdf(lightRec.m_geoRec);
    if (Dot(lightRec.m_geoRec.m_ns, -dir) > 0) {
        float dist = Length(dir);
        lightRec.m_wi = Normalize(dir);
        lightRec.m_pdf = lightRec.m_geoRec.m_pdf * dist * dist / Dot(-lightRec.m_wi, lightRec.m_geoRec.m_ns);
        lightRec.m_shadowRay = Ray(lightRec.m_ref, lightRec.m_wi, Ray::Epsilon, dist - Ray::Epsilon);
        return m_radiance;
    }
    else {
        return Spectrum(0.f);
    }
}
