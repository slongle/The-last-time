#pragma once

#include "core/texture.h"

template<typename T>
class ImageTexture : public Texture<T> {
public:
    ImageTexture(const int& w, const int& h, const std::shared_ptr<T[]>& ptr)
        :Texture<T>(w, h), m_image(ptr) {}
    T Evaluate(const Float2& uv) const;
private:
    std::shared_ptr<T[]> m_image;
};

extern template class ImageTexture<float>;
extern template class ImageTexture<Spectrum>;

std::shared_ptr<ImageTexture<float>> 
CreateFloatImageTexture(std::string filename);

std::shared_ptr<ImageTexture<Spectrum>> 
CreateSpectrumImageTexture(std::string filename);