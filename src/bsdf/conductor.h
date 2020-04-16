#pragma once

#include "core/primitive.h"
#include "bsdfutil.h"

class SmoothConductor :public BSDF {
public:
    SmoothConductor(
        const std::shared_ptr<Texture<Spectrum>>& reflectance,
        const Spectrum& eta, const Spectrum& k) 
        :m_reflectance(reflectance), m_eta(eta), m_k(k) {
        std::cout << eta << std::endl;
        std::cout << k << std::endl;
    }    

    Spectrum Sample(MaterialRecord& matRec, Float2 s) const {
        if (Frame::CosTheta(matRec.m_wi) <= 0) {            
            return Spectrum(0.f);
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
    bool IsDelta() const { return true; }
private:
    Float3 Reflect(const Float3& v) const {
        return Float3(-v.x, -v.y, v.z);
    }

    Spectrum m_eta, m_k;
    std::shared_ptr<Texture<Spectrum>> m_reflectance;
};