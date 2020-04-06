#pragma once

#include "core/primitive.h"

class EnvironmentLight{
public:
    EnvironmentLight(const std::shared_ptr<Texture<Spectrum>>& texture)
        :m_texture(texture) {}

    Spectrum Eval(const Ray& ray) const;

private:
    std::shared_ptr<Texture<Spectrum>> m_texture;
};