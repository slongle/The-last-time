#pragma once

#include "core/global.h"
#include "core/vector.h"
#include "core/primitive.h"
#include "shape/triangle.h"

#include <embree3/rtcore.h>

class EmbreeBVH {
public:
    EmbreeBVH(const std::vector<std::shared_ptr<Mesh>>& meshes, 
        const std::vector<Primitive>& primitives);
    ~EmbreeBVH();
    bool Intersect(Ray& ray, HitRecord& hitRec) const;
    bool Occlude(Ray& ray) const;
private:
    RTCDevice m_device;
    RTCScene m_scene;
    std::vector<std::shared_ptr<Mesh>> m_meshes;
    std::vector<Primitive> m_primitives;
    std::vector<int> m_prefix;
};