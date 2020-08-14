#pragma once

#include "core/sampler.h"

#include "pcg32/pcg32.h"

class IndependentSampler : public Sampler {
public:
    IndependentSampler() { }

    void Setup(uint64_t s) {
        m_rng.seed(s);
    }

    float Next1D() {
        return m_rng.nextFloat();
    }
    Float2 Next2D() {
        return Float2(Next1D(), Next1D());
    }

    pcg32 m_rng;
};