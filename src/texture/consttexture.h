#pragma once

#include "core/texture.h"

template<typename T>
class ConstTexture :public Texture<T> {
public:
    ConstTexture(const T& v) :m_value(v) {}
    T Evaluate(const Float2& uv) const { return m_value; }
private:
    T m_value;
};

template class ConstTexture<float>;
template class ConstTexture<Spectrum>;