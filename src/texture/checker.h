#pragma once

#pragma once

#include "core/texture.h"

class CheckerTexture :public Texture<Spectrum> {
public:
    CheckerTexture(const Spectrum& v0, const Spectrum& v1, const Float2& scale) 
        :Texture<Spectrum>(2, 2), m_color0(v0), m_color1(v1), m_scale(scale) {}
    Spectrum Evaluate(const Float2& uv) const { 
        Float2 st = Mod(Mod(uv * m_resolution * m_scale, m_resolution) + m_resolution, m_resolution);
        bool b0 = st.x < 1, b1 = st.y < 1;
        return b0 ^ b1 ? m_color1 : m_color0;
    }
private:
    Spectrum m_color0, m_color1;
    Float2 m_scale;
};
