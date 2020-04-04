#pragma once

// Platform-specific definitions
#if defined(_WIN32) || defined(_WIN64)
#define IS_WINDOWS
#endif

#if defined(_MSC_VER)
#define IS_MSVC
#endif

#include <stdint.h>
#if defined(IS_MSVC)
#include <float.h>
#include <intrin.h>
#pragma warning(disable : 4305)  // double constant assigned to float
#pragma warning(disable : 4244)  // int -> float conversion
#pragma warning(disable : 4843)  // double -> float conversion
#pragma warning(disable : 4267)  // size_t -> int
#pragma warning(disable : 4838)  // another double -> int
#endif

#include <algorithm>
#include <memory>
#include <string>
#include <filesystem>
#include <iostream>
#include <unordered_map>
#include <cassert>
#include <numeric>

#undef M_PI

#define M_PI         3.14159265358979323846f
#define TWO_PI       6.28318530717958647692f
#define INV_PI       0.31830988618379067154f
#define INV_TWOPI    0.15915494309189533577f
#define INV_FOURPI   0.07957747154594766788f
#define SQRT_TWO     1.41421356237309504880f
#define INV_SQRT_TWO 0.70710678118654752440f

inline std::shared_ptr<std::filesystem::path> GetFileResolver(){
    static std::shared_ptr<std::filesystem::path> path(new std::filesystem::path());
    return path;
}

inline std::string GetFileExtension(const std::string& filename) {
    std::string ext = filename.substr(filename.find_last_of('.') + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(),
        [](const unsigned char& c) {return std::tolower(c); });
    return ext;
}
