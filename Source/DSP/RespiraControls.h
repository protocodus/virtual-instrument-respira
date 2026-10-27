#pragma once

// Respira's player controls, shared by every front end: the property or
// parameter id, the range in physical units, the default and the mapping from
// a normalised 0..1 position. Front ends store and automate the normalised
// positions; the engine takes physical values (SetControls).

#include "DspMath.h"

#include <array>
#include <cmath>

namespace respira {

enum Control : int {
    kRegister,   // treble register switch (kRegisters, RespiraEngine.h)
    kMusette,    // detune of the M- and M+ tremolo reeds at A4, cents
    kBellows,    // air one full sweep of the wheel moves, litres
    kAssist,     // 0 strict air .. 1 the hand moves as much air as the open reeds need
    kGrille,     // treble grille: brightness of the treble side, Hz
    kWidth,      // stereo spread: treble keys across the right, bass side to the left
    kBass,       // bass and chord side level, dB
    kVolume,     // output level, dB
    kKeyboard,   // 0 split (bass and chord octaves below C3), 1 treble on every key
    kAir,        // air button: vents the bellows so the hand moves without sound
    kMomentum,   // the arm's lag behind the wheel, ms: 0 the wheel moves the bellows directly
    kOversampling, // 0, 1, 2: 1x, 2x, 4x the engine's own internal rate (4x the host rate)
    kControlCount
};

enum class Curve { kLinear, kLog, kStepped };

struct ControlInfo {
    const char* id;        // stable parameter / property id
    const char* name;
    double min, max, initial;
    int steps;             // > 0 for a switch: values 0 .. steps - 1
    Curve curve;
    const char* unit;
};

inline constexpr std::array<ControlInfo, kControlCount> kControlInfo { {
    { "register", "Register", 0.0, 7.0, 3.0, 8, Curve::kStepped, "" },
    { "musette", "Musette", 0.0, 40.0, 22.0, 0, Curve::kLinear, "cents" },
    { "bellows", "Bellows", 0.5, 8.0, 2.0, 0, Curve::kLog, "L" },
    { "assist", "Assist", 0.0, 1.0, 0.5, 0, Curve::kLinear, "" },
    { "grille", "Grille", 2000.0, 16000.0, 9000.0, 0, Curve::kLog, "Hz" },
    { "width", "Width", 0.0, 1.0, 0.6, 0, Curve::kLinear, "" },
    { "bass", "Bass", -24.0, 6.0, 0.0, 0, Curve::kLinear, "dB" },
    { "volume", "Volume", -36.0, 6.0, 0.0, 0, Curve::kLinear, "dB" },
    { "keyboard", "Keyboard", 0.0, 1.0, 0.0, 2, Curve::kStepped, "" },
    { "air", "Air", 0.0, 1.0, 0.0, 2, Curve::kStepped, "" },
    // Voiced, not measured: at 25 ms a bellows reversal takes 84 ms (10-90 %
    // of the velocity swing, 3.36 time constants), and a bellows shake of four
    // full cycles a second keeps 72 % of its travel (1 / (1 + (2 pi f T)^2)).
    { "momentum", "Momentum", 0.0, 200.0, 25.0, 0, Curve::kLinear, "ms" },
    { "oversampling", "Oversampling", 0.0, 2.0, 0.0, 3, Curve::kStepped, "" },
} };

// Physical value of control `index` at normalised position n (clamped to 0..1).
inline double FromNormalized(int index, double n) {
    const ControlInfo& c = kControlInfo[static_cast<std::size_t>(index)];
    n = n == n ? Clamp(n, 0.0, 1.0) : 0.0;
    switch (c.curve) {
    case Curve::kStepped: return std::floor(n * (c.steps - 1) + 0.5);
    case Curve::kLog: return c.min * std::pow(c.max / c.min, n);
    case Curve::kLinear: break;
    }
    return c.min + (c.max - c.min) * n;
}

inline double ToNormalized(int index, double value) {
    const ControlInfo& c = kControlInfo[static_cast<std::size_t>(index)];
    value = value == value ? Clamp(value, c.min, c.max) : c.initial;
    if (c.curve == Curve::kLog) return std::log(value / c.min) / std::log(c.max / c.min);
    return (value - c.min) / (c.max - c.min);
}

// Physical values of every control, in Control order.
struct Controls {
    std::array<double, kControlCount> value {};
    Controls() {
        for (int i = 0; i < kControlCount; ++i) value[static_cast<std::size_t>(i)] = kControlInfo[static_cast<std::size_t>(i)].initial;
    }
    double& operator[](int index) { return value[static_cast<std::size_t>(index)]; }
    double operator[](int index) const { return value[static_cast<std::size_t>(index)]; }
};

} // namespace respira
