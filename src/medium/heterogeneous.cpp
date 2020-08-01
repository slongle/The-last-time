#include "heterogeneous.h"

HeterogeneousMedium::HeterogeneousMedium(
    const std::shared_ptr<PhaseFunction>& pf, 
    const std::string& filename,
    const bool& lefthand,
    const std::string& densityName,
    const bool& blackbody,
    const std::string& temperatureName,
    const Spectrum& albedo,
    const float& scale)
    : Medium(pf), m_lefthand(lefthand), m_blackbody(blackbody), m_albedo(albedo), m_scale(scale), 
    m_densitySampler({}), m_temperatureSampler({})
{
    openvdb::initialize();
    openvdb::io::File file(GetFileResolver()->string() + "/" + filename);
    std::cout << "Loading " << filename << std::endl;

    file.open();    
    openvdb::GridBase::Ptr densityBaseGrid;
    densityBaseGrid = file.readGrid(densityName);
    if (m_blackbody) {
        openvdb::GridBase::Ptr temperatureBaseGrid;
        temperatureBaseGrid = file.readGrid(densityName);
    }
    file.close();    

    m_densityGrid = openvdb::gridPtrCast<openvdb::FloatGrid>(densityBaseGrid);
    m_densitySampler = VDBFloatSampler(*m_densityGrid);
    if (m_blackbody) {
        m_temperatureGrid = openvdb::gridPtrCast<openvdb::FloatGrid>(densityBaseGrid);
        m_temperatureSampler = VDBFloatSampler(*m_densityGrid);
    }

    m_densityGrid->evalMinMax(m_minDensity, m_maxDensity);
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
        t -= std::log(1 - s) * m_invMaxDensity / m_scale;
        if (t >= d) {
            mediumRec.m_p = ray(d);
            mediumRec.m_t = d;
            mediumRec.m_pdf = pdf;
            mediumRec.m_internal = false;
            break;
        }
        float density = Density(ray(t));
        //Tr *= (1 - density * m_invMaxDensity);
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
    float Tr = 1;
    while (true) {        
        float s = sampler.Next1D();
        t -= std::log(1 - s) * m_invMaxDensity / m_scale;
        if (t >= d) break;
        float density = Density(ray(t));
        Tr *= (1 - density * m_invMaxDensity);

        const float rrThreshold = .1;
        if (Tr < rrThreshold) {
            float q = std::max((float).05, 1 - Tr);
            if (sampler.Next1D() < q) {
                return Spectrum(0.);
            }
            Tr /= 1 - q;
        }
    }
    return Tr;
}

float HeterogeneousMedium::Density(const Float3& _pWorld) const
{
    openvdb::Vec3d pWorld;
    if (m_lefthand) {
        pWorld = openvdb::Vec3d(_pWorld.x, _pWorld.z, -_pWorld.y);
    }
    else {        
        pWorld = openvdb::Vec3d(_pWorld.x, _pWorld.y, _pWorld.z);
    }
    float ret = m_densitySampler.wsSample(pWorld);
    return ret;
}

Spectrum HeterogeneousMedium::BlackbodyRadiance(const Float3& p) const
{
    return Spectrum(0.f);
}
