#pragma once

#include "core/global.h"
#include "core/vector.h"
#include "core/primitive.h"

class BVH {
public:
    BVH(const std::vector<Primitive>& p) :m_primitives(p) {}

    void Build();
    bool Intersect(Ray& ray, HitRecord& hitRec) const;
    bool Occlude(Ray& ray) const;

private:
    int RecursiveBuild(int idx, int l, int r, float* tmp);

    struct BVHNode {
        union {
            /**
             * flag = 1
             * size is the number of shapes in the leaf node
             * start is the first shape's index in m_primitives array
             */
            struct {
                unsigned m_flag : 1;
                uint32_t m_size : 31;
                uint32_t m_start;
            } m_leaf;

            /**
             * flag = 0
             * 
             * leftChild is idx + 1
             */
            struct {
                unsigned m_flag : 1;
                uint32_t m_nextQuery : 31;
                uint32_t m_rightChild;
            } m_inner;

            uint64_t m_data;
        };
        Bounds m_bounds;
    };

private:
    std::vector<Primitive> m_primitives;
    std::vector<BVHNode> m_nodes;
};