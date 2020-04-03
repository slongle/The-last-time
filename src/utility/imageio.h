#pragma once

#include "core/spectrum.h"

std::unique_ptr<float[]> ReadImage(
    const std::string& filename,
    int* width,
    int* height);

void WriteImage(
    const std::string& filename,
    const int& width, 
    const int& height,
    float* buffer);