#pragma once

#include "global.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/geometric.hpp>

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

inline std::ostream& operator << (std::ostream& os, const Float3& v) {
    os << v.x << ' ' << v.y << ' ' << v.z;
    return os;
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

inline Float3 Cross(const Float3& u,const Float3& v) {
    return glm::cross(u, v);
}

inline float Radians(const float& deg) {
    return deg * 0.01745329251994329576923690768489f;
}