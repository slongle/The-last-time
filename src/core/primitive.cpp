#include "primitive.h"

Spectrum BSDF::Sample(MaterialRecord& matRec, Float2& s) const
{
    matRec.m_wo = SampleCosineHemisphere(s);
    matRec.m_pdf = PdfCosineHemisphere(matRec.m_wo);
    return Eval(matRec) / matRec.m_pdf;
}

float BSDF::Pdf(MaterialRecord& matRec) const
{
    return matRec.m_pdf = PdfCosineHemisphere(matRec.m_wo);
}
