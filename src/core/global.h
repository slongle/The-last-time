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
#pragma warning(disable : 4018)  // compare int with uint

// non ¨C DLL-interface class 'std::runtime_error' used as base for DLL-interface class
#pragma warning(disable : 4275)  // warning in fmtlib 
#pragma warning(disable : 4566)  // warning in fmtlib
#pragma warning(disable : 4146)  // warning in OpenVDB
#pragma warning(disable : 4251)  // warning in OpenVDB

#endif

#include <array>
#include <algorithm>
#include <memory>
#include <string>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <cassert>
#include <numeric>
#include <stack>

#include <fmt/format.h>
#include <glog/logging.h>

#include "utility/math.h"

#undef M_PI

#define M_PI         3.14159265358979323846f
#define TWO_PI       6.28318530717958647692f
#define INV_PI       0.31830988618379067154f
#define INV_TWOPI    0.15915494309189533577f
#define INV_FOURPI   0.07957747154594766788f
#define SQRT_TWO     1.41421356237309504880f
#define INV_SQRT_TWO 0.70710678118654752440f

class Medium;

inline std::string FormatNumber(const uint64_t& num) {
    if (num < 100) {
        return fmt::format("{}", num);
    }
    else if (num < 100000) {
        return fmt::format("{:.1f}K", num * 0.001);
    }
    else {
        return fmt::format("{:.1f}M", num * 0.000001);
    }
}

inline std::shared_ptr<std::filesystem::path> GetFileResolver() {
    static std::shared_ptr<std::filesystem::path> path(new std::filesystem::path());
    return path;
}

inline std::string GetFilename(const std::string& filename) {
    std::string name = filename.substr(0, filename.find_last_of('.'));
    return name;
}

inline std::string GetFileExtension(const std::string& filename) {
    std::string ext = filename.substr(filename.find_last_of('.') + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(),
        [](const unsigned char& c) {return std::tolower(c); });
    return ext;
}

template<typename T>
inline T Lerp(const T& l, const T& r, const float& s) {
    return l * (1 - s) + r * s;
}