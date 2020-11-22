#include "heterogeneous.h"

HeterogeneousMedium::HeterogeneousMedium(
    const std::shared_ptr<PhaseFunction>& pf, 
    const std::string& filename,
    const bool& lefthand,
    const std::string& densityName,
    const Spectrum& albedo,
    const float& scale)
    : Medium(pf), m_lefthand(lefthand), m_albedo(albedo), m_scale(scale), 
      m_densitySampler({})
{
    openvdb::initialize();
    openvdb::io::File file(GetFileResolver()->string() + "/" + filename);
    std::cout << "Loading " << filename << std::endl;

    file.open();    
    openvdb::GridBase::Ptr densityBaseGrid;
    densityBaseGrid = file.readGrid(densityName);
    file.close();

    m_densityGrid = openvdb::gridPtrCast<openvdb::FloatGrid>(densityBaseGrid);
    m_densitySampler = VDBFloatSampler(*m_densityGrid);

    m_densityGrid->evalMinMax(m_minDensity, m_maxDensity);
    m_minDensity = 0.f;
    m_invMaxDensity = m_maxDensity == 0 ? 0 : 1.f / m_maxDensity;
    
    //std::cout << m_minDensity << ' ' << m_maxDensity << std::endl;
}

Spectrum HeterogeneousMedium::Sample(const Ray& ray, MediumRecord& mediumRec, Sampler& sampler) const
{
    float d = ray.tMax;
    LOG_IF(FATAL, d == std::numeric_limits<float>::infinity()) << "The estimated ray is infinity.";
    float t = 0;
    double pdf = 1;
    double Tr = 1;

    mediumRec.m_internal = true;
    float s;
    while (true) {
        s = sampler.Next1D();
        float step = -std::log(1 - s) / (m_maxDensity * m_scale);
        // Outside the medium
        if (t + step >= d) {
            step = d - t;
            Tr *= std::exp(m_maxDensity * m_scale * (-step));
            pdf *= std::exp(m_maxDensity * m_scale * (-step));
            // Hit boundary
            mediumRec.m_p = ray(d);
            mediumRec.m_t = d;
            mediumRec.m_pdf = pdf;
            mediumRec.m_internal = false;
            return 1.f;
            float ret = pdf == 0 ? 0.f : Tr / pdf;
            if (std::isnan(ret)) {
                std::cout << Tr << ' ' << pdf << std::endl;
            }
            return ret;
        }
        // Get sigma_a, sigma_s(albedo is single channel)
        t += step;
        float sigma_t_bar = m_maxDensity * m_scale;
        float sigma_t = Density(ray(t)) * m_scale;
        float sigma_s = sigma_t * m_albedo.r;
        float sigma_a = sigma_t * std::max(0.f, 1 - m_albedo.r);
        float sigma_n = sigma_t_bar - sigma_t;

        Tr *= std::exp(m_maxDensity * m_scale * (-step));
        pdf *= std::exp(m_maxDensity * m_scale * (-step));
        // Sample particle's kind
        //int mod = SampleDiscrete({ sigma_s, sigma_a, sigma_n }, sampler.Next1D());
        int mod;
        s = sampler.Next1D();
        if (s <= sigma_s / sigma_t_bar) mod = 0;
        else if (s >= 1 - (sigma_n / sigma_t_bar)) mod = 2;
        else mod = 1;
        if (mod >= 1) mod = 2;

        //LOG_IF(FATAL, mod == 1) << "No absorption for now";

        if (mod == 0) {
            // Scatter
            //Tr *= sigma_s;
            //pdf *= sigma_s;
            // Hit authentic particle
            mediumRec.m_p = ray(t);
            mediumRec.m_t = t;
            mediumRec.m_pdf = pdf;
            mediumRec.m_internal = true;
            return 1.f;
			float ret = pdf == 0 ? 0.f : Tr / pdf;
			if (std::isnan(ret)) {
				std::cout << Tr << ' ' << pdf << std::endl;
			}
            return pdf == 0 ? 0.f : Tr / pdf;
        }
        else if (mod == 1) {
            // Absorb
			//Tr *= sigma_a;
			//pdf *= sigma_a;
            // Hit emitter
            //mediumRec.m_Le = BlackbodyRadiance(ray(t));
            return 0.f;
        }
        else if (mod == 2) {
            // Null
			//Tr *= (sigma_t_bar - sigma_s - sigma_a);
			//pdf *= (sigma_t_bar - sigma_s - sigma_a);
            // Hit fictional particle
        }
    }
    LOG_IF(FATAL, true) << "Fuck";
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
        float step = -std::log(1 - s) / (m_maxDensity * m_scale);
        t += step;           

        if (t >= d) break;

		float sigma_t_bar = m_maxDensity * m_scale;
		float sigma_t = Density(ray(t)) * m_scale;
        Tr *= (1 - sigma_t / sigma_t_bar);

        float rrThreshold = .1;
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
        //a = m_dG->LookUp(Float3(_pWorld.x, _pWorld.z, -_pWorld.y));
        //return m_dG->LookUp(Float3(_pWorld.x, _pWorld.z, -_pWorld.y));
    }
    else {        
        pWorld = openvdb::Vec3d(_pWorld.x, _pWorld.y, _pWorld.z);
        //a = m_dG->LookUp(Float3(_pWorld.x, _pWorld.y, _pWorld.z));
        //return m_dG->LookUp(Float3(_pWorld.x, _pWorld.y, _pWorld.z));
    }    
    pWorld = pWorld * 100;
    float ret = m_densitySampler.wsSample(pWorld);
    return ret;
}
