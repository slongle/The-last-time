#include "environment.h"

Spectrum EnvironmentLight::Eval(const Ray& ray) const
{
    float a = Dot(ray.d, ray.d), b = 2.f * Dot(ray.o, ray.d), c = Dot(ray.o, ray.o) - 1e8;
    float det = std::sqrt(b * b - 4 * a * c);
    float t = (-b + det) / (2 * a);
    Float3 p = Normalize(ray.o + ray.d * t);
    Float2 st(1 - Frame::SphericalPhi(p) * INV_TWOPI, 1 - Frame::SphericalTheta(p) * INV_PI);
    return m_texture->Evaluate(st);
}
