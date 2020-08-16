#include "photon.h"

void KDTree::Add(const Photon& photon)
{
    m_mutex.lock();
    m_photons.push_back(photon);
    m_bounds = Union(m_bounds, Bounds(photon.m_position));
    m_mutex.unlock();
}

uint32_t KDTree::RecursiveBuild(uint32_t l, uint32_t r, Bounds& bounds, const float& radius)
{
    int axis = -1, lc = -1, rc = -1;
    uint32_t mid = (l + r) * 0.5f;

    if (r - l > 1) {
        // Non-leaf node
        axis = bounds.MaxAxis();
        std::nth_element(m_photons.begin() + l, m_photons.begin() + mid, m_photons.begin() + r,
            [&](const Photon& p1, const Photon& p2)->bool {
                return p1.m_position[axis] < p2.m_position[axis];
            });

        Bounds lb, rb;
        Split(bounds, axis, m_photons[mid].m_position[axis], lb, rb);

        if (mid > l) {
            lc = RecursiveBuild(l, mid, lb, radius);
        }
        if (r > mid + 1) {
            rc = RecursiveBuild(mid + 1, r, rb, radius);
        }

        // Build BBH        
        if (radius != 0) {
            //std::cout << "WTF1";
            Bounds BBHBounds(m_photons[mid].m_position, radius);
            if (lc != -1) BBHBounds = Union(BBHBounds, m_nodes[lc].m_bounds);
            if (rc != -1) BBHBounds = Union(BBHBounds, m_nodes[rc].m_bounds);
            bounds = BBHBounds;
        }
    }
    else {
        // Leaf node
        // Build BBH        
        if (radius != 0) {
            //std::cout << "WTF2";
            Bounds BBHBounds(m_photons[mid].m_position, radius);
            bounds = BBHBounds;
        }
    }

    KDTreeNode node(bounds, axis, lc, rc, mid);
    m_nodes.push_back(node);
    return m_nodes.size() - 1;
}

void KDTree::Build(const float& radius)
{
    m_root = RecursiveBuild(0, m_photons.size(), m_bounds, radius);
}

void KDTree::Clear()
{
    int photonNum = m_photons.size();
    int nodeNum = m_nodes.size();
    m_photons.clear();
    m_nodes.clear();
    m_photons.reserve(photonNum);
    m_nodes.reserve(nodeNum);
    m_bounds = Bounds();
}

void KDTree::Query(const Float3& center, const float& radius, std::vector<const Photon*>& photons) const
{
    if (m_photons.empty()) return;
    float sqrRadius = std::sqr(radius);
    std::stack<uint32_t> stack;
    stack.push(m_root);
    while (!stack.empty()) {
        uint32_t idx = stack.top(); stack.pop();
        const auto& node = m_nodes[idx];
        const auto& photon = m_photons[node.m_photonIndex];
        float minSqrDistance =
            std::sqr(std::max(0.f, std::max(center.x - node.m_bounds.m_pMax.x,
                node.m_bounds.m_pMin.x - center.x))) +
            std::sqr(std::max(0.f, std::max(center.y - node.m_bounds.m_pMax.y,
                node.m_bounds.m_pMin.y - center.y))) +
            std::sqr(std::max(0.f, std::max(center.z - node.m_bounds.m_pMax.z,
                node.m_bounds.m_pMin.z - center.z)));
        if (minSqrDistance <= sqrRadius) {
            if (SqrLength(center - photon.m_position) <= sqrRadius) {
                photons.push_back(&photon);
            }
            if (node.m_children[0] != -1) {
                stack.push(node.m_children[0]);
            }
            if (node.m_children[1] != -1) {
                stack.push(node.m_children[1]);
            }
        }
    }
}

void KDTree::Query(const Float3& center, const float& radius, std::vector<Photon>& photons) const
{
    if (m_photons.empty()) return;
    float sqrRadius = std::sqr(radius);
    std::stack<uint32_t> stack;
    stack.push(m_root);
    while (!stack.empty()) {
        uint32_t idx = stack.top(); stack.pop();
        const auto& node = m_nodes[idx];
        const auto& photon = m_photons[node.m_photonIndex];
        float minSqrDistance =
            std::sqr(std::max(0.f, std::max(center.x - node.m_bounds.m_pMax.x,
                node.m_bounds.m_pMin.x - center.x))) +
            std::sqr(std::max(0.f, std::max(center.y - node.m_bounds.m_pMax.y,
                node.m_bounds.m_pMin.y - center.y))) +
            std::sqr(std::max(0.f, std::max(center.z - node.m_bounds.m_pMax.z,
                node.m_bounds.m_pMin.z - center.z)));
        if (minSqrDistance <= sqrRadius) {
            if (SqrLength(center - photon.m_position) <= sqrRadius) {
                photons.push_back(photon);
            }
            if (node.m_children[0] != -1) {
                stack.push(node.m_children[0]);
            }
            if (node.m_children[1] != -1) {
                stack.push(node.m_children[1]);
            }
        }
    }
}

bool IntersectSphere(const Float3& center, const float& radius, const Ray& ray, float* t) {
    float a = SqrLength(ray.d);
    float b = 2 * Dot(ray.o - center, ray.d);
    float c = SqrLength(ray.o - center) - std::sqr(radius);

    float t0, t1;
    if (!math::SolveQuadratic(a, b, c, t0, t1)) return false;

    *t = (t0 + t1) * 0.5f;
    return (ray.tMin < *t) && (*t < ray.tMax);
}

void KDTree::QueryBeam(
    const Ray& ray,
    const float& radius,
    std::vector<std::pair<const Photon*, float>>& photons) const
{
    if (m_photons.empty()) return;
    std::stack<uint32_t> stack;
    stack.push(m_root);
    while (!stack.empty()) {
        uint32_t idx = stack.top(); stack.pop();
        const auto& node = m_nodes[idx];
        const auto& photon = m_photons[node.m_photonIndex];
        // Check ray intersect bounds
        if (node.m_bounds.Intersect(ray)) {
            // Check the beam contain the photon
            float t;
            if (IntersectSphere(photon.m_position, radius, ray, &t)) {
                photons.push_back(std::make_pair(&photon, t));
            }
            // Add children nodes
            if (node.m_children[0] != -1) {
                stack.push(node.m_children[0]);
            }
            if (node.m_children[1] != -1) {
                stack.push(node.m_children[1]);
            }
        }
    }
}
