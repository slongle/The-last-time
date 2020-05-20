#include "hg.h"

Spectrum HenyeyGreenstein::EvalPdf(PhaseFunctionRecord& phaseRec) const
{
    float cosTheta = -Dot(phaseRec.m_wo, phaseRec.m_wi);
    float denom = 1 + m_g * m_g - 2 * m_g * cosTheta;
    float pdf = INV_FOURPI * (1 - m_g * m_g) / (denom * std::sqrt(denom));
    phaseRec.m_pdf = pdf;
    return Spectrum(pdf);
}

Spectrum HenyeyGreenstein::Sample(PhaseFunctionRecord& phaseRec, Float2& s) const
{
    float cosTheta;
    if (std::fabs(m_g) < 1e-3) {
        cosTheta = 1 - 2 * s.x;
    }
    else {
        float sqrTerm = (1 - m_g * m_g) / (1 - m_g + 2 * m_g * s.x);
        cosTheta = (1 + m_g * m_g - sqrTerm * sqrTerm) / (2 * m_g);
    }
    float denom = 1 + m_g * m_g - 2 * m_g * cosTheta;
    phaseRec.m_pdf = INV_FOURPI * (1 - m_g * m_g) / (denom * std::sqrt(denom));

    Frame frame(-phaseRec.m_wi);
    float phi = TWO_PI * s.y;
    phaseRec.m_wo = frame.ToWorld(Frame::SphericalToDirect(cosTheta, phi));
    return Spectrum(1.f);
}
