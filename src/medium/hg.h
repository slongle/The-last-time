#pragma once

#include "core/primitive.h"

class HenyeyGreenstein : public PhaseFunction {
public:
    HenyeyGreenstein(float g) :m_g(g) {}

    Spectrum EvalPdf(PhaseFunctionRecord& phaseRec) const;
    Spectrum Sample(PhaseFunctionRecord& phaseRec, Float2& s) const;

    float m_g;
};