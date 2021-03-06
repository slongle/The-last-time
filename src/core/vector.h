#pragma once

#include "global.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/geometric.hpp>

namespace std {
    inline float sqr(float a) {
        return a * a;
    }
};

/*
class Float3{
public:
};

class Float2 {
public:
};
*/

typedef glm::vec2 Float2;
typedef glm::vec3 Float3;
typedef glm::vec4 Float4;
typedef glm::ivec2 Int2;
typedef glm::ivec3 Int3;
typedef glm::u64vec3 ULL3;

//inline void ReSample(Float2& s, const float& v) {}

inline Float2 operator * (const Float2& u, const Int2& v) {
    return Float2(u.x * v.x, u.y * v.y);
}

inline Float2 operator + (const Float2& u, const Int2& v) {
    return Float2(u.x + v.x, u.y + v.y);
}

inline std::ostream& operator << (std::ostream& os, const Float2& v) {
    os << v.x << ' ' << v.y;
    return os;
}

inline std::ostream& operator << (std::ostream& os, const Float3& v) {
    os << v.x << ' ' << v.y << ' ' << v.z;
    return os;
}

inline std::ostream& operator << (std::ostream& os, const Int3& v) {
    os << v.x << ' ' << v.y << ' ' << v.z;
    return os;
}

inline Float2 Sqrt(const Float2& v) {
    return Float2(math::SafeSqrt(v.x), math::SafeSqrt(v.y));
}

inline Float2 Sqr(const Float2& v) {
    return Float2(math::Sqr(v.x), math::Sqr(v.y));
}

inline Float2 Mod(const Float2& u, const Int2& v) {
    return Float2(u.x - int(u.x) / v.x * v.x,
        u.y - int(u.y) / v.y * v.y);
}

inline float Length(const Float3& v) {
    return glm::length(v);
}

inline float SqrLength(const Float3& v) {
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

inline Float3 Normalize(const Float3& v) {
    return glm::normalize(v);
}

inline float Dot(const Float3& u, const Float3& v) {
    return glm::dot(u, v);
}

inline float AbsDot(const Float3& u, const Float3& v) {
    return std::abs(glm::dot(u, v));
}

inline Float3 Cross(const Float3& u, const Float3& v) {
    return glm::cross(u, v);
}

inline float Radians(const float& deg) {
    return deg * 0.01745329251994329576923690768489f;
}

inline Float3 Min(const Float3& v1, const Float3& v2) {
    return glm::min(v1, v2);
}

inline Float3 Max(const Float3& v1, const Float3& v2) {
    return glm::max(v1, v2);
}

inline Float3 Floor(const Float3& v) {
    return Float3(std::floor(v.x), std::floor(v.y), std::floor(v.z));
}

class Ray {
public:
    Ray() {}
    Ray(const Float3& _o, const Float3& _d,
        const std::shared_ptr<Medium>& _m = nullptr)
        :o(_o), d(_d), tMin(epsilon), tMax(std::numeric_limits<float>::infinity()), m_medium(_m) {}
    Ray(const Float3& _o, const Float3& _d, const float& _tMin, const float& _tMax,
        const std::shared_ptr<Medium>& _m = nullptr)
        :o(_o), d(_d), tMin(_tMin), tMax(_tMax), m_medium(_m) {}

    Float3 operator ()(const float& t) const { return o + d * t; }

    Float3 o, d;
    float tMin, tMax;
    std::shared_ptr<Medium> m_medium;

    static float epsilon;
    static float shadowEpsilon;
};

inline float Ray::epsilon = 2e-4;
inline float Ray::shadowEpsilon = 1e-3;


class Bounds {
public:
    Bounds() :m_pMin(std::numeric_limits<float>::max()), m_pMax(std::numeric_limits<float>::min()) {}
    Bounds(const Float3& p) :m_pMin(p), m_pMax(p) {}
    Bounds(const Float3& pMin, const Float3& pMax) :m_pMin(pMin), m_pMax(pMax) {}
    Bounds(const Float3& center, const float& radius) :
        m_pMin(center - Float3(radius)), m_pMax(center + Float3(radius)) {}

    const Float3& operator[](const int& idx) const { return idx == 0 ? m_pMin : m_pMax; }

    float Area() const {
        Float3 diag = m_pMax - m_pMin;
        return 2.f * (diag.x * diag.y + diag.x * diag.z + diag.y * diag.z);
    }

    Float3 Centroid() const { return (m_pMin + m_pMax) * 0.5f; }

    int MaxAxis() const {
        Float3 diag = m_pMax - m_pMin;
        return diag.x > diag.y ? (diag.x > diag.z ? 0 : 2) : (diag.y > diag.z ? 1 : 2);
    }

    bool Contain(const Float3& p) const {
        return m_pMin.x <= p.x && p.x <= m_pMax.x &&
            m_pMin.y <= p.y && p.y <= m_pMax.y &&
            m_pMin.z <= p.z && p.z <= m_pMax.z;
    }

    bool Intersect(const Ray& ray) const {
        float nearT = -std::numeric_limits<float>::infinity();
        float farT = std::numeric_limits<float>::infinity();

        for (int i = 0; i < 3; i++) {
            float origin = ray.o[i];
            float minVal = m_pMin[i], maxVal = m_pMax[i];

            if (ray.d[i] == 0) {
                if (origin < minVal || origin > maxVal)
                    return false;
            }
            else {
                float t1 = (minVal - origin) / ray.d[i];
                float t2 = (maxVal - origin) / ray.d[i];
                if (t1 > t2) {
                    std::swap(t1, t2);
                }
                nearT = std::max(t1, nearT);
                farT = std::min(t2, farT);
                if (!(nearT <= farT)) {
                    return false;
                }
            }
        }

        return ray.tMin <= farT && nearT <= ray.tMax;
    }

    bool Intersect(const Ray& ray, const Float3& invDir, const int* dirIsNeg) const {
        const Bounds& bounds = *this;

        // Check for ray intersection against $x$ and $y$ slabs
        float tMin = (bounds[dirIsNeg[0]].x - ray.o.x) * invDir.x;
        float tMax = (bounds[1 - dirIsNeg[0]].x - ray.o.x) * invDir.x;
        float tyMin = (bounds[dirIsNeg[1]].y - ray.o.y) * invDir.y;
        float tyMax = (bounds[1 - dirIsNeg[1]].y - ray.o.y) * invDir.y;
        //if (tMin > tMax) std::swap(tMin, tMax);
        //if (tyMin > tyMax) std::swap(tyMin, tyMax);

        // Update _tMax_ and _tyMax_ to ensure robust bounds intersection
        if (tMin > tyMax || tyMin > tMax) return false;
        if (tyMin > tMin) tMin = tyMin;
        if (tyMax < tMax) tMax = tyMax;

        // Check for ray intersection against $z$ slab
        float tzMin = (bounds[dirIsNeg[2]].z - ray.o.z) * invDir.z;
        float tzMax = (bounds[1 - dirIsNeg[2]].z - ray.o.z) * invDir.z;
        //if (tzMin > tzMax) std::swap(tzMin, tzMax);

        // Update _tzMax_ to ensure robust bounds intersection
        if (tMin > tzMax || tzMin > tMax) return false;
        if (tzMin > tMin) tMin = tzMin;
        if (tzMax < tMax) tMax = tzMax;
        return ray.tMin <= tMax && tMin <= ray.tMax;
    }

    std::string ToString() const {
        return fmt::format("pMin : [ {:.1f}, {:.1f}, {:.1f} ]\npMax : [ {:.1f}, {:.1f}, {:.1f} ]",
            m_pMin.x, m_pMin.y, m_pMin.z, m_pMax.x, m_pMax.y, m_pMax.z);
    }

    Float3 m_pMin, m_pMax;
};

inline Bounds Union(const Bounds& b1, const Bounds& b2) {
    return Bounds(Min(b1.m_pMin, b2.m_pMin), Max(b1.m_pMax, b2.m_pMax));
}

inline void Split(const Bounds& bounds, int axis, float val, Bounds& lb, Bounds& rb) {
    lb = Bounds(bounds), rb = Bounds(bounds);
    lb.m_pMax[axis] = rb.m_pMin[axis] = val;
}