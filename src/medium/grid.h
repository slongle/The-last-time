#pragma once

#include "core/primitive.h"

#include <openvdb/openvdb.h>
#include <openvdb/tools/Interpolation.h>
#include <openvdb/tools/ValueTransformer.h>
#include <openvdb/tools/Dense.h>

class Grid {
public:
    Grid(
        const std::string& filename,
        const std::string& grdiname,
        const float& scale);

    float LookUp(const Float3& pos) const;
    float GetVal(const Int3& p) const;

    float m_scale;
    Bounds m_worldBounds;
    ULL3 m_resolution;
    Float3 m_voxelLength;
    std::unique_ptr<float[]> m_grid;
};