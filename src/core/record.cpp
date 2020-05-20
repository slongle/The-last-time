#include "record.h"
#include "core/primitive.h"

std::shared_ptr<Medium> HitRecord::GetMedium(const Float3& d) const
{
    return Dot(m_geoRec.m_ns, d) > 0 ?
        m_primitive->m_mediumInterface.m_outsideMedium :
        m_primitive->m_mediumInterface.m_insideMedium;
}

std::shared_ptr<BSDF> HitRecord::GetBSDF() const
{
    return m_primitive->m_bsdf;
}
