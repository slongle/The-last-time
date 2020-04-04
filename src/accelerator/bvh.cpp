#include "bvh.h"

/// Heuristic cost value for traversal operations
#define TRAVERSAL_COST 1

/// Heuristic cost value for intersection operations
#define INTERSECTION_COST 1

void BVH::Build()
{
    int primNum = m_primitives.size();
    m_nodes.resize(primNum * 2);

    float* tmp = new float[primNum];
    int nodeNum = RecursiveBuild(0, 0, primNum, tmp);
    delete[] tmp;

    m_nodes.resize(nodeNum);    
}

bool BVH::Intersect(Ray& ray, HitRecord& hitRec) const
{
    int nodeIdx = 0, stackIdx = 0, stack[65];

    bool hit = false;
    while (true) {
        const BVHNode& node = m_nodes[nodeIdx];
        if (!node.m_bounds.Intersect(ray)) {
            if (stackIdx == 0) {
                break;
            }
            nodeIdx = stack[--stackIdx];
        }
        else {
            if (node.m_inner.m_flag == 0) {
                stack[stackIdx++] = node.m_inner.m_rightChild;
                nodeIdx++;
            }
            else {
                for (uint32_t i = node.m_leaf.m_start; i < node.m_leaf.m_start + node.m_leaf.m_size; i++) {
                    if (m_primitives[i].m_shape->Intersect(ray, hitRec)) {                        
                        ray.tMax = hitRec.m_t;
                        hitRec.m_primitive = &m_primitives[i];
                        hit = true;
                    }
                }
                if (stackIdx == 0) {
                    break;
                }
                nodeIdx = stack[--stackIdx];
            }
        }
    }
    if (hit) {
        hitRec.m_primitive->m_shape->SetGeometryRecord(hitRec.m_geoRec);
    }
    return hit;
}

bool BVH::Occlude(Ray& ray) const
{
    int nodeIdx = 0, stackIdx = 0, stack[65];

    HitRecord hitRec;
    bool hit = false;
    while (true) {
        const BVHNode& node = m_nodes[nodeIdx];
        if (!node.m_bounds.Intersect(ray)) {
            if (stackIdx == 0) {
                break;
            }
            nodeIdx = stack[--stackIdx];
            continue;
        }
        else {
            if (node.m_inner.m_flag == 0) {
                stack[stackIdx++] = node.m_inner.m_rightChild;
                nodeIdx++;
            }
            else {
                for (uint32_t i = node.m_leaf.m_start; i < node.m_leaf.m_start + node.m_leaf.m_size; i++) {
                    if (m_primitives[i].m_shape->Intersect(ray, hitRec)) {                        
                        return true;
                    }
                }
                if (stackIdx == 0) {
                    break;
                }
                nodeIdx = stack[--stackIdx];
            }
        }
    }
    return false;
}

int BVH::RecursiveBuild(int idx, int l, int r, float* tmp)
{
    int size = r - l;
    float* leftArea = tmp;
    BVHNode& node = m_nodes[idx];

    float bestCost = INTERSECTION_COST * size;
    int bestAxis = -1, bestIndex = 0;

    for (int axis = 0; axis < 3; axis++) {
        std::sort(m_primitives.begin() + l, m_primitives.begin() + r, 
            [&](const Primitive& p1, const Primitive& p2) {
                return p1.m_shape->GetBounds().Centroid()[axis]
                    < p2.m_shape->GetBounds().Centroid()[axis];
            });
        {
            Bounds bounds;
            for (int i = 0; i < size; i++) {
                bounds = Union(bounds, m_primitives[l + i].m_shape->GetBounds());
                leftArea[i] = bounds.Area();
            }
            if (axis == 0) {
                node.m_bounds = bounds;
            }
        }
        {
            Bounds bounds;
            float triFactor = INTERSECTION_COST / node.m_bounds.Area();
            for (int i = size - 1; i >= 1; i--) {
                bounds = Union(bounds, m_primitives[l + i].m_shape->GetBounds());
                float lArea = leftArea[i - 1], rArea = bounds.Area();
                int pLeft = i, pRight = size - i;
                float sahCost = 2.0f * TRAVERSAL_COST + triFactor * (pLeft * lArea + pRight * rArea);
                if (sahCost < bestCost) {
                    bestCost = sahCost;
                    bestAxis = axis;
                    bestIndex = i;
                }
            }
        }
    }

    if (bestAxis == -1) {
        node.m_leaf.m_flag = 1;
        node.m_leaf.m_size = size;
        node.m_leaf.m_start = l;
        return idx + 1;
    }
    else {
        std::sort(m_primitives.begin() + l, m_primitives.begin() + r,
            [&](const Primitive& p1, const Primitive& p2) {
                return p1.m_shape->GetBounds().Centroid()[bestAxis]
                    < p2.m_shape->GetBounds().Centroid()[bestAxis];
            });
        int nxtIdx;
        nxtIdx = RecursiveBuild(idx + 1, l, l + bestIndex, tmp);
        node.m_inner.m_flag = 0;
        node.m_inner.m_rightChild = nxtIdx;
        nxtIdx = RecursiveBuild(nxtIdx, l + bestIndex, r, tmp);
        return nxtIdx;
    }
}
