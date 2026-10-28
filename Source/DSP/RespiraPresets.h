#pragma once

// The factory bank every front end and the demo renderer read: one preset per
// treble register, each voiced around what that register is played for.

#include "RespiraControls.h"

#include <array>

namespace respira {

struct Preset {
    const char* name;
    // Physical values in Control order: register, musette, bellows, assist,
    // grille, width, bass, volume, keyboard, air, momentum, oversampling,
    // mechanics.
    std::array<double, kControlCount> values;

    Controls ToControls() const {
        Controls c;
        c.value = values;
        return c;
    }
};

inline constexpr std::array<Preset, 8> kPresets { {
    { "Clarinet Solo", { 0, 22, 2, 0.5, 9000, 0.6, -2, 0.3, 0, 0, 25, 0, 0 } },
    { "Bassoon Low", { 1, 22, 2, 0.5, 6000, 0.5, 0, 0.2, 0, 0, 25, 0, 0 } },
    { "Violin Folk", { 2, 14, 2, 0.5, 9000, 0.6, -3, 0.7, 0, 0, 25, 0, 0 } },
    { "Paris Musette", { 3, 28, 2, 0.5, 10000, 0.7, -3, 0.6, 0, 0, 25, 0, 0 } },
    { "Tango Bandoneon", { 4, 22, 2, 0.5, 6500, 0.5, 2, 0.1, 0, 0, 25, 0, 0 } },
    { "Organ Bellows", { 5, 22, 2, 0.5, 8000, 0.6, -2, 1.5, 0, 0, 25, 0, 0 } },
    { "Bright Oboe", { 6, 22, 2, 0.5, 14000, 0.6, -4, 0.7, 0, 0, 25, 0, 0 } },
    { "Master Tutti", { 7, 18, 2, 0.6, 9000, 0.8, 0, -1.2, 0, 0, 25, 0, 0 } },
} };

} // namespace respira
