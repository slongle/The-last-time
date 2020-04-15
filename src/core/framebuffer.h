#pragma once

#include <string>

#include "spectrum.h"

const int tile_size = 4;

// For GLFW, the bottom-left corner is (0, 0)

class Framebuffer
{
public:    

    struct Tile {
        int pos[2];
        int res[2];
    };

    Framebuffer() {}
    Framebuffer(const std::string& filename, const int& width, const int& height);        
    ~Framebuffer();

    void AddSample(int x, int y, const Spectrum& s);
    void SetVal(int x, int y, const Spectrum& s);    
    void Save();
    sRGB* GetsRGBBuffer() const;

    int m_width, m_height;
private:
    std::string m_filename;
    sRGB *m_sRGB;
    Spectrum *m_accumulate;
    unsigned int *m_sampleNum;
};