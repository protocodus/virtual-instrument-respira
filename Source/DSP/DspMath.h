#pragma once

// The Rack Extension toolchain compiles with -fno-builtin, which turns the
// libc++ spellings of these into out-of-line calls (its sqrt is a bit-serial
// software routine there). The builtins are the same IEEE operations in line.

#include <cmath>
#include <cstdint>

namespace respira {

#if defined(__GNUC__) || defined(__clang__)
inline double Sqrt(double x) { return __builtin_sqrt(x); }
inline double Abs(double x) { return __builtin_fabs(x); }
inline float Abs(float x) { return __builtin_fabsf(x); }
#else
inline double Sqrt(double x) { return std::sqrt(x); }
inline double Abs(double x) { return std::fabs(x); }
inline float Abs(float x) { return std::fabs(x); }
#endif

// The index of a word's lowest set bit (the word is not zero).
#if defined(__GNUC__) || defined(__clang__)
inline int LowestBit(std::uint64_t bits) { return __builtin_ctzll(bits); }
#else
inline int LowestBit(std::uint64_t bits) {
    int i = 0;
    while (!(bits & 1)) { bits >>= 1; ++i; }
    return i;
}
#endif

inline double Min(double a, double b) { return a < b ? a : b; }
inline double Max(double a, double b) { return a > b ? a : b; }
inline double Clamp(double x, double low, double high) { return x < low ? low : (x > high ? high : x); }

// The variance two cascaded one-pole low-passes, y += c (x - y), leave of
// unit-variance white noise: c^4 (1 + r^2) / (1 - r^2)^3, r = 1 - c.
inline double CascadeVariance(double c) {
    const double r = 1.0 - c, d = 1.0 - r * r;
    return c * c * c * c * (1.0 + r * r) / (d * d * d);
}

} // namespace respira
