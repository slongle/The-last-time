#pragma once

#include "core/primitive.h"
#include "bsdf/bsdfutil.h"

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
        const float& eta) 
        : m_reflectance(refl), m_transmittance(tran), m_eta(eta), m_invEta(1.f / eta) {}

    Spectrum Sample(MaterialRecord& matRec, Float2 s) const {
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
            float factor = cosThetaT < 0.f ? m_invEta : m_eta;
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
                float factor = cosThetaT < 0.f ? m_invEta : m_eta;
                return m_transmittance->Evaluate(matRec.m_st) * (factor * factor) * (1 - Fr);
            }
        }
        return Spectrum(0.f);
    }
    bool IsDelta() const { return true; }
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