#pragma once

#include "core/primitive.h"
#include "bsdfutil.h"
#include "microfacet.h"

class SVBRDF :public BSDF {
public:
    SVBRDF(
        const std::shared_ptr<Texture<Spectrum>>& diffuse,
        const std::shared_ptr<Texture<Spectrum>>& specular,
        const std::shared_ptr<Texture<Spectrum>>& normal,
        const std::shared_ptr<Texture<float>>& roughness,
        const std::shared_ptr<Texture<float>>& alpha)
        :BSDF(alpha), m_diffuse(diffuse), m_specular(specular),
        m_normal(normal), m_roughness(roughness) {}

    Spectrum Sample(MaterialRecord& matRec, Float2 s) const {
        LOG(FATAL) << "No implementation.";
    }
    Spectrum Eval(MaterialRecord& matRec) const {
        if (Frame::CosTheta(matRec.m_wi) <= 0 ||
            Frame::CosTheta(matRec.m_wo) <= 0 ||
            !Frame::SameHemisphere(matRec.m_wi, matRec.m_wo)) {
            return Spectrum(0.f);
        }

        auto InvGamma = [](const Spectrum& s)->Spectrum {
            return Spectrum(GammaCorrect(s.r), GammaCorrect(s.g), GammaCorrect(s.b));
        };

        Spectrum n = ((m_normal->Evaluate(matRec.m_st)));
        Spectrum diffuse = m_diffuse->Evaluate(matRec.m_st);
        Spectrum specular = m_specular->Evaluate(matRec.m_st);
        float roughness = m_roughness->Evaluate(matRec.m_st);
        Float3 microNormal(n.r, n.g, n.b);        
        //return microNormal;
        microNormal = Normalize(microNormal - Float3(0.5));
        //return microNormal;

        auto GGX = [](float NoH, float roughness)->float {
            float alpha = roughness * roughness;
            float tmp = alpha / std::max(1e-8, (NoH * NoH * (alpha * alpha - 1.0) + 1.0));
            //float tmp = 1 / (alpha * NoH * NoH * );
            return tmp * tmp * INV_PI;
        };

        auto SmithG = [](float NoV, float NoL, float roughness) ->float {
            float k = std::max(1e-8, roughness * roughness * 0.5);
            float G1L = NoL / (NoL * (1.0 - k) + k);
            float G1V = NoV / (NoV * (1.0 - k) + k);
            return G1L * G1V;
        };

        auto Fresnel = [](Spectrum F0, float VoH) ->Spectrum {
            float factor = std::sqr(std::sqr(1 - VoH)) * (1 - VoH);
            return F0 + (Spectrum(1.f) - F0) * factor;
        };
               
        Float3 half = Normalize((matRec.m_wi + matRec.m_wo));
        float NoH = Dot(microNormal, half);
        float NoV = Dot(microNormal, matRec.m_wi);
        float NoL = Dot(microNormal, matRec.m_wo);
        float VoH = Dot(half, matRec.m_wo);

        if (NoV <= 0) {
            return Spectrum(0, 0, 0.0);
        }
        if (NoL <= 0) {
            return Spectrum(0.0, 0, 0);
        }

        Spectrum F = Fresnel(specular, VoH);
        float D = GGX(NoH, roughness);
        float G = SmithG(NoV, NoL, roughness);
        return (diffuse * INV_PI * NoL) + (F * D * G) / (4 * NoV);
        //return diffuse * INV_PI * NoL;
        //return (F * D * G) / (4 * NoV * NoL + 1e-12) * NoL;
    }
    float Pdf(MaterialRecord& matRec) const {
        LOG(FATAL) << "No implementation.";
    }
    Spectrum EvalPdf(MaterialRecord& matRec) const {
        LOG(FATAL) << "No implementation.";
    }
    bool IsDelta(const Float2& st) const { return false; }
private:
    std::shared_ptr<Texture<Spectrum>> m_diffuse;
    std::shared_ptr<Texture<Spectrum>> m_specular;
    std::shared_ptr<Texture<Spectrum>> m_normal;
    std::shared_ptr<Texture<float>> m_roughness;
};