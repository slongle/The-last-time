#include "integrator.h"

void Integrator::DrawLine(const Float3& p, const Float3& q, const Spectrum& c)
{
    Float2 pScreen(Inverse(m_camera->m_screenToWorld).TransformPoint(p));
    Float2 qScreen(Inverse(m_camera->m_screenToWorld).TransformPoint(q));
    m_buffer->DrawLine(pScreen, qScreen, c);
}

void Integrator::DrawBounds(const Bounds& bounds, const Spectrum& c)
{
    float xl = bounds.m_pMin.x, xr = bounds.m_pMax.x;
    float yl = bounds.m_pMin.y, yr = bounds.m_pMax.y;
    float zl = bounds.m_pMin.z, zr = bounds.m_pMax.z;
    DrawLine(Float3(xl, yl, zl), Float3(xr, yl, zl), c);
    DrawLine(Float3(xl, yr, zl), Float3(xr, yr, zl), c);
    DrawLine(Float3(xl, yl, zl), Float3(xl, yr, zl), c);
    DrawLine(Float3(xr, yl, zl), Float3(xr, yr, zl), c);

    DrawLine(Float3(xl, yl, zr), Float3(xr, yl, zr), c);
    DrawLine(Float3(xl, yr, zr), Float3(xr, yr, zr), c);
    DrawLine(Float3(xl, yl, zr), Float3(xl, yr, zr), c);
    DrawLine(Float3(xr, yl, zr), Float3(xr, yr, zr), c);

    DrawLine(Float3(xl, yl, zl), Float3(xl, yl, zr), c);
    DrawLine(Float3(xl, yr, zl), Float3(xl, yr, zr), c);
    DrawLine(Float3(xr, yl, zl), Float3(xr, yl, zr), c);
    DrawLine(Float3(xr, yr, zl), Float3(xr, yr, zr), c);
}
