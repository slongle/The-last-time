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

    void Initialize();
    void AddSample(int x, int y, const Spectrum& s);
    void Save(const std::string& prefix = "");
    sRGB* GetsRGBBuffer() const;
    sRGB GetPixelSpectrum(const Int2& pos) const;
    // Debug
    void SetVal(int x, int y, const Spectrum& s);
    void DrawCircle(float cx, float cy, float r, const Spectrum& s);
    void DrawLine(Float2 p, Float2 q, const Spectrum& s);
    void ClearDebugBuffer();
public:
    int m_width, m_height;
private:
    std::string m_filename;
    sRGB *m_outputBuffer;
    sRGB *m_debugBuffer;
    sRGB *m_image;
    Spectrum *m_accumulate;
    unsigned int *m_sampleNum;
};