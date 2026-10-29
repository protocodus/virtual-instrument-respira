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
    // mechanics. Each grille is its voiced value scaled as the default was
    // refitted, from 9 kHz to 1 kHz (RespiraControls.h).
    std::array<double, kControlCount> values;

    Controls ToControls() const {
        Controls c;
        c.value = values;
        return c;
    }
};

inline constexpr std::array<Preset, 8> kPresets { {
    { "Clarinet Solo", { 0, 22, 2, 0.5, 1000, 0.6, -2, 0.3, 0, 0, 25, 0, 10 } },
    { "Bassoon Low", { 1, 22, 2, 0.5, 667, 0.5, 0, 0.2, 0, 0, 25, 0, 10 } },
    { "Violin Folk", { 2, 14, 2, 0.5, 1000, 0.6, -3, 0.7, 0, 0, 25, 0, 10 } },
    { "Paris Musette", { 3, 28, 2, 0.5, 1111, 0.7, -3, 0.6, 0, 0, 25, 0, 10 } },
    { "Tango Bandoneon", { 4, 22, 2, 0.5, 722, 0.5, 2, 0.1, 0, 0, 25, 0, 10 } },
    { "Organ Bellows", { 5, 22, 2, 0.5, 889, 0.6, -2, 1.5, 0, 0, 25, 0, 10 } },
    { "Bright Oboe", { 6, 22, 2, 0.5, 1556, 0.6, -4, 0.7, 0, 0, 25, 0, 10 } },
    { "Master Tutti", { 7, 18, 2, 0.6, 1000, 0.8, 0, -1.2, 0, 0, 25, 0, 10 } },
} };

} // namespace respira
