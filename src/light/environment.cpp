#include "environment.h"

Spectrum EnvironmentLight::Eval(const Ray& ray) const
{
    float a = Dot(ray.d, ray.d), b = 2.f * Dot(ray.o, ray.d), c = Dot(ray.o, ray.o) - (m_radius * m_radius);
    float det = std::sqrt(b * b - 4 * a * c);
    float t = (-b + det) / (2 * a);
    Float3 p = Normalize(ray.o + ray.d * t);
    Float2 st(1 - Frame::SphericalPhi(p) * INV_TWOPI, 1 - Frame::SphericalTheta(p) * INV_PI);
    return m_texture->Evaluate(st);
}

Spectrum EnvironmentLight::Sample(LightRecord& lightRec, Float2& s) const
{
    float theta = std::acos(1 - 2 * s.x), phi = s.y * M_PI * 2;
    Float3 p = Float3(std::sin(theta) * std::cos(phi), std::sin(theta) * std::sin(phi), std::cos(theta)) * m_radius;
    float pdf = INV_FOURPI;
    Float3 d = Normalize(p - lightRec.m_ref);
    Float3 o = lightRec.m_ref;

    float a = Dot(d, d), b = 2.f * Dot(o, d), c = Dot(o, o) - (m_radius * m_radius);
    float det = std::sqrt(b * b - 4 * a * c);
    float t = (-b + det) / (2 * a);
    Float2 st(1 - phi * INV_TWOPI, 1 - theta * INV_PI);

    lightRec.m_geoRec.m_ng = lightRec.m_geoRec.m_ns = -p;
    lightRec.m_geoRec.m_p = p;
    lightRec.m_geoRec.m_pdf = INV_FOURPI;
    lightRec.m_geoRec.m_st = lightRec.m_geoRec.m_uv = st;
            
    lightRec.m_wi = d;
    lightRec.m_pdf = lightRec.m_geoRec.m_pdf;
    lightRec.m_shadowRay = Ray(lightRec.m_ref, lightRec.m_wi, Ray::epsilon, t * (1 - Ray::shadowEpsilon));
    return m_texture->Evaluate(st) / lightRec.m_pdf;
}

Spectrum EnvironmentLight::EvalPdf(LightRecord& lightRec) const
{
    float a = Dot(lightRec.m_wi, lightRec.m_wi), 
          b = 2.f * Dot(lightRec.m_ref, lightRec.m_wi), 
          c = Dot(lightRec.m_ref, lightRec.m_ref) - (m_radius * m_radius);
    float det = std::sqrt(b * b - 4 * a * c);
    float t = (-b + det) / (2 * a);
    Float3 p = Normalize(lightRec.m_ref + lightRec.m_wi * t);
    Float2 st(1 - Frame::SphericalPhi(p) * INV_TWOPI, 1 - Frame::SphericalTheta(p) * INV_PI);

    lightRec.m_geoRec.m_p = lightRec.m_ref + lightRec.m_wi * t;
    lightRec.m_geoRec.m_pdf = INV_FOURPI;
    lightRec.m_geoRec.m_st = lightRec.m_geoRec.m_uv = st;

    lightRec.m_pdf = lightRec.m_geoRec.m_pdf;
    lightRec.m_shadowRay = Ray(lightRec.m_ref, lightRec.m_wi, Ray::epsilon, t * (1 - Ray::shadowEpsilon));
    return m_texture->Evaluate(st);
}
