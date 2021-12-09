#pragma once

#include "core/primitive.h"

#include "grid.h"

#include <openvdb/openvdb.h>
#include <openvdb/tools/Interpolation.h>
#include <openvdb/tools/ValueTransformer.h>
#include <openvdb/tools/Dense.h>

class HeterogeneousMedium :public Medium {
public:
    HeterogeneousMedium(
        const std::shared_ptr<PhaseFunction>& pf,
        const std::string& filename,
        const bool& lefthand,
        const std::string& densityName,
        const Spectrum& albedo,
        const float& scale,
        const std::string& temperatureName,
        const float& temperatureScale);

    Spectrum Sample(const Ray& ray, MediumRecord& mediumRec, Sampler& sampler) const;
    Spectrum Transmittance(const Ray& ray, Sampler& sampler) const;

    float Density(const Float3& p) const;
    float Temperature(const Float3& p) const;

    
    Spectrum m_albedo;
    float m_scale;
    float m_temperatureScale;

    float m_minDensity, m_maxDensity;
    float m_invMaxDensity;

    typedef openvdb::tools::GridSampler<openvdb::FloatGrid, openvdb::tools::BoxSampler> VDBFloatSampler;
    typedef openvdb::FloatGrid::Ptr VDBFloatGridPtr;
    VDBFloatGridPtr m_densityGrid;
    VDBFloatSampler m_densitySampler;
	VDBFloatGridPtr m_temperatureGrid;
	VDBFloatSampler m_temperatureSampler;

    bool m_lefthand;
    bool m_blackbody;
};