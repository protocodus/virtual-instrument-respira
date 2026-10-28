#pragma once

// The mechanism's own sound (owner decision, 2026-10-28): a key or button
// landing on its felt as it goes down, a pallet landing on its seat as the
// spring closes it, a register switch, the air button. Each is a collision:
// a part of effective mass m meets its stop at speed v and the stop's felt
// brings it to rest, and the force it passes to the instrument is what
// radiates. No oscillator and no envelope: the felt's law shapes every knock,
// so a faster key lands harder, shorter and brighter.

#include "DspMath.h"

#include <cmath>

namespace respira {

// A felt or cloth stop's force (N) at compression x (m), compressed at speed
// v (m/s): (a x^3 + b x^2 + c x)(1 + d v), never pulling. Least-squares fits
// to loading curves measured on a grand piano action's stops (M. Hirschkorn,
// "Dynamic model of a piano action mechanism", MASc thesis, University of
// Waterloo 2004, Table A.8, contacts 2 and 3). An accordion's key and pallet
// stops are felt and leather of the same kind; theirs are not measured.
struct Felt {
    double a, b, c, d;
    double Force(double x, double v) const {
        if (x <= 0.0) return 0.0;
        return Max(0.0, ((a * x + b) * x + c) * x * (1.0 + d * v));
    }
};
inline constexpr Felt kKeyFelt { 4.41e10, -1.02e7, 1.57e4, 1.219 };    // under the key front, where a pressed key lands
inline constexpr Felt kRestCloth { 2.42e10, -1.11e7, 6.54e3, 2.502 };  // the back rail cloth a released key returns onto

// A moving part of the mechanism and the stop it lands on. Masses and travels
// are estimates from typical sizes, not measurements.
struct Stop {
    double mass;      // effective mass at the stop, kg
    double travel;    // its travel to the stop, m: the speed it lands at is travel / stroke time
    const Felt* felt;
};
// A treble key and the fingertip riding on it, at the key front: 5 mm of dip,
// as reported for a Scandalli Air.
inline constexpr Stop kTrebleKey { 0.020, 0.005, &kKeyFelt };
// The key and its pallet, at the pallet: the 5 mm lift the pallet holes
// assume (RespiraEngine.cpp, kHalfPressed: lifted 1 mm, open 0.2).
inline constexpr Stop kTreblePallet { 0.010, 0.005, &kRestCloth };
// A bass, chord or air button and its rod: about 3 mm of travel, the
// thickness of a button top, as repairers give it.
inline constexpr Stop kButton { 0.015, 0.003, &kKeyFelt };
inline constexpr Stop kBassPallet { 0.015, 0.005, &kRestCloth };   // a bass pallet or the air button's pad
// A register switch and the slides it moves, pressed over the time a slide
// takes to open (the softest key stroke's).
inline constexpr Stop kRegisterSwitch { 0.030, 0.005, &kKeyFelt };

// The mechanism radiates as the key does: a compact body of the key's size
// (a strip 20 mm wide moving over 100 mm, whose added mass is rho pi (w/2)^2 L)
// accelerated by the contact force, its dipole sound averaged over directions
// (1 / sqrt 3), at 1 m: p = (pi (w/2)^2 L / m) rho / (4 pi c sqrt 3) dF/dt.
// Every part radiates through this one coupling, as the force reaches the air
// through the same keyboard and body, whose own radiation is not modelled; so
// it sets the level of the whole mechanism, and Mechanics is in dB re it. In
// the engine's units (radiated flow's rate of change, m^3/s^2, which
// rho / (4 pi) turns into Pa at 1 m), per N/s of force change.
inline constexpr double kForceToFlow = 3.14159265358979 * 0.01 * 0.01 * 0.1 / (kTrebleKey.mass * 343.0 * 1.7320508075688772);

// The body radiates that force only above its lowest panel modes: a real
// accordion's mechanism sound spans about 210 Hz to 4 kHz (K. Niewczas,
// "Nagłośnienie i rejestracja akordeonu", Akademia Muzyczna w Łodzi 2019,
// Fig. 50, DOI 10.5281/zenodo.20128891). A second-order Butterworth high-pass
// at 210 Hz (bilinear, prewarped), in transposed direct form II.
struct BodyHighPass {
    static constexpr double kHz = 210.0;
    double b0 = 1.0, a1 = 0.0, a2 = 0.0, z1 = 0.0, z2 = 0.0;
    void Set(double rate) {
        const double k = std::tan(3.14159265358979 * kHz / rate), norm = 1.0 / (1.0 + 1.4142135623730951 * k + k * k);
        b0 = norm;
        a1 = 2.0 * (k * k - 1.0) * norm;
        a2 = (1.0 - 1.4142135623730951 * k + k * k) * norm;
    }
    void Clear() { z1 = z2 = 0.0; }
    void FlushTiny() {
        if (Abs(z1) < 1.0e-20) z1 = 0.0;
        if (Abs(z2) < 1.0e-20) z2 = 0.0;
    }
    double Process(double x) {   // b1 = -2 b0, b2 = b0
        const double y = b0 * x + z1;
        z1 = -2.0 * b0 * x - a1 * y + z2;
        z2 = b0 * x - a2 * y;
        return y;
    }
};

// One collision, stepped at the host rate. Semi-implicit steps, four a sample,
// keep the stiffest landing (a hard key at 22 kHz) well inside stability.
struct Knock {
    static constexpr int kSteps = 4;
    bool active = false, bass = false;
    float panL = 0.0f, panR = 0.0f;
    double x = 0.0, v = 0.0, mass = 1.0, force = 0.0;
    const Felt* felt = &kKeyFelt;

    void Start(const Stop& stop, double speed, double massScale) {
        felt = stop.felt;
        mass = stop.mass * massScale;
        x = force = 0.0;
        v = speed;
        active = true;
    }
    // The change in contact force over one host sample, N. The part leaves
    // its felt (or comes to rest on it) once the felt has pushed it back out.
    double Step(double dt) {
        const double before = force, h = dt / kSteps;
        for (int i = 0; i < kSteps; ++i) {
            v -= felt->Force(x, v) / mass * h;
            x += v * h;
        }
        force = felt->Force(x, v);
        if (x <= 0.0 || (v <= 0.0 && force <= 0.0)) active = false;
        return force - before;
    }
};

} // namespace respira
