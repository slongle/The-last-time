#pragma once

#include "core/primitive.h"

class Matte : public BSDF{
public:
    Matte(const Spectrum& reflectance) :m_reflectance(reflectance) {}

    Spectrum Eval(MaterialRecord& matRec) const {
        return m_reflectance * INV_PI * Frame::AbsCosTheta(matRec.m_wo);
    }

    Spectrum EvalPdf(MaterialRecord& matRec) const {
        matRec.m_pdf = PdfCosineHemisphere(matRec.m_wo);
        return m_reflectance * INV_PI * Frame::AbsCosTheta(matRec.m_wo);
    }

    Spectrum m_reflectance;
};