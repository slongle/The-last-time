#pragma once

#include "global.h"
#include "vector.h"

#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

typedef glm::mat4x4 Matrix4x4;

inline std::ostream& operator << (std::ostream& os, const Matrix4x4& t) {
    auto a = glm::transpose(t);
    os << glm::to_string(a[0]) << '\n'
        << glm::to_string(a[1]) << '\n'
        << glm::to_string(a[2]) << '\n'
        << glm::to_string(a[3]);
    return os;
}

class Transform {
public:
    Transform() :m(1.f), invM(1.f) {}
    Transform(const Matrix4x4& _m) :m(_m), invM(glm::inverse(_m)) {}
    Transform(const Matrix4x4& _m, const Matrix4x4& _invM) :m(_m), invM(_invM) {}

    Transform operator * (const Transform& t) const { return Transform(m * t.m, t.invM * invM); }
    Float3 TransformPoint(const Float3& p) const {
        Float4 ret = m * Float4(p, 1.f);
        ret /= ret.w;
        return Float3(ret.x, ret.y, ret.z);
    }

    Matrix4x4 m, invM;

    friend std::ostream& operator << (std::ostream& os, const Transform& t) {
        os << "m = \n" << t.m << "\nmInv = \n" << t.invM;
        return os;
    }
};

Transform Inverse(const Transform& t);
Transform LookAt(const Float3& pos, const Float3& lookat, const Float3& up);
Transform Perspective(const float& fov, const float& width, const float& height);
Transform Scale(const float& x, const float& y, const float& z);
Transform Translate(const float& x, const float& y, const float& z);