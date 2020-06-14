#include "sampling.h"

void CoordinateSystem(const Float3& a, Float3& b, Float3& c)
{
    if (std::abs(a.x) > std::abs(a.y)) {
        float invLen = 1.0f / std::sqrt(a.x * a.x + a.z * a.z);
        c = Float3(a.z * invLen, 0.0f, -a.x * invLen);
    }
    else {
        float invLen = 1.0f / std::sqrt(a.y * a.y + a.z * a.z);
        c = Float3(0.0f, a.z * invLen, -a.y * invLen);
    }
    b = Cross(c, a);
}

Float3 SampleCosineHemisphere(const Float2& s)
{
    float sinTheta = std::sqrt(s.x), cosTheta = std::sqrt(1 - s.x);
    float sinPhi = std::sin(M_PI * 2 * s.y), cosPhi = std::cos(M_PI * 2 * s.y);
    return Float3(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta);
}

float PdfCosineHemisphere(const Float3& v)
{
    return v.z * INV_PI;
}

Float2 SampleUniformTriangle(const Float2& s)
{
    float su = std::sqrt(s.x);
    return Float2(1 - su, s.y * su);
}

Float3 SampleUniformSphere(const Float2& s)
{
    float theta = std::acos(1 - 2 * s.x), phi = s.y * M_PI * 2;
    Float3 ret(std::sin(theta) * std::cos(phi), std::sin(theta) * std::sin(phi), std::cos(theta));
    return ret;
}

float PdfUniformSphere(const Float3& v)
{
    return INV_FOURPI;
}

Distribution1D::Distribution1D(const float* ptr, int n)
    :m_func(ptr, ptr + n), m_cdf(n + 1)
{
    m_cdf[0] = 0;
    for (int i = 1; i < n + 1; i++) {
        m_cdf[i] = m_cdf[i - 1] + m_func[i - 1] / n;
    }
    m_sum = m_cdf[n];
    for (int i = 1; i < n + 1; i++) {
        m_cdf[i] /= m_sum;
    }
}

float Distribution1D::Sample(const float& s, float& pdf) const
{
    int l = 0, r = m_cdf.size() - 1;
    while (l + 1 < r) {
        int mid = (l + r) * 0.5f;
        if (m_cdf[mid] <= s) l = mid;
        else r = mid;
    }
    pdf = m_func[l] / m_sum;
    return l + (s - m_cdf[l]) / (m_cdf[l + 1] - m_cdf[l]);
}

float Distribution1D::Pdf(const float& s) const
{
    uint32_t idx = std::min(uint64_t(s * m_func.size()), m_func.size() - 1);
    return m_func[idx] / m_sum;
}

Distribution2D::Distribution2D(const float* ptr, uint32_t nu, uint32_t nv)
    :m_nu(nu), m_nv(nv)
{
    m_conditional.reserve(m_nv);
    for (uint32_t i = 0; i < m_nv; i++) {
        m_conditional.emplace_back(new Distribution1D(ptr + m_nu * i, m_nu));
    }

    std::vector<float> sum;
    sum.reserve(m_nv);
    for (uint32_t i = 0; i < m_nv; i++) {
        sum.push_back(m_conditional[i]->GetSum());
    }
    m_marginal.reset(new Distribution1D(&sum[0], nv));
}

Float2 Distribution2D::Sample(const Float2& s, float& pdf) const
{
    float pdfU, pdfV, u, v;
    v = m_marginal->Sample(s.x, pdfV);
    u = m_conditional[std::min(uint32_t(v), m_nv - 1)]->Sample(s.y, pdfU);
    pdf = pdfU * pdfV;
    return Float2(u / m_nu, v / m_nv);
}

float Distribution2D::Pdf(const Float2& s) const
{
    int iv = std::min(uint32_t(s.y * m_nv), m_nv - 1);
    int iu = std::min(uint32_t(s.x * m_nu), m_nu - 1);    
    float pdf = m_conditional[iv]->m_func[iu] / m_marginal->GetSum();
    return pdf;
}

