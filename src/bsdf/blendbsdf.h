#pragma once

#include "core/primitive.h"

class BlendBSDF :public BSDF {
public:
    BlendBSDF(
        const std::shared_ptr<Texture<float>>& weight,
        const std::shared_ptr<BSDF>& bsdf1,
        const std::shared_ptr<BSDF>& bsdf2,
        const std::shared_ptr<Texture<float>>& alpha)
        :BSDF(alpha), m_weight(weight), m_bsdf{ bsdf1, bsdf2 } {}

    Spectrum Sample(MaterialRecord& matRec, Float2 s) const {
        // Alpha texture
        bool opaque = !(m_alpha->Evaluate(matRec.m_st) >= 0.99f);
        if (opaque) {
            matRec.m_wo = -matRec.m_wi;
            matRec.m_pdf = 1.f;
            return Spectrum(1.f);
        }

        float w = std::min(1.f, std::max(0.f, m_weight->Evaluate(matRec.m_st)));
        float weight[2] = { w,1 - w };
        int slot;
        if (s.x < weight[0]) {
            slot = 0;
            s.x /= weight[0];
        }
        else {
            slot = 1;
            s.x = (s.x - weight[0]) / (1 - weight[0]);
        }
        Spectrum fA = m_bsdf[slot]->Sample(matRec, s) * matRec.m_pdf;
        float pdfA = matRec.m_pdf;

        // Sample failed
        if (fA.IsBlack()) {
            return Spectrum(0.f);
        }

        Spectrum fB = m_bsdf[slot ^ 1]->EvalPdf(matRec);
        float pdfB = matRec.m_pdf;
        matRec.m_pdf = pdfA * weight[slot] + pdfB * weight[slot ^ 1];
        Spectrum f = fA * weight[slot] + fB * weight[slot ^ 1];
        return f / matRec.m_pdf;
    }

    Spectrum Eval(MaterialRecord& matRec) const {
        float weight = m_weight->Evaluate(matRec.m_st);
        return m_bsdf[0]->Eval(matRec) * weight +
            m_bsdf[1]->Eval(matRec) * (1 - weight);
    }

    float Pdf(MaterialRecord& matRec) const {
        float weight = m_weight->Evaluate(matRec.m_st);
        float pdf = m_bsdf[0]->Pdf(matRec) * weight +
            m_bsdf[1]->Pdf(matRec) * (1 - weight);
        matRec.m_pdf = pdf;
        return pdf;
    }

    Spectrum EvalPdf(MaterialRecord& matRec) const {
        float weight = m_weight->Evaluate(matRec.m_st);
        Spectrum f1 = m_bsdf[0]->EvalPdf(matRec);
        float pdf1 = matRec.m_pdf;
        Spectrum f2 = m_bsdf[1]->EvalPdf(matRec);
        float pdf2 = matRec.m_pdf;
        matRec.m_pdf = pdf1 * weight + pdf2 * (1 - weight);
        return f1 * weight + f2 * (1 - weight);
    }

    bool IsDelta(const Float2& st) const {
        return BSDF::IsDelta(st) ? true : m_bsdf[0]->IsDelta(st) & m_bsdf[1]->IsDelta(st);
    }
private:
    std::shared_ptr<Texture<float>> m_weight;
    std::shared_ptr<BSDF> m_bsdf[2];
};