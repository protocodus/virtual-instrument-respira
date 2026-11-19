#pragma once

// The factory bank every front end and the demo renderer read: one preset per
// treble register, each voiced around what that register is played for, on
// the Default build, then one per other build (Type) in the register its kind
// of instrument is best known by.

#include "RespiraControls.h"

#include <array>

namespace respira {

struct Preset {
    const char* name;
    // Physical values in Control order: register, musette, bellows, assist,
    // grille, width, bass, volume, keyboard, air, momentum, oversampling,
    // mechanics, auto, room, life, touch, type. Each grille is its voiced value scaled as the default was
    // refitted, from 9 kHz to 1 kHz (RespiraControls.h). Each volume keeps the
    // preset within 1 dB of Init on the reference stroke (PresetsSitTogether).
    std::array<double, kControlCount> values;

    Controls ToControls() const {
        Controls c;
        c.value = values;
        return c;
    }
};

inline constexpr std::array<Preset, 24> kPresets { {
    { "Clarinet Solo", { 0, 22, 2, 0.5, 1000, 0.6, -2, 0.1, 0, 0, 25, 0, 5.6, 1, 0.005, 0.5, 1, 0 } },
    { "Bassoon Low", { 1, 22, 2, 0.5, 667, 0.5, 0, 0, 0, 0, 25, 0, 5.6, 1, 0.005, 0.5, 1, 0 } },
    { "Violin Folk", { 2, 14, 2, 0.5, 1000, 0.6, -3, 0.5, 0, 0, 25, 0, 5.6, 1, 0.005, 0.5, 1, 0 } },
    { "Paris Musette", { 3, 28, 2, 0.5, 1111, 0.7, -3, 0.5, 0, 0, 25, 0, 5.6, 1, 0.005, 0.5, 1, 0 } },
    { "Tango Bandoneon", { 4, 22, 2, 0.5, 722, 0.5, 2, -0.1, 0, 0, 25, 0, 5.6, 1, 0.005, 0.5, 1, 0 } },
    { "Organ Bellows", { 5, 22, 2, 0.5, 889, 0.6, -2, 1.3, 0, 0, 25, 0, 5.6, 1, 0.005, 0.5, 1, 0 } },
    { "Bright Oboe", { 6, 22, 2, 0.5, 1556, 0.6, -4, 0.4, 0, 0, 25, 0, 5.6, 1, 0.005, 0.5, 1, 0 } },
    { "Master Tutti", { 7, 18, 2, 0.6, 1000, 0.8, 0, -1.3, 0, 0, 25, 0, 5.6, 1, 0.005, 0.5, 1, 0 } },
    { "Zinc Bandoneon", { 4, 22, 2, 0.5, 722, 0.5, 2, -0.1, 0, 0, 25, 0, 5.6, 1, 0.005, 0.5, 1, 1 } },
    { "Irish Button Box", { 3, 22, 2, 0.5, 1111, 0.7, -3, 0.5, 0, 0, 25, 0, 5.6, 1, 0.005, 0.5, 1, 2 } },
    { "German Club", { 2, 22, 2, 0.5, 1000, 0.6, -3, 0.5, 0, 0, 25, 0, 5.6, 1, 0.005, 0.5, 1, 3 } },
    { "French Musette", { 3, 22, 2, 0.5, 1111, 0.7, -3, 0.5, 0, 0, 25, 0, 5.6, 1, 0.005, 0.5, 1, 4 } },
    { "Jazz Cassotto", { 4, 22, 2, 0.5, 1000, 0.6, -2, 0.0, 0, 0, 25, 0, 5.6, 1, 0.005, 0.5, 1, 5 } },
    { "Balkan Kolo", { 7, 22, 2, 0.6, 1000, 0.8, 0, -1.3, 0, 0, 25, 0, 5.6, 1, 0.005, 0.5, 1, 6 } },
    { "Italian Liscio", { 3, 22, 2, 0.5, 1111, 0.7, -3, 0.5, 0, 0, 25, 0, 5.6, 1, 0.005, 0.5, 1, 7 } },
    { "Finnish Tango", { 3, 22, 2, 0.5, 1111, 0.7, -3, 0.5, 0, 0, 25, 0, 5.6, 1, 0.005, 0.5, 1, 8 } },
    { "Bandoneon Solo", { 0, 22, 2, 0.5, 722, 0.5, 2, 0.0, 0, 0, 25, 0, 5.6, 1, 0.005, 0.5, 1, 1 } },
    { "Irish Session", { 2, 22, 2, 0.5, 1111, 0.7, -3, 0.5, 0, 0, 25, 0, 5.6, 1, 0.005, 0.5, 1, 2 } },
    { "German Master", { 7, 22, 2, 0.6, 1000, 0.8, 0, -1.3, 0, 0, 25, 0, 5.6, 1, 0.005, 0.5, 1, 3 } },
    { "Old Paris Musette", { 3, 30, 2, 0.5, 1111, 0.7, -3, 0.5, 0, 0, 25, 0, 5.6, 1, 0.005, 0.5, 1, 4 } },
    { "Jazz Clarinet", { 0, 22, 2, 0.5, 1000, 0.6, -2, 0.0, 0, 0, 25, 0, 5.6, 1, 0.005, 0.5, 1, 5 } },
    { "Balkan Musette", { 3, 22, 2, 0.5, 1111, 0.7, -3, 0.5, 0, 0, 25, 0, 5.6, 1, 0.005, 0.5, 1, 6 } },
    { "Italian Master", { 7, 22, 2, 0.6, 1000, 0.8, 0, -1.3, 0, 0, 25, 0, 5.6, 1, 0.005, 0.5, 1, 7 } },
    { "Finnish Waltz", { 2, 22, 2, 0.5, 1000, 0.6, -3, 0.5, 0, 0, 25, 0, 5.6, 1, 0.005, 0.5, 1, 8 } },
} };

} // namespace respira
