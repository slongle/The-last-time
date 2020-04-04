#include "imageio.h"

// Stb_image
#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"
#endif // !STB_IMAGE_IMPLEMENTATION

#ifndef STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image/stb_image_write.h"
#endif // !STB_IMAGE_WRITE_IMPLEMENTATION


/**
 * @brief stb_image : index of left-upper is (0, 0)
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
    const std::string ext = GetFileExtension(filename);
    float* ptr = stbi_loadf(filename.c_str(), width, height, channel, 3);
    return std::shared_ptr<float[]>(ptr);
}

/**
 * @brief stb_image : index of left-upper is (0, 0)
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