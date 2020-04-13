#pragma once

#include "core/vector.h"
#include "core/spectrum.h"

template<typename T>
class Texture {    
public:
    Texture(const uint32_t& w, const uint32_t& h) :m_width(w), m_height(h) {}
    virtual ~Texture() {}
    virtual T Evaluate(const Float2& uv) const = 0;
public:
    uint32_t m_width, m_height;
};