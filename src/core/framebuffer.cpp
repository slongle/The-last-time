#include "framebuffer.h"

#include "utility/imageio.h"

Framebuffer::Framebuffer(const std::string& filename, const int& width, const int& height)
    : m_filename(filename), m_width(width), m_height(height)
{
    m_sRGB = new sRGB[m_width * m_height];
    m_accumulate = new Spectrum[m_width * m_height];
    m_sampleNum = new unsigned int[m_width * m_height];
    for (int i = 0; i < m_height; i++) {
        for (int j = 0; j < m_width; j++) {
            int idx = i * m_width + j;
            if (((i / tile_size) ^ (j / tile_size)) & 1) {
                m_sRGB[idx] = sRGB(0.3f);
            }
            else {
                m_sRGB[idx] = sRGB(0.7f);
            }
            m_sampleNum[idx] = 0;
        }
    }
}

Framebuffer::~Framebuffer()
{
    delete[] m_sRGB;
    delete[] m_accumulate;
    delete[] m_sampleNum;
}

void Framebuffer::AddSample(int x, int y, const Spectrum& s)
{
    int idx = y * m_width + x;
    m_sampleNum[idx] ++;
    m_accumulate[idx] += s;
    m_sRGB[idx] = (m_accumulate[idx] / float(m_sampleNum[idx])).TosRGB();
}

void Framebuffer::SetVal(int x, int y, const Spectrum& s)
{
    if (x < 0 || x >= m_width || y < 0 || y >= m_height) {
        return;
    }

    int idx = y * m_width + x;
    m_sampleNum[idx] = 1;
    m_accumulate[idx] = s;
    m_sRGB[idx] = (m_accumulate[idx] / float(m_sampleNum[idx])).TosRGB();
}

void Framebuffer::Save()
{
    float* linearRGB = new float[m_width * m_height * 3];
    for (int i = 0; i < m_width * m_height; i++) {
        if (m_sampleNum[i] == 0) {
            m_sampleNum[i] = 1;
        }
        Spectrum c = (m_accumulate[i] / float(m_sampleNum[i]));
        linearRGB[i * 3 + 0] = c.r;
        linearRGB[i * 3 + 1] = c.g;
        linearRGB[i * 3 + 2] = c.b;
    }
    WriteImage(m_filename, m_width, m_height, linearRGB);
    delete[] linearRGB;

    std::cout << "Save image in " + m_filename << std::endl;
}

sRGB* Framebuffer::GetsRGBBuffer() const
{
    return m_sRGB;
}
