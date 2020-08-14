#pragma once

#include "global.h"
#include "vector.h"

#include <random>

class Sampler {
public:
    Sampler() {}

    virtual void Setup(uint64_t s) {
        m_engine.seed(s);        
    }

    virtual float Next1D() {
        return m_rng(m_engine);
    }
    virtual Float2 Next2D() {
        return Float2(Next1D(), Next1D());
    }

private:
    std::default_random_engine m_engine;
    std::uniform_real_distribution<float> m_rng;
};