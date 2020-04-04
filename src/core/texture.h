#pragma once

#include "core/vector.h"
#include "core/spectrum.h"

template<typename T>
class Texture {    
public:
    virtual ~Texture() {}
    virtual T Evaluate(const Float2& uv) const = 0;
};