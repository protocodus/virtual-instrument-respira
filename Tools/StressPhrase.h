#pragma once

// Respira's stress phrase for the CPU measurement (U12). The portfolio's
// default rings its chords under CC 64, which is Respira's air button: there
// the sixteen keys played into vented bellows and the reeds hardly sounded.
// This holds the same keys down instead, on Auto's bellows as a freshly
// loaded preset plays them, and turns the bellows twice with the keyswitches,
// the second time under all sixteen keys: a turn restarts every reed on its
// other tongue and lands every valve and fold, the instrument's dearest
// moment.

#include <cstdint>
#include <vector>

namespace respira {

struct StressEvent {
    double seconds;
    std::uint8_t bytes[3];
};

inline std::vector<StressEvent> StressPhrase() {
    std::vector<StressEvent> events;
    const auto add = [&](double seconds, int status, int data1, int data2) {
        events.push_back({ seconds, { static_cast<std::uint8_t>(status), static_cast<std::uint8_t>(data1),
                                      static_cast<std::uint8_t>(data2) } });
    };
    const auto key = [](int k) { return 48 + (36 * k + 3) / 7; };   // eight keys over C3-C6, as the default's
    const int pullSwitch = 12, pushSwitch = 13;                     // C0, C#0: Auto's keyswitches
    for (int k = 0; k < 8; ++k) add(0.0, 0x90, key(k), 127);
    for (int k = 0; k < 8; ++k) {   // each key struck again, 50 ms apart
        add(1.5 + 0.05 * k, 0x80, key(k), 0);
        add(1.5 + 0.05 * k, 0x90, key(k), 110);
    }
    add(2.0, 0x90, pushSwitch, 100);   // Auto's first stroke pulls
    add(2.1, 0x80, pushSwitch, 0);
    for (int k = 0; k < 8; ++k) add(2.5, 0x90, key(k) + 2, 120);   // a second chord a tone higher on top
    add(3.0, 0x90, pullSwitch, 100);
    add(3.1, 0x80, pullSwitch, 0);
    for (int k = 0; k < 8; ++k) {
        add(4.0, 0x80, key(k), 0);
        add(4.0, 0x80, key(k) + 2, 0);
    }
    return events;
}

} // namespace respira
