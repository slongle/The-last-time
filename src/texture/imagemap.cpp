#include "imagemap.h"

#include "utility/imageio.h"

template<typename T>
inline T ImageTexture<T>::Evaluate(const Float2& uv) const
{
    Float2 st = uv * Float2(m_width, m_height);
    Int2 pos((int(st.x) + m_width) % m_width, (int(st.y) + m_height) % m_height);
    return m_image[pos.y * m_width + pos.x];
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

std::shared_ptr<ImageTexture<Spectrum>> CreateSpectrumImageTexture(std::string filename)
{
    std::cout << "Loading " << filename << std::endl;
    filename = GetFileResolver()->string() + "/" + filename;

    int width, height, channel;
    std::shared_ptr<float[]> image = ReadImage(filename, &width, &height, &channel, 3);

    std::shared_ptr<Spectrum[]> spectrumImage(new Spectrum[width * height]);
    for (int i = 0; i < width * height; i++) {
        spectrumImage[i] = Spectrum(image[i * 3 + 0], image[i * 3 + 1], image[i * 3 + 2]);
    }
    return std::make_shared<ImageTexture<Spectrum>>(width, height, spectrumImage);
}
