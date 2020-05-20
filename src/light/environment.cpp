#include "environment.h"
#include "core/framebuffer.h"

EnvironmentLight::EnvironmentLight(const std::shared_ptr<Texture<Spectrum>>& texture)
    :Light(MediumInterface(nullptr, nullptr)), m_texture(texture)
{
    std::vector<float> img;
    for (uint32_t i = 0; i < m_texture->m_height; i++) {
        for (uint32_t j = 0; j < m_texture->m_width; j++) {
            img.push_back(m_texture->Evaluate(
                Float2(float(j) / m_texture->m_width, float(i) / m_texture->m_height)).y());
        }
    }
    m_distribution.reset(new Distribution2D(&img[0], m_texture->m_width, m_texture->m_height));
}

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
    float pdf;
    Float2 st = m_distribution->Sample(s, pdf);
    float theta = (1 - st.y) * TWO_PI, phi = (1 - st.x)* M_PI;

    Float3 p = Float3(std::sin(theta) * std::cos(phi), std::sin(theta) * std::sin(phi), std::cos(theta)) * m_radius;
    Float3 d = Normalize(p - lightRec.m_ref);
    Float3 o = lightRec.m_ref;

    float a = Dot(d, d), b = 2.f * Dot(o, d), c = Dot(o, o) - (m_radius * m_radius);
    float det = std::sqrt(b * b - 4 * a * c);
    float t = (-b + det) / (2 * a);

    lightRec.m_geoRec.m_ng = lightRec.m_geoRec.m_ns = -p;
    lightRec.m_geoRec.m_p = p;
    lightRec.m_geoRec.m_pdf = pdf;
    lightRec.m_geoRec.m_st = lightRec.m_geoRec.m_uv = st;
            
    //std::cout << t << std::endl;
    lightRec.m_wi = d;
    lightRec.m_pdf = lightRec.m_geoRec.m_pdf / (2 * M_PI * M_PI * std::sin(theta));
    lightRec.m_shadowRay = Ray(lightRec.m_ref, lightRec.m_wi, Ray::epsilon, t * (1 - Ray::shadowEpsilon));
    Spectrum e = m_texture->Evaluate(st);
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
    lightRec.m_geoRec.m_pdf = m_distribution->Pdf(Float2((1 - st.x) * TWO_PI, (1 - st.y) * M_PI));
    lightRec.m_geoRec.m_st = lightRec.m_geoRec.m_uv = st;

    float theta = (1 - st.y) * TWO_PI;
    lightRec.m_pdf = lightRec.m_geoRec.m_pdf / (2 * M_PI * M_PI * std::sin(theta));
    lightRec.m_shadowRay = Ray(lightRec.m_ref, lightRec.m_wi, Ray::epsilon, t * (1 - Ray::shadowEpsilon));
    return m_texture->Evaluate(st);
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
            buffer.SetVal(j, i, Spectrum(s.y()));
        }
    }

    /*for (int i = 0; i < 10; i++) {
        buffer.DrawCircle(i, 0, 3, Spectrum(1, 0, 0));
    }
    buffer.Save();
    return;
    */
    
    Sampler sampler;
    for (uint32_t i = 0; i < sampleNum; i++) {
        float pdf;
        Float2 uv = m_distribution->Sample(sampler.Next2D(), pdf);
        uv *= Float2(w, h);
        buffer.DrawCircle(uv.x, uv.y, 3, Spectrum(1, 0, 0));
    }

    buffer.Save();
    
}
