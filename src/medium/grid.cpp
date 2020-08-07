#include "grid.h"

Grid::Grid(
    const std::string& filename, 
    const std::string& grdiname, 
    const float& scale)
    : m_scale(scale)
{
    openvdb::initialize();
    openvdb::io::File file(GetFileResolver()->string() + "/" + filename);
    std::cout << "Loading " << filename << std::endl;

    file.open();
    openvdb::GridBase::Ptr baseGrid;
    baseGrid = file.readGrid(grdiname);
    file.close();

    openvdb::FloatGrid::Ptr grid = openvdb::gridPtrCast<openvdb::FloatGrid>(baseGrid);
    openvdb::tools::GridSampler<openvdb::FloatGrid, openvdb::tools::BoxSampler> sampler(*grid);

    auto bboxDim = grid->evalActiveVoxelDim();
    auto bbox = grid->evalActiveVoxelBoundingBox();
    auto ws_min = grid->indexToWorld(bbox.min());
    auto ws_max = grid->indexToWorld(bbox.max());

    m_resolution = ULL3(bboxDim.x() - 1, bboxDim.y() - 1, bboxDim.z() - 1);
    m_worldBounds = Bounds(Float3(ws_min.x(), ws_min.y(), ws_min.z()),
        Float3(ws_max.x(), ws_max.y(), ws_max.z()));    
    m_voxelLength = (m_worldBounds.m_pMax - m_worldBounds.m_pMin) / Float3(m_resolution);



    m_grid =std::unique_ptr<float[]>(new float[m_resolution.x * m_resolution.y * m_resolution.z]);
    uint32_t idx = 0;
    for (int k = bbox.min().z(); k < bbox.max().z(); ++k) {
        for (int j = bbox.min().y(); j < bbox.max().y(); ++j) {
            for (int i = bbox.min().x(); i < bbox.max().x(); ++i) {
                float value = sampler.isSample(openvdb::Vec3R(i, j, k));
                m_grid[idx++] = value;
            }
        }
    }
}

float Grid::LookUp(const Float3& pos) const
{
    Float3 is_pos = (pos - m_worldBounds.m_pMin) / m_voxelLength;
    Int3 p0 = Floor(is_pos), p1 = p0 + Int3(1, 1, 1);
    int x0 = p0.x, y0 = p0.y, z0 = p0.z, x1 = p1.x, y1 = p1.y, z1 = p1.z;

    if (x0 < 0 || y0 < 0 || z0 < 0 || 
        x1 >= m_resolution.x || y1 >= m_resolution.y || z1 >= m_resolution.z) {
        return 0;
    }

    float fx = is_pos.x - x0, fy = is_pos.y - y0, fz = is_pos.z - z0;

    float d000 = GetVal(Int3(x0, y0, z0));
    float d001 = GetVal(Int3(x1, y0, z0));
    float d010 = GetVal(Int3(x0, y1, z0));
    float d011 = GetVal(Int3(x1, y1, z0));
    float d100 = GetVal(Int3(x0, y0, z1));
    float d101 = GetVal(Int3(x1, y0, z1));
    float d110 = GetVal(Int3(x0, y1, z1));
    float d111 = GetVal(Int3(x1, y1, z1));
    
    float val = Lerp(
        Lerp(Lerp(d000, d001, fx), Lerp(d010, d011, fx), fy),
        Lerp(Lerp(d100, d101, fx), Lerp(d110, d111, fx), fy), fz);
    return val * m_scale;
}

float Grid::GetVal(const Int3& p) const
{
    uint32_t idx = ((p.z * m_resolution.y) + p.y)* m_resolution.x + p.x;
    return m_grid[idx];
}
