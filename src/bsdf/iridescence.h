#pragma once

#include "core/primitive.h"
#include "bsdfutil.h"
#include "microfacet.h"

class IridescenceConductor :public BSDF {
public:
    IridescenceConductor(
        float eta_1, float eta_2, float eta_3, float k_3, float alpha, float Dinc,
        const Spectrum& reflectance,
        const std::shared_ptr<Texture<float>>& alphaTexture
    ) :BSDF(alphaTexture), m_eta_1(eta_1), m_eta_2(eta_2),
        m_eta_3(eta_3), m_k_3(k_3), m_alpha(alpha), m_Dinc(Dinc), m_reflectance(reflectance) {}

    // Return f * |cos| / pdf
    Spectrum Sample(MaterialRecord& matRec, Float2 s) const override {
        if (Frame::CosTheta(matRec.m_wi) <= 0) {
            return Spectrum(0.f);
        }

        Microfacet microfacet(m_alpha);
        Float3 wh = microfacet.SampleVisible(matRec.m_wi, s);
        float D = microfacet.D(wh);

        matRec.m_wo = Reflect(matRec.m_wi, wh);
        if (Frame::CosTheta(matRec.m_wo) <= 0) {
            return Spectrum(0.f);
        }

        Spectrum R = IridescenceTerm(Dot(wh, matRec.m_wi));

        Spectrum weight = R * microfacet.G1(matRec.m_wo, wh) * m_reflectance;
        float pdf = microfacet.G1(matRec.m_wi, wh) * D / (4 * Frame::CosTheta(matRec.m_wi));

        matRec.m_pdf = pdf;
        matRec.m_eta = 1.f;
        return weight;
    }
    // Return f * |cos|
    Spectrum Eval(MaterialRecord& matRec) const override {
        if (Frame::CosTheta(matRec.m_wi) <= 0 ||
            Frame::CosTheta(matRec.m_wo) <= 0) {
            return Spectrum(0.f);
        }

        // Microfacet model part
        Float3 wh = Normalize((matRec.m_wi + matRec.m_wo));
        Microfacet microfacet(m_alpha);
        float D = microfacet.D(wh);
        float G = microfacet.G(matRec.m_wi, matRec.m_wo, wh);
        Spectrum model = D * G / (4.0f * Frame::CosTheta(matRec.m_wi)) * m_reflectance;

        // Iridescence fresnel part
        Spectrum R = IridescenceTerm(Dot(wh, matRec.m_wi));

        return model * R;
    }
    // Return pdf
    float Pdf(MaterialRecord& matRec) const override {
        if (Frame::CosTheta(matRec.m_wi) <= 0 ||
            Frame::CosTheta(matRec.m_wo) <= 0) {
            return matRec.m_pdf = 0.f;
        }

        Float3 wh = Normalize((matRec.m_wi + matRec.m_wo));
        Microfacet microfacet(m_alpha);
        float pdf = microfacet.D(wh) * microfacet.G1(matRec.m_wi, wh) / (4 * Frame::CosTheta(matRec.m_wi));

        return matRec.m_pdf = pdf;
    }
    // Return f * |cos|
    Spectrum EvalPdf(MaterialRecord& matRec) const override {
        Spectrum ret = Eval(matRec);
        Pdf(matRec);
        return ret;
    }

    bool IsDelta(const Float2& st) const override {
        return false;
    }

    /**
     * @brief 
     * @param OPD 
     * @param phiShift 
     * @return color in XYZ colorspace
    */
    Spectrum EvalSensitivity(float OPD, float phiShift) const {
        // Use Gaussian fits, given by 3 parameters: val, pos and var
        float phase = 2 * M_PI * OPD * 1.0e-9;
        Spectrum val(5.4856e-13, 4.4201e-13, 5.2481e-13);
        Spectrum pos(1.6810e+06, 1.7953e+06, 2.2084e+06);
        Spectrum var(4.3278e+09, 9.3046e+09, 6.6121e+09);
        Spectrum xyz = val * Sqrt(2 * M_PI * var) * Cos(pos * phase + phiShift) * Exp(-var * phase * phase);
        xyz.r += 9.7470e-14 * math::SafeSqrt(2 * M_PI * 4.5282e+09) * std::cos(2.2399e+06 * phase + phiShift) * 
            std::exp(-4.5282e+09 * phase * phase);
        return xyz / 1.0685e-7;
    }

    Spectrum IridescenceTerm(float cosTheta1) const {
        // First interface        
        float cosTheta2 = math::SafeSqrt(1.f - math::Sqr(1.0 / m_eta_2) * (1 - math::Sqr(cosTheta1)));
        Float2 R12, phi12;
        FresnelDieletricExt(cosTheta1, m_eta_1, m_eta_2, R12, phi12);
        Float2 R21 = R12;
        Float2 phi21 = Float2(M_PI) - phi12;
        Float2 T121 = Float2(1.f) - R12;
        // Second interface
        Float2 R23, phi23;
        FresnelConductorExt(cosTheta2, m_eta_2, m_eta_3, m_k_3, R23, phi23);

        float OPD = m_Dinc * cosTheta2;
        Float2 phi2 = phi21 + phi23;

        Spectrum R;
        Float2 R123 = R12 * R23;
        Float2 r123 = Sqrt(R123);
        Float2 Rs = Sqr(T121) * R23 / (Float2(1.f) - R123);

        // Reflectance term for m=0 (DC term amplitude)
        Float2 C0 = R12 + Rs;
        Spectrum S0 = EvalSensitivity(0.0, 0.0);
        R += (C0.x + C0.y) * 0.5f * S0;

        // Reflectance term for m>0 (pairs of diracs)
        Float2 Cm = Rs - T121;
        for (int m = 1; m <= 3; ++m) {
            Cm *= r123;
            Spectrum SmS = 2.0 * EvalSensitivity(m * OPD, m * phi2.x);
            Spectrum SmP = 2.0 * EvalSensitivity(m * OPD, m * phi2.y);
            R += (Cm.x * SmS + Cm.y * SmP) * 0.5f;
        }

        // Convert back to RGB reflectance
        //R = Clamp(XYZToRGB(R), Spectrum(0.f), Spectrum(1.f));
        R = ClampNegative(XYZToRGB(R));
        return R;
    }

    float m_eta_1;
    float m_eta_2;
    float m_eta_3;
    float m_k_3;
    float m_alpha;
    float m_Dinc;
    Spectrum m_reflectance;
};