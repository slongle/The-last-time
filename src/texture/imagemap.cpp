#include "imagemap.h"

#include "utility/imageio.h"

template<typename T>
inline T ImageTexture<T>::Evaluate(const Float2& uv) const
{
    Float2 st = uv * m_resolution;
    Float2 p00 = Mod(Mod(st + Float2(0, 0), m_resolution) + m_resolution, m_resolution);
    Float2 p01 = Mod(Mod(st + Float2(0, 1), m_resolution) + m_resolution, m_resolution);
    Float2 p10 = Mod(Mod(st + Float2(1, 0), m_resolution) + m_resolution, m_resolution);
    Float2 p11 = Mod(Mod(st + Float2(1, 1), m_resolution) + m_resolution, m_resolution);
    Float2 dxdy = Mod(st, Float2(1, 1));
    T c00 = LookUp(p00);
    T c01 = LookUp(p01);
    T c10 = LookUp(p10);
    T c11 = LookUp(p11);
    T c0 = Lerp(c00, c10, dxdy.x);
    T c1 = Lerp(c01, c11, dxdy.x);
    T c = Lerp(c0, c1, dxdy.y);
    return c;
    //return m_image[int(p00.y) * m_resolution.x + int(p00.x)];
}

template<typename T>
T ImageTexture<T>::LookUp(const Float2& pos) const
{
    return m_image[int(pos.y) * m_resolution.x + int(pos.x)];
}

template class ImageTexture<float>;
template class ImageTexture<Spectrum>;

std::shared_ptr<ImageTexture<float>> CreateFloatImageTexture(std::string filename)
{
    std::cout << "Loading " << filename << std::endl;
    filename = GetFileResolver()->string() + "/" + filename;

    int width, height, channel;
    std::shared_ptr<float[]> image = ReadImage(filename, &width, &height, &channel, 1);    

    return std::make_shared<ImageTexture<float>>(width, height, image);
}

std::shared_ptr<ImageTexture<float>> CreateFloatImageTexture(std::string filename, uint32_t slot)
{
    std::cout << "Loading " << filename << std::endl;
    filename = GetFileResolver()->string() + "/" + filename;

    int width, height, channel;
    std::shared_ptr<float[]> _image = ReadImage(filename, &width, &height, &channel, 3);

    std::shared_ptr<float[]> image(new float[width * height]);
    for (int i = 0; i < width * height; i ++) {
        image[i] = _image[i * 3 + slot];
    }

    return std::make_shared<ImageTexture<float>>(width, height, image);
}

std::shared_ptr<ImageTexture<Spectrum>> CreateSpectrumImageTexture(std::string filename)
{
    std::cout << "Loading " << filename << std::endl;
    filename = GetFileResolver()->string() + "/" + filename;

    int width, height, channel;
    std::shared_ptr<float[]> image = ReadImage(filename, &width, &height, &channel, 3);

    std::string ext = GetFileExtension(filename);
    std::shared_ptr<Spectrum[]> spectrumImage(new Spectrum[width * height]);    
    for (int i = 0; i < width * height; i++) {
        spectrumImage[i] = Spectrum(
            (image[i * 3 + 0]),
            (image[i * 3 + 1]),
            (image[i * 3 + 2]));
    }
    return std::make_shared<ImageTexture<Spectrum>>(width, height, spectrumImage);
}
