#pragma once

#include "core/spectrum.h"

std::shared_ptr<float[]> ReadImage(
    const std::string& filename,
    int* width,
    int* height,
    int* channel,
    int reqChannel);

void WriteImage(
    const std::string& filename,
    const int& width, 
    const int& height,
    float* buffer);