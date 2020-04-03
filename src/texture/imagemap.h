#pragma once

#include "core/texture.h"

template<typename T>
class ImageTexture : public Texture<T> {
public:
    ImageTexture(const std::shared_ptr<T[]>& ) {}
};

