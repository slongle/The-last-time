#include "integrator.h"

void Integrator::DrawLine(const Float3& p, const Float3& q, const Spectrum& c)
{
    Float2 pScreen(Inverse(m_camera->m_screenToWorld).TransformPoint(p));
    Float2 qScreen(Inverse(m_camera->m_screenToWorld).TransformPoint(q));
    m_buffer->DrawLine(pScreen, qScreen, c);
}
