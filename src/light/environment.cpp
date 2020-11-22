#include "environment.h"
#include "core/framebuffer.h"

EnvironmentLight::EnvironmentLight(
    const std::shared_ptr<Texture<Spectrum>>& texture, const float& radius, const float& scale)
    :Light(MediumInterface(nullptr, nullptr)), m_texture(texture), m_radius(radius), m_scale(scale)
{
    std::vector<float> img;
    for (uint32_t i = 0; i < m_texture->m_height; i++) {
        float v = (i + 0.5f) / m_texture->m_height;
        float theta = (1 - v) * M_PI;
        float sinTheta = std::sin(theta);
        for (uint32_t j = 0; j < m_texture->m_width; j++) {
            float u = (j + 0.5f) / m_texture->m_width;
            img.push_back(m_texture->Evaluate(Float2(u, v)).y() * sinTheta);
        }
    }
    m_distribution.reset(new Distribution2D(&img[0], m_texture->m_width, m_texture->m_height));    
}

Spectrum EnvironmentLight::Eval(const Ray& ray) const
{   
    Float3 p = GetIntersectPoint(ray);
    Float2 st(1 - Frame::SphericalPhi(p) * INV_TWOPI, 1 - Frame::SphericalTheta(p) * INV_PI);
    return m_texture->Evaluate(st) * m_scale;
}

Spectrum EnvironmentLight::Sample(LightRecord& lightRec, Float2& s) const
{
    // Importance sample on environment light
    float pdf;
    Float2 st = m_distribution->Sample(s, pdf);

    // Calculate shadow ray
    float theta = (1 - st.y) * M_PI, phi = (1 - st.x) * TWO_PI;
    Float3 p = Float3(std::sin(theta) * std::cos(phi), std::sin(theta) * std::sin(phi), std::cos(theta)) * m_radius;
    Float3 d = Normalize(p - lightRec.m_ref);
    Float3 o = lightRec.m_ref;
    lightRec.m_wi = d;
    lightRec.m_shadowRay = Ray(lightRec.m_ref, lightRec.m_wi, Ray::epsilon, m_radius);
    
    // Calculate pdf
    lightRec.m_pdf = pdf / (2 * M_PI * M_PI * std::sin(theta));

    // Get emission
    Spectrum emission = m_texture->Evaluate(st) * m_scale;
    return emission / lightRec.m_pdf;
}

Spectrum EnvironmentLight::EvalPdf(LightRecord& lightRec) const
{
    Float3 p = GetIntersectPoint(Ray(lightRec.m_ref, lightRec.m_wi));
    Float2 st(1 - Frame::SphericalPhi(p) * INV_TWOPI, 1 - Frame::SphericalTheta(p) * INV_PI);
    float theta = (1 - st.y) * M_PI;
    
    lightRec.m_pdf = m_distribution->Pdf(st) / (2 * M_PI * M_PI * std::sin(theta));
    return m_texture->Evaluate(st) * m_scale;
}

Spectrum EnvironmentLight::SamplePhoton(Float2& s1, Float2& s2, Ray& ray) const
{
    std::cout << "No implementation!!!\n";
    exit(-1);
    return Spectrum();
}

void EnvironmentLight::TestSampling(const std::string& filename, const uint32_t& sampleNum) const
{

    uint32_t w = m_texture->m_width, h = m_texture->m_height;
    Framebuffer buffer(filename, w, h);

    for (uint32_t i = 0; i < h; i++) {
        for (uint32_t j = 0; j < w; j++) {
            Spectrum s = m_texture->Evaluate(Float2(float(j) / w, float(i) / h));            
            buffer.AddSample(j, i, Spectrum(s.y()));
        }
    }

    Sampler sampler;
    for (uint32_t i = 0; i < sampleNum; i++) {
        float pdf;
        Float2 uv = m_distribution->Sample(sampler.Next2D(), pdf);
        uv *= Float2(w, h);
        buffer.DrawCircle(uv.x, uv.y, 3, Spectrum(1, 0, 0));
    }

    buffer.Save();
}

Float3 EnvironmentLight::GetIntersectPoint(const Ray& ray) const
{
    //return ray.d;
    float a = Dot(ray.d, ray.d), b = 2.f * Dot(ray.o, ray.d), c = Dot(ray.o, ray.o) - (m_radius * m_radius);    
    LOG_IF(FATAL, b * b - 4 * a * c < 0) << ray.o << "Environment light's radius is too small";
    float det = std::sqrt(b * b - 4 * a * c);
    float t = (-b + det) / (2 * a);
    Float3 p = Normalize(ray.o + ray.d * t);
    return p;
}
