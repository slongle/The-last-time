#pragma once

#include "core/primitive.h"

class AreaLight :public Light {
public:
    AreaLight(const Spectrum& radiance, const std::shared_ptr<Shape> shape)
        :m_radiance(radiance), m_shape(shape) {}

    Spectrum Sample(LightRecord& lightRec, Float2& s) const;
    Spectrum Eval(LightRecord& lightRec) const;
    float Pdf(LightRecord& lightRec) const;
    Spectrum EvalPdf(LightRecord& lightRec) const;

    Spectrum m_radiance;
    std::shared_ptr<Shape> m_shape;
};