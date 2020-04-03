#pragma once

#include "global.h"
#include "vector.h"

#include <random>

class Sampler {
public:
    Sampler() {}

    void Setup(unsigned int s) {
        m_engine.seed(s);
    }

    float Next1D() {
        return m_rng(m_engine);
    }
    Float2 Next2D() {
        return Float2(Next1D(), Next1D());
    }

    std::default_random_engine m_engine;
    std::uniform_real_distribution<float> m_rng;
};