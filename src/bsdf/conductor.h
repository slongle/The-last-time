#pragma once

#include "core/primitive.h"
#include "microfacet.h"
#include "bsdfutil.h"

class SmoothConductor :public BSDF {
public:
    SmoothConductor(
        const std::shared_ptr<Texture<Spectrum>>& reflectance,
        const Spectrum& eta, const Spectrum& k,
        const std::shared_ptr<Texture<float>>& alpha)
        :BSDF(alpha), m_reflectance(reflectance), m_eta(eta), m_k(k) {}

    Spectrum Sample(MaterialRecord& matRec, Float2 s) const {
        if (Frame::CosTheta(matRec.m_wi) <= 0) {
            return Spectrum(0.f);
        }

        // Alpha texture
        bool opaque = !(m_alpha->Evaluate(matRec.m_st) >= 0.99f);
        if (opaque) {
            matRec.m_wo = -matRec.m_wi;
            matRec.m_pdf = 1.f;
            return Spectrum(1.f);
        }

        matRec.m_wo = Reflect(matRec.m_wi);
        matRec.m_pdf = 1.f;
        matRec.m_eta = 1.f;
        return m_reflectance->Evaluate(matRec.m_st) *
            FresnelConductor(Frame::CosTheta(matRec.m_wi), m_eta, m_k);
    }
    Spectrum Eval(MaterialRecord& matRec) const {
        if (Frame::CosTheta(matRec.m_wi) <= 0 ||
            Frame::CosTheta(matRec.m_wo) <= 0 ||
            !Frame::SameDirection(matRec.m_wo, Reflect(matRec.m_wi))) {
            return Spectrum(0.f);
        }
        return m_reflectance->Evaluate(matRec.m_st) *
            FresnelConductor(Frame::CosTheta(matRec.m_wi), m_eta, m_k);
    }
    float Pdf(MaterialRecord& matRec) const {
        if (false || Frame::CosTheta(matRec.m_wi) <= 0 ||
            Frame::CosTheta(matRec.m_wo) <= 0 ||
            !Frame::SameDirection(matRec.m_wo, Reflect(matRec.m_wi))) {
            return matRec.m_pdf = 0.f;
        }
        return matRec.m_pdf = 1.f;
    }
    Spectrum EvalPdf(MaterialRecord& matRec) const {
        if (Frame::CosTheta(matRec.m_wi) <= 0 ||
            Frame::CosTheta(matRec.m_wo) <= 0 ||
            !Frame::SameDirection(matRec.m_wo, Reflect(matRec.m_wi))) {
            matRec.m_pdf = 0.f;
            return Spectrum(0.f);
        }
        matRec.m_pdf = 1.f;
        return m_reflectance->Evaluate(matRec.m_st) *
            FresnelConductor(Frame::CosTheta(matRec.m_wi), m_eta, m_k);
    }
    bool IsDelta(const Float2& st) const { return true; }
private:    

    Spectrum m_eta, m_k;
    std::shared_ptr<Texture<Spectrum>> m_reflectance;
};

class RoughConductor :public BSDF {
public:
    RoughConductor(
        const std::shared_ptr<Texture<Spectrum>>& reflectance,
        const Spectrum& eta, const Spectrum& k,
        const float& alphaU, const float& alphaV,
        const std::shared_ptr<Texture<float>>& alphaTexture)
        :BSDF(alphaTexture), m_reflectance(reflectance), m_eta(eta), m_k(k),
        m_alphaU(alphaU), m_alphaV(alphaV) {}

    Spectrum Sample(MaterialRecord& matRec, Float2 s) const {
        if (Frame::CosTheta(matRec.m_wi) <= 0) {
            return Spectrum(0.f);
        }

        // Alpha texture
        bool opaque = !(m_alpha->Evaluate(matRec.m_st) >= 0.99f);
        if (opaque) {
            matRec.m_wo = -matRec.m_wi;
            matRec.m_pdf = 1.f;
            return Spectrum(1.f);
        }

        Microfacet microfacet(Float2(m_alphaU, m_alphaV));
        Float3 wh = microfacet.SampleVisible(matRec.m_wi, s);
        float D = microfacet.D(wh);
        matRec.m_wo = Reflect(matRec.m_wi, wh);

        Spectrum F = m_reflectance->Evaluate(matRec.m_st) *
            FresnelConductor(Frame::CosTheta(matRec.m_wi), m_eta, m_k);

        Spectrum weight = F * microfacet.G1(matRec.m_wo, wh);;
        float pdf = microfacet.G1(matRec.m_wi, wh) * D / (4 * Frame::CosTheta(matRec.m_wi));

        matRec.m_pdf = pdf;
        matRec.m_eta = 1.f;
        return weight;
    }
    Spectrum Eval(MaterialRecord& matRec) const {
        if (Frame::CosTheta(matRec.m_wi) <= 0 ||
            Frame::CosTheta(matRec.m_wo) <= 0) {
            return Spectrum(0.f);
        }

        Float3 wh = Normalize((matRec.m_wi + matRec.m_wo));
        Microfacet microfacet(Float2(m_alphaU, m_alphaV));
        Spectrum F = m_reflectance->Evaluate(matRec.m_st) *
            FresnelConductor(Frame::CosTheta(matRec.m_wi), m_eta, m_k);
        float G = microfacet.G(matRec.m_wi, matRec.m_wo, wh);
        float D = microfacet.D(wh);

        float absCosTheta = Frame::AbsCosTheta(matRec.m_wi);
        Spectrum fs = F * D * G / (4 * absCosTheta);

        return fs;
    }
    float Pdf(MaterialRecord& matRec) const {
        if (Frame::CosTheta(matRec.m_wi) <= 0 ||
            Frame::CosTheta(matRec.m_wo) <= 0) {
            return matRec.m_pdf = 0.f;
        }

        Float3 wh = Normalize((matRec.m_wi + matRec.m_wo));
        Microfacet microfacet(Float2(m_alphaU, m_alphaV));
        float absCosTheta = Frame::AbsCosTheta(matRec.m_wi);
        float pdf = microfacet.D(wh) * microfacet.G1(matRec.m_wi, wh) / (4 * absCosTheta);

        return matRec.m_pdf = pdf;
    }
    Spectrum EvalPdf(MaterialRecord& matRec) const {
        if (Frame::CosTheta(matRec.m_wi) <= 0 ||
            Frame::CosTheta(matRec.m_wo) <= 0) {
            matRec.m_pdf = 0.f;
            return Spectrum(0.f);
        }

        Float3 wh = Normalize((matRec.m_wi + matRec.m_wo));
        Microfacet microfacet(Float2(m_alphaU, m_alphaV));
        Spectrum F = m_reflectance->Evaluate(matRec.m_st) *
            FresnelConductor(Frame::CosTheta(matRec.m_wi), m_eta, m_k);
        float G = microfacet.G(matRec.m_wi, matRec.m_wo, wh);
        float G1 = microfacet.G1(matRec.m_wi, wh);
        float D = microfacet.D(wh);

        float absCosTheta = Frame::AbsCosTheta(matRec.m_wi);
        Spectrum fs = F * D * G / (4 * absCosTheta);
        float pdf = D * G1 / (4 * absCosTheta);

        matRec.m_pdf = pdf;
        return fs;
    }
    bool IsDelta(const Float2& st) const { return BSDF::IsDelta(st); }
private:

    Spectrum m_eta, m_k;
    float m_alphaU, m_alphaV;
    std::shared_ptr<Texture<Spectrum>> m_reflectance;
};