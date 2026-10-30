#pragma once

// The mechanism's own sound (owner decision, 2026-10-28): a key or button
// landing on its felt as it goes down, a pallet landing on its seat as the
// spring closes it, a register switch, the air button. Each is a collision:
// a part of effective mass m meets its stop at speed v and the stop's felt
// brings it to rest, and the force it passes to the instrument is what
// radiates. No oscillator and no envelope: the felt's law shapes every knock,
// so a faster key lands harder, shorter and brighter.

#include "DspMath.h"

#include <array>
#include <cmath>
#include <cstdint>

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
// The key and its pallet, at the pallet: taken to lift as far as the key
// front dips.
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

// The bellows' folds. Each fold's walls are card panels in cloth, hinged at
// the fold's edges by cloth and leather with a little play: pressing, the air
// bows every panel out until its hinges draw taut; drawing, it sucks them in.
// When the bellows turn and the pressure changes sign, every panel crosses
// that play and lands on its hinges the other way: the rustle of a reversal,
// harder the faster the pressure turns. Between turns they rest on their
// hinges and are silent. A full-size bellows of 18 folds, 4 walls and two
// panels a fold in each wall holds 144 panels, 35 mm deep (the fold's depth)
// and 410 or 190 mm long (the long and short walls); card and cloth weigh
// about 1 kg/m^2; the play is taken as 0.5 mm at a panel's middle. All of
// these are estimates from typical sizes, not measurements, and the play sets
// how hard they land: from none (silent) to 1 mm they span a turn as quiet as
// nothing to one as loud as the chord. So the folds' level is voiced by ear
// (kFoldVoicingDb, the owner's choice), not derived. The hinge stands on the
// back rail cloth's law, as a pallet's leather does.
inline constexpr int kPleats = 144;
inline constexpr double kPleatDepth = 0.035, kPleatLongWall = 0.41, kPleatShortWall = 0.19;
inline constexpr double kPleatSurfaceMass = 1.0, kPleatPlay = 0.0005;
inline constexpr double kPleatHold = 1.0;   // Pa: the cloth's friction within the play, an estimate
// A panel radiates as a key does (kForceToFlow), a strip of its own size
// driven by the force on it: pi (w/2)^2 L / m with m = rho_s w L, the same for
// every panel whatever its length, against the key's.
inline constexpr double kPleatToFlow = 3.14159265358979 * kPleatDepth / 4.0
                                       / (kPleatSurfaceMass * 343.0 * 1.7320508075688772);

struct Pleat {
    double x = 0.0, v = 0.0, contact = 0.0;   // from the middle of its play, m; the hinge's force, N
    double mass = 0.0, area = 0.0, play = kPleatPlay;
    bool moving = false;

    // The change in the hinges' force over one host sample, N, under bellows
    // pressure p (Pa, positive pressing: outwards). A panel that has come to
    // rest on its hinge sleeps there, the hinge bearing the load as it eases,
    // until the pressure pulls the panel the other way; it then leaves from
    // the compression that load held. Within its play the cloth's friction
    // slows it and holds it still until the pressure on it passes
    // kPleatHold, so with the bellows vented the panels settle where they are.
    double Step(double p, double dt) {
        const double load = p * area, hold = kPleatHold * area;
        if (!moving) {
            const bool loose = Abs(x) <= play;
            if (loose ? Abs(load) <= hold : load * x >= 0.0) {
                contact = loose ? 0.0 : load;   // held still, so radiating nothing
                return 0.0;
            }
            if (loose) contact = 0.0;
            else x = (x > 0.0 ? 1.0 : -1.0) * (play + Compression(Abs(contact)));
            moving = true;
        }
        const double before = contact, h = dt / Knock::kSteps, slow = hold / mass * h;
        for (int i = 0; i < Knock::kSteps; ++i) {
            v += (load - Hinge(x, v)) / mass * h;
            if (Abs(x) <= play) v = Abs(v) <= slow ? 0.0 : v - (v > 0.0 ? slow : -slow);
            x += v * h;
        }
        contact = Hinge(x, v);
        const bool loose = Abs(x) <= play;
        if (Abs(v) < 1.0e-4 && (loose ? Abs(load) <= hold : Abs(contact - load) < 0.02 * Abs(load))) {
            moving = false;
            v = 0.0;
        }
        return contact - before;
    }
    // The hinge's compression under a still force: Newton's method on the
    // cloth's cubic, which rises everywhere (3 a c^2 + 2 b c + c' > 0).
    static double Compression(double force) {
        const Felt& f = kRestCloth;
        double c = 0.0;
        for (int i = 0; i < 12; ++i) c -= (((f.a * c + f.b) * c + f.c) * c - force) / ((3.0 * f.a * c + 2.0 * f.b) * c + f.c);
        return Max(c, 0.0);
    }
    double Hinge(double at, double speed) const {
        if (at > play) return kRestCloth.Force(at - play, speed);
        if (at < -play) return -kRestCloth.Force(-play - at, -speed);
        return 0.0;
    }
};

// All the panels. A resting panel only waits for the pressure to turn
// against the side it rests on, so the resting ones are kept in sets by that
// side, and only the moving ones and those the pressure turns against are
// stepped: in a phrase a few panels move at a time. A resting panel's contact
// (the load it bears) is brought up to date only when it is stepped, from the
// pressure of the sample before, so every panel moves to the bit as stepping
// all of them every sample moved it.
struct Folds {
    static constexpr int kWords = (kPleats + 63) / 64;
    std::array<Pleat, kPleats> pleats {};
    std::uint64_t moving[kWords] {}, out[kWords] {}, in[kWords] {};   // resting outwards, inwards
    std::uint64_t fresh[kWords] {};   // came to rest last sample, still holding the hinge's force
    double last = 0.0;                // the pressure the sample before

    // The panels as set up, all resting in the middle of their play.
    void Rest() {
        for (int w = 0; w < kWords; ++w) moving[w] = out[w] = in[w] = fresh[w] = 0;
        last = 0.0;
    }

    // The change in all the hinges' force over one host sample, N, as the sum
    // of Pleat::Step over the panels in order.
    double Step(double p, double dt) {
        // Under 1e-200 Pa a resting panel's load times its place could round
        // to zero, which Pleat::Step reads as resting; every one is asked then.
        const bool tiny = p != 0.0 && Abs(p) < 1.0e-200;
        // The panels to step, in order: listed first, so that stepping them
        // is one plain loop.
        std::uint8_t list[kPleats];
        int count = 0;
        for (int w = 0; w < kWords; ++w) {
            const int size = kPleats - 64 * w < 64 ? kPleats - 64 * w : 64;
            const std::uint64_t all = size == 64 ? ~std::uint64_t(0) : (std::uint64_t(1) << size) - 1;
            const std::uint64_t resting = all & ~moving[w];
            // Those within their play (in neither set) move only above kPleatHold.
            const std::uint64_t held = Abs(p) > kPleatHold ? all : out[w] | in[w];
            const std::uint64_t against = p == 0.0 ? 0 : tiny ? resting : resting & ~(p > 0.0 ? out[w] : in[w]) & held;
            for (std::uint64_t step = moving[w] | against; step != 0; step &= step - 1)
                list[count++] = static_cast<std::uint8_t>(64 * w + LowestBit(step));
        }
        double flips = 0.0;
        for (int i = 0; i < count; ++i) {
            // A copy, which the compiler keeps in registers through the step.
            Pleat pleat = pleats[list[i]];
            const int w = list[i] / 64;
            const std::uint64_t bit = std::uint64_t(1) << (list[i] % 64);
            if (!pleat.moving && !(fresh[w] & bit)) pleat.contact = last * pleat.area;
            flips += pleat.Step(p, dt);
            pleats[list[i]] = pleat;
        }
        for (int w = 0; w < kWords; ++w) fresh[w] = 0;
        for (int i = 0; i < count; ++i) {
            const Pleat& pleat = pleats[list[i]];
            const int w = list[i] / 64;
            const std::uint64_t bit = std::uint64_t(1) << (list[i] % 64);
            const bool was = moving[w] & bit;
            out[w] &= ~bit;
            in[w] &= ~bit;
            moving[w] &= ~bit;
            if (pleat.moving) moving[w] |= bit;
            else {
                // One resting within its play is in neither set: any
                // pressure may move it, and Pleat::Step holds it below kPleatHold.
                if (pleat.x > pleat.play) out[w] |= bit;
                else if (pleat.x < -pleat.play) in[w] |= bit;
                if (was) fresh[w] |= bit;
            }
        }
        last = p;
        return flips;
    }
};

// The folds' leather and cloth rub as the bellows move: friction noise, its
// force growing with the bellows end's speed (the air leaving the bellows over
// their section, about 0.41 by 0.19 m). Nothing measured sets its level or
// spectrum, so both are voiced (kRustleVoicingDb, the owner's choice): white
// noise in N per m/s, through the body's 210 Hz high-pass and a one-pole
// low-pass at 2 kHz, the band a soft material's rubbing fills. Below 1 mm/s
// (an estimate) the folds stick rather than slide: so the bellows' own leak,
// which creeps a closed bellows at well under that, rubs nothing, and a
// device left with its bellows pressed falls silent.
inline constexpr double kBellowsSection = 0.41 * 0.19, kFoldsStick = 0.001;
struct Rustle {
    std::uint32_t seed = 0x2545F491u;
    double low = 0.0, lowCoeff = 0.0;
    BodyHighPass body;
    void Set(double rate) {
        lowCoeff = 1.0 - std::exp(-2.0 * 3.14159265358979 * 2000.0 / rate);
        body.Set(rate);
    }
    void Clear() {
        low = 0.0;
        body.Clear();
    }
    // Force (N) at bellows end speed v (m/s).
    double Step(double v) {
        const double sliding = Max(Abs(v) - kFoldsStick, 0.0);
        if (sliding == 0.0 && Abs(low) < 1.0e-20) {
            Clear();
            return 0.0;
        }
        seed = seed * 1664525u + 1013904223u;
        const double draw = 1.7320508075688772 * ((seed >> 8) / double(1u << 24) * 2.0 - 1.0);
        low += lowCoeff * (body.Process(draw * sliding) - low);
        return low;
    }
};

} // namespace respira
