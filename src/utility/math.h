#pragma once

namespace math {
    inline float signum(float v) {
        return copysign(1, v);
    }

    inline bool SolveQuadratic(float a, float b, float c, float& x0, float& x1) {
        /* Linear case */
        if (a == 0) {
            if (b != 0) {
                x0 = x1 = -c / b;
                return true;
            }
            return false;
        }

        float discrim = b * b - 4.0f * a * c;

        /* Leave if there is no solution */
        if (discrim < 0)
            return false;

        float temp, sqrtDiscrim = std::sqrt(discrim);

        /* Numerically stable version of (-b (+/-) sqrtDiscrim) / (2 * a)
         *
         * Based on the observation that one solution is always
         * accurate while the other is not. Finds the solution of
         * greater magnitude which does not suffer from loss of
         * precision and then uses the identity x1 * x2 = c / a
         */
        if (b < 0)
            temp = -0.5f * (b - sqrtDiscrim);
        else
            temp = -0.5f * (b + sqrtDiscrim);

        x0 = temp / a;
        x1 = c / temp;

        /* Return the results so that x0 < x1 */
        if (x0 > x1)
            std::swap(x0, x1);

        return true;
    }

    inline bool SolveQuadraticDouble(double a, double b, double c, double& x0, double& x1) {
        /* Linear case */
        if (a == 0) {
            if (b != 0) {
                x0 = x1 = -c / b;
                return true;
            }
            return false;
        }

        double discrim = b * b - 4.0f * a * c;

        /* Leave if there is no solution */
        if (discrim < 0)
            return false;

        double temp, sqrtDiscrim = std::sqrt(discrim);

        /* Numerically stable version of (-b (+/-) sqrtDiscrim) / (2 * a)
         *
         * Based on the observation that one solution is always
         * accurate while the other is not. Finds the solution of
         * greater magnitude which does not suffer from loss of
         * precision and then uses the identity x1 * x2 = c / a
         */
        if (b < 0)
            temp = -0.5f * (b - sqrtDiscrim);
        else
            temp = -0.5f * (b + sqrtDiscrim);

        x0 = temp / a;
        x1 = c / temp;

        /* Return the results so that x0 < x1 */
        if (x0 > x1)
            std::swap(x0, x1);

        return true;
    }

}