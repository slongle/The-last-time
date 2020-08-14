#pragma once

#include "core/vector.h"
#include "core/spectrum.h"

#include <atomic>
#include <thread>
#include <mutex>

struct Photon {
    Photon(const Float3& p, const Float3& d, const Spectrum& f)
        :m_position(p), m_direction(d), m_flux(f) {}

    Float3 m_position;
    Float3 m_direction;
    Spectrum m_flux;
};

struct KDTreeNode {
    KDTreeNode(const Bounds& bounds, int axis, int lc, int rc, uint32_t photonIndex)
        :m_bounds(bounds), m_axis(axis), m_children{ lc, rc }, m_photonIndex(photonIndex) {}
    KDTreeNode(const KDTreeNode& node)
        :m_bounds(node.m_bounds), m_axis(node.m_axis), m_children(node.m_children),
        m_photonIndex(node.m_photonIndex) {}

    Bounds m_bounds;
    int m_axis;
    std::array<int, 2> m_children;

    uint32_t m_photonIndex;
};

class KDTree {
public:
    void Add(const Photon& photon);
    void Build();
    void Clear();
    void Query(Float3 center, const float& radius, std::vector<Photon>& photons) const;

    uint32_t RecursiveBuild(uint32_t l, uint32_t r, const Bounds& bounds);

    uint32_t m_root;
    std::vector<Photon> m_photons;
    std::vector<KDTreeNode> m_nodes;

    std::mutex m_mutex;
    Bounds m_bounds;
};