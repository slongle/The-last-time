#include "imageio.h"

// Stb_image
#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#endif // !STB_IMAGE_IMPLEMENTATION

#ifndef STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#endif // !STB_IMAGE_WRITE_IMPLEMENTATION

#include <OpenImageIO/imageio.h>
using namespace OIIO;


/**
 * @brief stb_image : index of left-lower is (0, 0)
 *  OIIO loads the image in linear space
 *  stb_image loads the image in sRGB
 * 
 * @param filename 
 * @param width 
 * @param height 
 * @return std::shared_ptr<float[]> 
 */
std::shared_ptr<float[]> ReadImage(
    const std::string& filename,
    int* width,
    int* height,
    int* channel,
    int reqChannel)
{
    
    stbi_set_flip_vertically_on_load(true);
    const std::string ext = GetFileExtension(filename);
    float* ptr = stbi_loadf(filename.c_str(), width, height, channel, reqChannel);
    LOG_IF(FATAL, !ptr) << "Can't load image : " << filename;
    //std::cout << ptr[0] << ' ' << ptr[1] << ' ' << ptr[2] << std::endl;
    return std::shared_ptr<float[]>(ptr);
    
    
    auto in = ImageInput::open(filename);
    if (!in) {
        LOG(FATAL) << "Can't load " << filename;
    }
    const ImageSpec& spec = in->spec();
    int xres = spec.width;
    int yres = spec.height;
    int channels = spec.nchannels;
    std::shared_ptr<float[]> pixels(new float[xres * yres * channels]);
    in->read_image(TypeDesc::FLOAT, &pixels[0]);
    in->close();
    *width = xres;
    *height = yres;
    *channel = channels;
    return pixels;
}

/**
 * @brief stb_image : index of left-lower is (0, 0)
 * 
 * @param filename 
 * @param width 
 * @param height 
 * @param buffer 
 */
void WriteImage(
    const std::string& filename,
    const int& width, 
    const int& height,
    float* buffer)
{        
    const std::string ext = GetFileExtension(filename);
    if (ext == "png")
    {
        uint8_t* sRGB = new uint8_t[width * height * 3];
        for (int i = 0; i < width * height * 3; i++) {
            sRGB[i] = (uint8_t)(Clamp(GammaCorrect(buffer[i]) * 255.f + 0.5f, 0.f, 255.f));            
        }
        stbi_flip_vertically_on_write(1);
        stbi_write_png(filename.c_str(), width, height, 3, sRGB, 0);
        delete[] sRGB;
    }
    else if (ext == "hdr")
    {
        stbi_flip_vertically_on_write(1);
        stbi_write_hdr(filename.c_str(), width, height, 3, buffer);
    }
    else
    {
        assert(false);
    }
}