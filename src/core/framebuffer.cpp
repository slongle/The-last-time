#include "framebuffer.h"

#include "utility/imageio.h"

Framebuffer::Framebuffer(const std::string& filename, const int& width, const int& height)
    : m_filename(filename), m_width(width), m_height(height)
{
    m_name = GetFilename(m_filename);
    m_ext = GetFileExtension(m_filename);
    m_outputBuffer = new sRGB[m_width * m_height];
    m_debugBuffer = new sRGB[m_width * m_height];
    m_image = new sRGB[m_width * m_height];
    m_accumulate = new Spectrum[m_width * m_height];
    m_sampleNum = new unsigned int[m_width * m_height];
    Initialize();
}

Framebuffer::~Framebuffer()
{
    delete[] m_outputBuffer;
    delete[] m_debugBuffer;
    delete[] m_image;
    delete[] m_accumulate;
    delete[] m_sampleNum;
}

void Framebuffer::Initialize()
{
    for (int i = 0; i < m_height; i++) {
        for (int j = 0; j < m_width; j++) {
            int idx = i * m_width + j;
            if (((i / tile_size) ^ (j / tile_size)) & 1) {
                m_image[idx] = sRGB(0.3f);
            }
            else {
                m_image[idx] = sRGB(0.7f);
            }
            m_accumulate[idx] = Spectrum(0.f);
            m_sampleNum[idx] = 0;
        }
    }
}

void Framebuffer::AddSample(int x, int y, const Spectrum& s)
{
    int idx = y * m_width + x;
    m_sampleNum[idx] ++;
    m_accumulate[idx] += s;
    m_image[idx] = (m_accumulate[idx] / float(m_sampleNum[idx])).TosRGB();
}

void Framebuffer::SetVal(int x, int y, const Spectrum& s)
{
    if (x < 0 || x >= m_width || y < 0 || y >= m_height) {
        return;
    }

    int idx = y * m_width + x;
    //m_sampleNum[idx] = 1;
    //m_accumulate[idx] = s;
    //m_sRGB[idx] = (m_accumulate[idx] / float(m_sampleNum[idx])).TosRGB();
    m_debugBuffer[idx] = s.TosRGB();
}

void Framebuffer::DrawCircle(float cx, float cy, float r, const Spectrum& s)
{
    for (int x = std::ceil(cx - r); x <= std::floor(cx + r); x++) {
        for (int y = std::ceil(cy - r); y <= std::floor(cy + r); y++) {
            if (x < 0 || x >= m_width || y < 0 || y >= m_height) {
                continue;
            }
            if ((x - cx) * (x - cx) + (y - cy) * (y - cy) <= r * r) {
                SetVal(x, y, s);
            }
        }
    }
}

void Framebuffer::DrawLine(Float2 p, Float2 q, const Spectrum& s)
{
    int x0 = p.x, y0 = p.y;
    int x1 = q.x, y1 = q.y;
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = (dx > dy ? dx : -dy) / 2;

    while (SetVal(x0, y0, s), x0 != x1 || y0 != y1) {
        int e2 = err;
        if (e2 > -dx) { err -= dy; x0 += sx; }
        if (e2 < dy) { err += dx; y0 += sy; }
    }
}

void Framebuffer::Save(const std::string& suffix)
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

    std::string filename = suffix == "" ?
        fmt::format("{0}/{1}.{2}", GetFileResolver()->string(), m_name, m_ext) :
        fmt::format("{0}/{1}_{2}.{3}", GetFileResolver()->string(), m_name, suffix, m_ext);

    WriteImage(filename, m_width, m_height, linearRGB);
    delete[] linearRGB;

    std::cout << "Save image in " + filename << std::endl;
}

sRGB* Framebuffer::GetsRGBBuffer() const
{
    //return m_sRGB;
    for (int i = 0; i < m_width * m_height; i++) {
        m_outputBuffer[i] = m_debugBuffer[i].IsBlack() ? m_image[i] : m_debugBuffer[i];
    }
    return m_outputBuffer;
}

sRGB Framebuffer::GetPixelSpectrum(const Int2& pos) const
{
    if (0 <= pos.x && pos.x < m_width && 0 <= pos.y && pos.y < m_height) {
        int idx = pos.y * m_width + pos.x;
        return m_image[idx];
    }
    else {
        return sRGB();
    }
}

void Framebuffer::ClearDebugBuffer()
{
    memset(m_debugBuffer, 0, sizeof(sRGB) * m_width * m_height);
}
