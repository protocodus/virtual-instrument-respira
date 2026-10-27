#pragma once

// The Rack Extension toolchain compiles with -fno-builtin, which turns the
// libc++ spellings of these into out-of-line calls (its sqrt is a bit-serial
// software routine there). The builtins are the same IEEE operations in line.

#include <cmath>

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

inline double Min(double a, double b) { return a < b ? a : b; }
inline double Max(double a, double b) { return a > b ? a : b; }
inline double Clamp(double x, double low, double high) { return x < low ? low : (x > high ? high : x); }

} // namespace respira
