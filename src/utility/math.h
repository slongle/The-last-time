#pragma once

namespace math {
    inline float signum(float v) {
        return copysign(1, v);
    }
}