#include "heterogeneous.h"

HeterogeneousMedium::HeterogeneousMedium(
    const std::shared_ptr<PhaseFunction>& pf, 
    const std::string& filename,
    const std::string& densityName,
    const Spectrum& albedo,
    const float& scale)
    : Medium(pf), m_albedo(albedo), m_scale(scale), m_sampler({})
{
    openvdb::initialize();
    openvdb::io::File file(GetFileResolver()->string() + "/volumes/" + filename);
    file.open();    

    openvdb::GridBase::Ptr baseGrid;   
    for (openvdb::io::File::NameIterator nameIter = file.beginName();
        nameIter != file.endName(); ++nameIter)
    {
        if (nameIter.gridName() == densityName) {
            baseGrid = file.readGrid(nameIter.gridName());            
            break;
        }
    }

    file.close();
    LOG_IF(FATAL, !baseGrid) << "Miss the volume named " << densityName;

    m_grid = openvdb::gridPtrCast<openvdb::FloatGrid>(baseGrid);
    m_sampler = VDBFloatSampler(*m_grid);    

    m_grid->evalMinMax(m_minDensity, m_maxDensity);
    m_invMaxDensity = m_maxDensity == 0 ? 0 : 1.f / m_maxDensity;
    //std::cout << m_minVal << ' ' << m_maxVal << std::endl;
}

Spectrum HeterogeneousMedium::Sample(const Ray& ray, MediumRecord& mediumRec, Sampler& sampler) const
{
    float d = ray.tMax;
    LOG_IF(FATAL, d == std::numeric_limits<float>::infinity()) << "The estimated ray is infinity.";
    float t = 0;
    float pdf = 1;
    float Tr = 1;

    mediumRec.m_internal = true;
    while (true) {
        float s = sampler.Next1D();
        t -= std::log(1 - s) * m_invMaxDensity;
        if (t >= d) {
            mediumRec.m_p = ray(d);
            mediumRec.m_t = d;
            mediumRec.m_pdf = pdf;
            mediumRec.m_internal = false;
            break;
        }
        float density = Density(ray(t));
        Tr *= (1 - density * m_invMaxDensity);
        pdf *= (1 - density * m_invMaxDensity);
        if (sampler.Next1D() <= density * m_invMaxDensity) {
            mediumRec.m_p = ray(t);
            mediumRec.m_t = t;
            mediumRec.m_pdf = pdf;
            mediumRec.m_internal = true;
            return m_albedo;
        }
    }
    return Spectrum(1.f);
}

Spectrum HeterogeneousMedium::Transmittance(const Ray& ray, Sampler& sampler) const
{
    float d = ray.tMax;
    LOG_IF(FATAL, d == std::numeric_limits<float>::infinity()) << "The estimated ray is infinity.";
    float t = 0;
    Spectrum Tr = 1;
    while (true) {
        float s = sampler.Next1D();
        t -= std::log(1 - s) * m_invMaxDensity;
        if (t >= d) break;
        float density = Density(ray(t));
        Tr *= (1 - density * m_invMaxDensity);
    }
    return Tr;
}

float HeterogeneousMedium::Density(const Float3& _pWorld) const
{
    auto pWorld = openvdb::Vec3d(_pWorld.x, _pWorld.z, -_pWorld.y);
    float ret = m_sampler.wsSample(pWorld);
    return ret;
}
