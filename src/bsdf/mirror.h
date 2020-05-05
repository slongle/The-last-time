#pragma once

#include "core/primitive.h"
#include "bsdfutil.h"

class Mirror :public BSDF {
public:
    Mirror(
        const std::shared_ptr<Texture<Spectrum>>& reflectance,
        const std::shared_ptr<Texture<float>>& alpha)
        :BSDF(alpha), m_reflectance(reflectance) {}

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
        return m_reflectance->Evaluate(matRec.m_st);
    }
    Spectrum Eval(MaterialRecord& matRec) const {
        if (Frame::CosTheta(matRec.m_wi) <= 0 ||
            Frame::CosTheta(matRec.m_wo) <= 0 ||
            !Frame::SameDirection(matRec.m_wo, Reflect(matRec.m_wi))) {
            return Spectrum(0.f);
        }
        return m_reflectance->Evaluate(matRec.m_st);
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
        return m_reflectance->Evaluate(matRec.m_st);
    }
    bool IsDelta(const Float2& st) const { return true; }
private:
    Float3 Reflect(const Float3& v) const {
        return Float3(-v.x, -v.y, v.z);
    }

    std::shared_ptr<Texture<Spectrum>> m_reflectance;
};