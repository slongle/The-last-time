#pragma once

namespace math {
    float Signum(float v);
    float SafeSqrt(float v);
    float Sqr(float v);
    bool SolveQuadratic(float a, float b, float c, float& x0, float& x1);
    bool SolveQuadraticDouble(double a, double b, double c, double& x0, double& x1);
}