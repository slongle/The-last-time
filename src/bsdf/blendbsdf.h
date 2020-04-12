#pragma once

#include "core/primitive.h"

class BlendBSDF :public BSDF {
public:
    BlendBSDF(
        const std::shared_ptr<Texture<float>>& weight,
        const std::shared_ptr<BSDF>& bsdf1,
        const std::shared_ptr<BSDF>& bsdf2)
        :m_weight(weight), m_bsdf{ bsdf1, bsdf2 } {}
    Spectrum Sample(MaterialRecord& matRec, Float2 s) const {
        float weight = m_weight->Evaluate(matRec.m_st);
        int slot;
        float pdf;
        if (s.x < weight) {
            slot = 0; 
            s.x /= weight;
            pdf = weight;
        }
        else {
            slot = 1;
            s.x = (s.x - weight) / (1 - weight);
            pdf = 1 - weight;
        }
        Spectrum f = m_bsdf[slot]->Sample(matRec, s);
        f /= pdf;
        matRec.m_pdf *= pdf;
        return f;
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

    bool IsDelta() const {
        return m_bsdf[0]->IsDelta() & m_bsdf[1]->IsDelta();
    }
private:
    std::shared_ptr<Texture<float>> m_weight;
    std::shared_ptr<BSDF> m_bsdf[2];
};