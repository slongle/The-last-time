#include "transform.h"

Transform Inverse(const Transform& t)
{
    return Transform(t.invM, t.m);
}

Transform LookAt(const Float3& pos, const Float3& lookat, const Float3& up)
{
    Matrix4x4 result(1.f);
    Float3 zDir = Normalize(pos - lookat);
    Float3 xDir = Cross(up, zDir);
    Float3 yDir = Cross(zDir, xDir);
    result[0] = Float4(xDir, 0);
    result[1] = Float4(yDir, 0);
    result[2] = Float4(zDir, 0);
    result[3] = Float4(pos, 1);
    return Transform(result);
}

Transform Perspective(const float& fov, const float& width, const float& height)
{
    return Transform(glm::perspectiveFovRH(Radians(fov), width, height, 0.001f, 10000.f));
}

Transform Scale(const float& x, const float& y, const float& z)
{
    Matrix4x4 I(1.0f);
    return Transform(glm::scale(I, Float3(x, y, z)), glm::scale(I, Float3(1.f / x, 1.f / y, 1.f / z)));
}

Transform Translate(const float& x, const float& y, const float& z)
{
    Matrix4x4 I(1.0f);
    return Transform(glm::translate(I, Float3(x, y, z)), glm::translate(I, Float3(-x, -y, -z)));
}
