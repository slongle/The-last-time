#pragma once

#include "core/primitive.h"
#include "bsdf/bsdfutil.h"
#include "bsdf/microfacet.h"

/*
 * eta = int_ior / ext_ior = eta_t / eta_i (e.g. glass_ior / air_ior)
 * cosThetaI > 0, ext -> int (air -> glass)
 * cosThetaI < 0, int -> ext (glass -> air)
 */

class SmoothDielectric : public BSDF {
public:
    SmoothDielectric(
        const std::shared_ptr<Texture<Spectrum>>& refl,
        const std::shared_ptr<Texture<Spectrum>>& tran,
        const float& eta,
        const std::shared_ptr<Texture<float>>& alpha)
        :BSDF(alpha), m_reflectance(refl), m_transmittance(tran), m_eta(eta), m_invEta(1.f / eta) {}

    Spectrum Sample(MaterialRecord& matRec, Float2 s) const {
        // Alpha texture
        bool opaque = !(m_alpha->Evaluate(matRec.m_st) >= 0.99f);
        if (opaque) {
            matRec.m_wo = -matRec.m_wi;
            matRec.m_pdf = 1.f;
            return Spectrum(1.f);
        }

        float cosThetaT;
        float Fr = FresnelDieletric(Frame::CosTheta(matRec.m_wi), cosThetaT, m_eta);
        if (s.x < Fr) {
            // Reflect
            matRec.m_pdf = Fr;
            matRec.m_wo = Reflect(matRec.m_wi);
            matRec.m_eta = 1.f;
            return m_reflectance->Evaluate(matRec.m_st);
        }
        else {
            // Refract
            matRec.m_pdf = 1 - Fr;
            matRec.m_wo = Refract(matRec.m_wi, cosThetaT);
            matRec.m_eta = cosThetaT < 0.f ? m_eta : m_invEta;
            float factor = matRec.m_mode == Radiance ? (cosThetaT < 0.f ? m_invEta : m_eta) : 1.f;
            return m_transmittance->Evaluate(matRec.m_st) * (factor * factor);
        }
    }

    Spectrum Eval(MaterialRecord& matRec) const { return Spectrum(0.f); }
    float Pdf(MaterialRecord& matRec) const { matRec.m_pdf = 0.f; return 0.f; }
    Spectrum EvalPdf(MaterialRecord& matRec) const { 
        float cosThetaT;
        float Fr = FresnelDieletric(Frame::CosTheta(matRec.m_wi), cosThetaT, m_eta);
        if (Frame::SameHemisphere(matRec.m_wi, matRec.m_wo)) {
            // Reflect
            if (Frame::SameDirection(matRec.m_wo, Reflect(matRec.m_wi))) {
                matRec.m_pdf = Fr;
                return m_reflectance->Evaluate(matRec.m_st) * Fr;
            }
        }
        else {
            // Refract
            if (Frame::SameDirection(matRec.m_wo, Refract(matRec.m_wi, cosThetaT))) {
                matRec.m_pdf = 1 - Fr;
                float factor = matRec.m_mode == Radiance ? (cosThetaT < 0.f ? m_invEta : m_eta) : 1.f;
                return m_transmittance->Evaluate(matRec.m_st) * (factor * factor) * (1 - Fr);
            }
        }
        return Spectrum(0.f);
    }
    bool IsDelta(const Float2& st) const {
        return true;
    }
private:
    Float3 Reflect(const Float3& v) const {
        return Float3(-v.x, -v.y, v.z);
    }

    Float3 Refract(const Float3& v, const float& cosThetaT) const {
        float factor = -(cosThetaT < 0.f ? m_invEta : m_eta);
        return Float3(factor * v.x, factor * v.y, cosThetaT);
    }

    float m_eta, m_invEta;
    std::shared_ptr<Texture<Spectrum>> m_reflectance;
    std::shared_ptr<Texture<Spectrum>> m_transmittance;
};


class RoughDielectric : public BSDF {
public:
    RoughDielectric(
        const std::shared_ptr<Texture<Spectrum>>& refl,
        const std::shared_ptr<Texture<Spectrum>>& tran,
        const float& eta,
        const float& alphaU, const float& alphaV,
        const std::shared_ptr<Texture<float>>& alpha)
        :BSDF(alpha), m_reflectance(refl), m_transmittance(tran), m_eta(eta), m_invEta(1.f / eta),
        m_alphaU(alphaU), m_alphaV(alphaV) {}

    Spectrum Sample(MaterialRecord& matRec, Float2 s) const {
        // Alpha texture
        bool opaque = !(m_alpha->Evaluate(matRec.m_st) >= 0.99f);
        if (opaque) {
            matRec.m_wo = -matRec.m_wi;
            matRec.m_pdf = 1.f;
            return Spectrum(1.f);
        }

        float sign = math::signum(Frame::CosTheta(matRec.m_wi));

        Microfacet microfacet(Float2(m_alphaU, m_alphaV));
        Float3 wh = microfacet.SampleVisible(sign * matRec.m_wi, s); 
        float pdf_wh = microfacet.PdfVisible(sign * matRec.m_wi, wh);
        if (pdf_wh == 0) {
            return Spectrum(0.f);
        }
        float D = microfacet.D(wh);

        float cosThetaT;
        float F = FresnelDieletric(Dot(matRec.m_wi, wh), cosThetaT, m_eta);        
        if (s.x < F) {
            // Reflect
            matRec.m_wo = Reflect(matRec.m_wi, wh);
            if (Frame::CosTheta(matRec.m_wi) * Frame::CosTheta(matRec.m_wo) <= 0.f) {
                return Spectrum(0.f);
            }
            matRec.m_eta = 1.f;
            float dwh_dwo = 1.0f / (4 * AbsDot(matRec.m_wi,wh));
            matRec.m_pdf = F * pdf_wh * dwh_dwo;
            return m_reflectance->Evaluate(matRec.m_st) * microfacet.G1(matRec.m_wo, wh);
        }
        else {
            // Refract
            if (cosThetaT == 0) {
                return Spectrum(0.f);
            }
            float eta = cosThetaT < 0.f ? m_eta : m_invEta;
            matRec.m_wo = Refract(matRec.m_wi, wh, cosThetaT);
            if (Frame::CosTheta(matRec.m_wi) * Frame::CosTheta(matRec.m_wo) >= 0.f) {
                return Spectrum(0.f);
            }
            float sqrtDenom = Dot(matRec.m_wi, wh) + eta * Dot(matRec.m_wo, wh);
            float dwh_dwo = eta * eta * AbsDot(matRec.m_wo, wh) / (sqrtDenom * sqrtDenom);
            matRec.m_pdf = (1 - F) * pdf_wh * dwh_dwo;
            matRec.m_eta = eta;
            float factor = matRec.m_mode == Radiance ? (cosThetaT < 0.f ? m_invEta : m_eta) : 1.f;
            return m_transmittance->Evaluate(matRec.m_st) * (factor * factor) * 
                microfacet.G1(matRec.m_wo, wh);
        }
    }

    Spectrum Eval(MaterialRecord& matRec) const { return Spectrum(0.f); }
    float Pdf(MaterialRecord& matRec) const { matRec.m_pdf = 0.f; return 0.f; }
    /*
     * CosTheta(wi) > 0 is not the same as cosThetaT < 0
     * The former one is wi.z, the latter one is Dot(wo, wh)
     * For example, 
     * wo = array([0.714, 0.578, 0.394])
     * wi = array([-0.488, -0.845, -0.217])
     * eta_i = 1.5, eta_o = 1
     * then wh = Normalize(eta_i * wi + eta_o + wo)
     * wh = array([-0.02596923, -0.99476568,  0.09882734])
     * so 
     * CosTheta(wi) = wi.z = -0.217 < 0
     * cosThetaT = Dot(wo, wh) = -0.554 < 0
     */
    Spectrum EvalPdf(MaterialRecord& matRec) const {
        bool reflect = Frame::SameHemisphere(matRec.m_wi, matRec.m_wo);        
       
        Float3 wh;
        float dwh_dwo;

        
        if (reflect) {
            // Reflect
            wh = Normalize(matRec.m_wo + matRec.m_wi);
            dwh_dwo = 1 / (4 * AbsDot(matRec.m_wo, wh));
        }
        else {
            // Refract
            float eta = Frame::CosTheta(matRec.m_wi) > 0.f ? m_eta : m_invEta;
            wh = Normalize(matRec.m_wi + eta * matRec.m_wo);
            float sqrtDenom = Dot(matRec.m_wi, wh) + eta * Dot(matRec.m_wo, wh);
            dwh_dwo = eta * eta * AbsDot(matRec.m_wo,wh) / (sqrtDenom * sqrtDenom);
        }

        wh *= math::signum(Frame::CosTheta(wh));
        Microfacet microfacet(Float2(m_alphaU, m_alphaV));
        float D = microfacet.D(wh);
        if (D == 0) {
            return Spectrum(0.f);
        }
        float G = microfacet.G(matRec.m_wi, matRec.m_wo, wh);

        float cosThetaT;
        float F = FresnelDieletric(Dot(matRec.m_wi, wh), cosThetaT, m_eta);

        Float3 wi = math::signum(Frame::CosTheta(matRec.m_wi)) * matRec.m_wi;
        matRec.m_pdf = microfacet.PdfVisible(wi, wh) * dwh_dwo * (reflect ? F : 1 - F);
        if (matRec.m_pdf == 0) {
            return Spectrum(0.f);
        }
         
        if (reflect) {
            float fs = F * D * G / (4 * Frame::AbsCosTheta(matRec.m_wi));            
            return m_reflectance->Evaluate(matRec.m_st) * fs;
        }
        else {
            float eta = Frame::CosTheta(matRec.m_wi) > 0.0f ? m_eta : m_invEta;
            float sqrtDenom = Dot(matRec.m_wi, wh) + eta * Dot(matRec.m_wo, wh);
            float fs = (1 - F) * G * D * AbsDot(matRec.m_wo, wh) * AbsDot(matRec.m_wi, wh)
                * eta * eta / (Frame::AbsCosTheta(matRec.m_wi) * sqrtDenom * sqrtDenom);
            float factor = matRec.m_mode == Radiance ? 
                (Frame::CosTheta(matRec.m_wi) > 0.0f ? m_invEta : m_eta) : 1.f;            
            return m_transmittance->Evaluate(matRec.m_st) * (factor * factor) * fs;
        }
    }
    bool IsDelta(const Float2& st) const {
        return BSDF::IsDelta(st);
    }
private:
    Float3 Reflect(const Float3& v, const Float3& n) const {
        return 2 * Dot(v, n) * n - v;
    }

    Float3 Refract(const Float3& v, const Float3& n, const float& cosThetaT) const {
        float factor = -(cosThetaT < 0.f ? m_invEta : m_eta);
        return n * (-Dot(v, n) * factor + cosThetaT) + v * factor;
    }

    float m_eta, m_invEta;
    float m_alphaU, m_alphaV;
    std::shared_ptr<Texture<Spectrum>> m_reflectance;
    std::shared_ptr<Texture<Spectrum>> m_transmittance;
};