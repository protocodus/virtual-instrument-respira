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

// The body radiates only above its lowest panel modes: a real accordion's
// mechanism sound spans about 210 Hz to 4 kHz (K. Niewczas, "Nagłośnienie i
// rejestracja akordeonu", Akademia Muzyczna w Łodzi 2019, Fig. 50, DOI
// 10.5281/zenodo.20128891). The folds' rubbing (Rustle) reaches the air
// through this: a second-order Butterworth high-pass at 210 Hz followed by two
// parallel bilinear-prewarped modal resonators, in transposed direct form II,
// voiced as the keybed/soundboard (~850 Hz) and the key-rod/grille frame
// (~2.4 kHz). The knocks ring the panel they land on instead (KeyBed).
struct BodyHighPass {
    static constexpr double kHz = 210.0;
    struct Mode {
        double b0 = 0.0, a1 = 0.0, a2 = 0.0, z1 = 0.0, z2 = 0.0;
        void Set(double hz, double q, double gain, double rate) {
            const double k = std::tan(3.14159265358979 * hz / rate);
            const double norm = 1.0 / (1.0 + k / q + k * k);
            b0 = gain * (k / q) * norm;
            a1 = 2.0 * (k * k - 1.0) * norm;
            a2 = (1.0 - k / q + k * k) * norm;
        }
        void Clear() { z1 = z2 = 0.0; }
        bool Tiny() const { return Abs(z1) < 1.0e-30 && Abs(z2) < 1.0e-30; }
        double Process(double x) {
            const double y = b0 * x + z1;
            z1 = -a1 * y + z2;
            z2 = -b0 * x - a2 * y;
            return y;
        }
    };
    double b0 = 1.0, a1 = 0.0, a2 = 0.0, z1 = 0.0, z2 = 0.0;
    Mode wood, metal;
    // The bass mechanism sits in the deeper enclosed wooden bass casing with
    // heavy steel Stradella pistons and clevises (~640 Hz and ~1.85 kHz),
    // whereas the treble keybed and aluminum grille rods ring higher (~850 Hz
    // and ~2.4 kHz).
    void Set(double rate, bool bassSide = false) {
        const double k = std::tan(3.14159265358979 * kHz / rate), norm = 1.0 / (1.0 + 1.4142135623730951 * k + k * k);
        b0 = norm;
        a1 = 2.0 * (k * k - 1.0) * norm;
        a2 = (1.0 - 1.4142135623730951 * k + k * k) * norm;
        wood.Set(bassSide ? 640.0 : 850.0, bassSide ? 4.0 : 4.5, bassSide ? 0.38 : 0.35, rate);
        metal.Set(bassSide ? 1850.0 : 2400.0, bassSide ? 5.0 : 5.5, bassSide ? 0.35 : 0.40, rate);
    }
    void Clear() {
        z1 = z2 = 0.0;
        wood.Clear();
        metal.Clear();
    }
    void FlushTiny() {
        if (Abs(z1) < 1.0e-30 && Abs(z2) < 1.0e-30 && wood.Tiny() && metal.Tiny()) Clear();
    }
    double Process(double x) {   // b1 = -2 b0, b2 = b0
        const double y = b0 * x + z1;
        z1 = -2.0 * b0 * x - a1 * y + z2;
        z2 = b0 * x - a2 * y;
        return y + wood.Process(y) + metal.Process(y);
    }
};

// No two strokes of a finger land alike. Expert pianists striking one key
// again and again at a target of MIDI velocity 75 scatter it by 4.1 (SD,
// 1.2-7.5 per player; Tominaga et al., "Kinematic origins of motor
// inconsistency in expert pianists", PLoS One 11(8), 2016, Results), 5.5 %
// of the target. Each stroke's velocity scatters by this much at Life 0.5
// (ImperfectionScale 1), none at Life 0: through KeyOpeningSeconds the key's
// descent, the pallets it lifts and so its note's attack, and its landing on
// the felt; a register switch's and the air button's press and release, and so
// their slides' and valve's speed, directly. Applied as well to a key's return
// as the finger lets it up, so to how its note's pallets close, which no
// measurement covers: the finger rides the key part of the way, by an amount
// that differs each time.
inline constexpr double kStrokeScatter = 0.055;

// The most bending modes a key bed rings with (KeyBed).
inline constexpr int kKeyBedModes = 10;

// One collision, stepped at the host rate. Semi-implicit steps, four a sample,
// keep the stiffest landing (a hard key at 22 kHz) well inside stability.
struct Knock {
    static constexpr int kSteps = 4;
    bool active = false, bass = false;
    int wait = 0;   // host samples still in flight before the part lands
    double x = 0.0, v = 0.0, mass = 1.0, force = 0.0;
    double coupling[kKeyBedModes] {};   // how hard it drives each of its key bed's modes, from where it lands
    const Felt* felt = &kKeyFelt;

    void Start(const Stop& stop, double speed, double massScale, int flightSamples = 0) {
        felt = stop.felt;
        mass = stop.mass * massScale;
        x = force = 0.0;
        v = speed;
        wait = flightSamples;
        active = true;
    }
    // The change in contact force over one host sample, N. The part leaves
    // its felt (or comes to rest on it) once the felt has pushed it back out.
    double Step(double dt) {
        if (wait > 0) {
            --wait;
            return 0.0;
        }
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

// The panel a part lands on, which rings and radiates its knock: the treble
// keyboard's panel, or the bass side's button board. A bare dipole of the key
// (kForceToFlow alone) radiated each landing as one cycle of about 500 Hz
// that started with a jump, which Reason Studios' reviewers and the owner
// heard as a pop (2026-11-09); a part landing on a wooden instrument rings
// its panel. Each is a simply supported plywood plate, its bending modes
// f = (pi/2) sqrt(D / rho h) ((m/a)^2 + (n/b)^2), D = E h^3 / (12 (1 - nu^2))
// (A. W. Leissa, "Vibration of Plates", NASA SP-160, 1969, ch. 4). The parts
// land along its middle, which leaves the n = 2 modes still, and only n = 1
// is kept: n = 3 starts above 1.5 kHz, where little of a landing's force is.
// It is as deep (b) as puts its lowest mode at 210 Hz, the lower edge of the
// band above, chosen by ear over a shallower panel's 500 Hz (the owner,
// 2026-11-09), and rings every mode to 4 kHz, the band's upper edge. A part
// landing at x along it (0..1) drives mode m by sin(m pi x). Plywood 4 mm,
// 8 GPa and 650 kg/m^3, the panels 0.42 and 0.30 m long and a loss factor of
// 0.04 (Q 25, a glued wooden box under felt and leather) are estimates from
// typical instruments, not measurements, as the masses and travels are.
//
// The landing's force reaches the panel through key, guide pin, felt and the
// player's hold, never twice alike: each landing moves every mode by up to
// kKeyBedScatter (uniform, scaled by Life as the reeds' imperfections are),
// the owner's requirement that no two knocks sound like one sample played
// (2026-11-09) and the 60-75 % sameness the owner accepted: eight landings
// of one key at one velocity then match about 72 % as it lands and 68 % as
// its pallet shuts (median waveform correlation, Life 50 %; 52 % and 41 % at
// Life 100 %), where they matched 98-100 %. Nothing measured sets that
// spread; at Life 0 every landing rings alike.
inline constexpr double kKeyBedScatter = 0.04;
// Each side's level, so a landing carries the energy the dipole estimate did
// (kForceToFlow, through BodyHighPass, which Mechanics' default was chosen
// by ear against): a treble key or bass button struck in kKeyBedStroke and
// averaged over the panel's places, at 48 kHz (Respira.Engine,
// MechanicsKnock, checks it from 44.1 to 96 kHz).
inline constexpr double kKeyBedStroke = 0.012;
inline constexpr double kKeyBedTrebleLevel = 3.128, kKeyBedBassLevel = 4.153;

struct KeyBed {
    static constexpr double kTop = 4000.0, kQ = 25.0;
    BodyHighPass::Mode modes[kKeyBedModes];
    double hz[kKeyBedModes] {};
    int count = 0;
    double level = 1.0, rate = 48000.0;
    // Each panel draws its own landings' scatter, so what lands on one never
    // changes how the other rings.
    std::uint32_t luck = 1u;
    bool quiet = true;   // every mode at rest: nothing to step until a part lands

    void Set(double hostRate, bool bassSide) {
        rate = hostRate;
        level = bassSide ? kKeyBedBassLevel : kKeyBedTrebleLevel;
        luck = bassSide ? 2u : 1u;
        const double length = bassSide ? 0.30 : 0.42, h = 0.004;
        const double stiffness = 8.0e9 * h * h * h / (12.0 * (1.0 - 0.3 * 0.3));
        const double c = 1.5707963267948966 * std::sqrt(stiffness / (650.0 * h));
        const double across = BodyHighPass::kHz / c - 1.0 / (length * length);   // (1/b)^2
        count = 0;
        for (int m = 1; m <= kKeyBedModes; ++m) {
            const double f = c * (m * m / (length * length) + across);
            if (f > kTop || f > 0.45 * rate) break;
            hz[count++] = f;
        }
        for (int m = 0; m < count; ++m) modes[m].Set(hz[m], kQ, 1.0, rate);
        Clear();
    }
    // A part landing at `place` (0..1 along the panel) drives mode m by
    // sin((m + 1) pi place), by the sines' recurrence.
    static void Couple(double place, double* coupling) {
        const double angle = 3.14159265358979 * place, twice = 2.0 * std::cos(angle);
        double before = 0.0, now = std::sin(angle);
        for (int m = 0; m < kKeyBedModes; ++m) {
            coupling[m] = now;
            const double next = twice * now - before;
            before = now;
            now = next;
        }
    }
    // A landing moves every mode by up to `spread` of itself (kKeyBedScatter
    // times Life's scale), but never out of the band the mechanism's sound is
    // measured in; a ringing mode keeps its state.
    void Scatter(double spread) {
        if (spread <= 0.0) return;
        for (int m = 0; m < count; ++m) {
            luck = luck * 1664525u + 1013904223u;
            const double z1 = modes[m].z1, z2 = modes[m].z2;
            const double moved = hz[m] * (1.0 + spread * ((luck >> 8) / double(1u << 24) * 2.0 - 1.0));
            modes[m].Set(Clamp(moved, BodyHighPass::kHz, kTop), kQ, 1.0, rate);
            modes[m].z1 = z1;
            modes[m].z2 = z2;
        }
    }
    void Clear() {
        for (auto& mode : modes) mode.Clear();
        quiet = true;
    }
    // At a block's end: a panel that has rung out sleeps.
    void FlushTiny() {
        if (quiet) return;
        for (int m = 0; m < count; ++m)
            if (!modes[m].Tiny()) return;
        Clear();
    }
    // The pressure's rate of change it radiates, in kForceToFlow's units, with
    // `drive[m]` the force change on mode m this sample (N/s).
    double Process(const double* drive, bool driven) {
        if (quiet && !driven) return 0.0;
        quiet = false;
        double out = 0.0;
        for (int m = 0; m < count; ++m) out += modes[m].Process(drive[m]);
        return level * out;
    }
};

// A push reed's valve: the leather or plastic flap over its slot on the
// chamber's side of the plate, which the push air lifts and which shuts the
// slot to the pull. Repairers hear it slap shut as the bellows turn and as a
// note ends, loudest on the low reeds (accordionists.info, "low reed valve
// noise", 2009). The flap, of surface density kFlapDensity, covers the slot
// with kFlapMargin to spare round the tongue, and lands on the plate on
// leather (the back rail cloth's law, as a pallet's). Moving, it displaces
// the chamber's air, a source of A v, so landing it radiates A dv/dt = F /
// kFlapDensity through the pallet hole, as the reeds do. As the bellows turn,
// the drop across it reverses at R Pa/s and, starting from rest at its lift
// d, drives it shut in t = (6 kFlapDensity d / R)^(1/3), landing at
// R t^2 / (2 kFlapDensity): harder the faster the bellows turn. As a pushed
// note ends, its own curl shuts it as the chamber fills, at d times its
// natural frequency, its curl lifting it d at kFlapOpenPa. Either way it
// lands its flight time after it lets go, not as it lets go: a quarter of its
// natural period as its curl shuts it (10 ms), t as the bellows turn (about
// 5 ms). Each valve's lift and curl differ from its neighbours' by up to
// kFlapSpread, fixed per instrument as the keys' and the folds' panels are,
// so a chord's valves land one after another over a few milliseconds, not
// on one sample with one speed. The pull reed's
// valve is on the bellows' side and is not heard. The flutter the reeds'
// pulses shake into an open valve is left out, so their tone stays as it was
// fitted. All of these are estimates from typical valves, not measurements,
// so the slaps' level is voiced (kValveVoicing), under Mechanics with the
// rest of the mechanism.
inline constexpr double kFlapDensity = 0.4;     // kg/m^2: 0.4 mm of leather or 0.3 mm of plastic
inline constexpr double kFlapMargin = 0.003;    // m, across and along the slot
inline constexpr double kFlapLift = 0.0015;     // m, at the flap's middle
inline constexpr double kFlapOpenPa = 15.0;     // the drop that lifts it that far
// The drop past which the valve counts as lifted or reversed, Pa: the
// bellows' ripple and a still hand's drift do not move it.
inline constexpr double kFlapHold = 2.0;
// The pull reed's check valve sits on the bellows side of the reed plate: its
// slap must couple through the reed slot and chamber (or bellows walls) rather
// than directly into the chamber mouth, attenuating it relative to the push
// valve's slap.
inline constexpr double kPullValveShare = 0.35;

inline constexpr double kFlapSpread = 0.2;

inline double FlapArea(double length, double width) { return (length + kFlapMargin) * (width + kFlapMargin); }
// When (s after it lets go) and how fast (m/s) a valve lands.
struct FlapLanding {
    double delay, speed;
};
// At a turn whose drop reverses at `rate` Pa/s, from a lift of `lift` m.
inline FlapLanding FlapTurn(double rate, double lift = kFlapLift) {
    rate = Max(rate, 1.0);
    const double t = std::cbrt(6.0 * kFlapDensity * lift / rate);
    return { t, rate * t * t / (2.0 * kFlapDensity) };
}
inline double FlapTurnSpeed(double rate) { return FlapTurn(rate).speed; }
// As its curl shuts it: a quarter period of sqrt(stiffness / mass), landing
// at lift times that, its curl lifting it `lift` at `openPa`.
inline FlapLanding FlapCurl(double lift = kFlapLift, double openPa = kFlapOpenPa) {
    const double w = Sqrt(openPa / (lift * kFlapDensity));
    return { 1.5707963267948966 / w, lift * w };
}
inline constexpr double kFlapCurlSpeed = kFlapLift * 158.11388300841895;   // sqrt(kFlapOpenPa / (kFlapLift kFlapDensity))

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
// A panel's hinge is stepped at no more than this. Under the full kMaxPressure
// its cloth resonates near 0.5 kHz and damps at about 8 000 a second, so a step
// of 25 us (one host sample from 40 kHz up) is under a tenth of a radian of
// the resonance and a fifth of the damping time, well inside an explicit step's
// stability.
inline constexpr double kPleatMaxStep = 25.0e-6;
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
        const int steps = Steps(dt);
        return Step(p, steps, dt / steps);
    }
    // The hinge's steps in a host sample of dt.
    static int Steps(double dt) { return dt > kPleatMaxStep ? static_cast<int>(dt / kPleatMaxStep) + 1 : 1; }
    // Step(p, dt) in `steps` steps of h, as Folds takes them for every panel.
    double Step(double p, int steps, double h) {
        const double load = p * area, hold = kPleatHold * area;
        // A panel already moving has its hinge's force from the step before
        // at the same place and speed: contact is Hinge(x, v) to the bit.
        const bool wasMoving = moving;
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
        const double before = contact, slow = hold / mass * h;
        for (int i = 0; i < steps; ++i) {
            v += (load - (i == 0 && wasMoving ? contact : Hinge(x, v))) / mass * h;
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
    std::uint64_t unplaced[kWords] {};   // not yet anywhere in its play (Rest)
    double last = 0.0;                // the pressure the sample before

    // The panels as set up. Where a panel stands in its play before the first
    // stroke is not the model's to know, and a player's hold loads the bellows
    // the way of that stroke before a key opens: so the first pressure past
    // kPleatHold finds every panel already resting on the hinges it pushes
    // them to, and the first stroke flips none. (Set up in the middle of
    // their play, as they once were, all 144 flew half of it and landed on
    // the first stroke after every load and Reset: a thump no later stroke
    // the same way makes, 33 dB over the next stroke's folds.)
    void Rest() {
        for (int w = 0; w < kWords; ++w) {
            moving[w] = out[w] = in[w] = fresh[w] = 0;
            unplaced[w] = ~std::uint64_t(0);
        }
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
        const int steps = Pleat::Steps(dt);
        const double h = dt / steps;
        for (int i = 0; i < count; ++i) {
            // A copy, which the compiler keeps in registers through the step.
            Pleat pleat = pleats[list[i]];
            const int w = list[i] / 64;
            const std::uint64_t bit = std::uint64_t(1) << (list[i] % 64);
            if ((unplaced[w] & bit) && Abs(p) > kPleatHold) {
                // Its first load: at rest on that side's hinge, bearing it.
                unplaced[w] &= ~bit;
                pleat.contact = p * pleat.area;
                pleat.x = (p > 0.0 ? 1.0 : -1.0) * (pleat.play + Pleat::Compression(Abs(pleat.contact)));
                pleats[list[i]] = pleat;
                continue;
            }
            if (!pleat.moving && !(fresh[w] & bit)) pleat.contact = last * pleat.area;
            flips += pleat.Step(p, steps, h);
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
// How fast the bass end moves, m/s, when the hand moves the bellows at
// `handVelocity` wheel ranges a second and one range is `litres` of air over
// the bellows' section: the speed the strap's slack meets the wrist at.
inline double BellowsEndSpeed(double litres, double handVelocity) { return litres * handVelocity / kBellowsSection; }
struct Rustle {
    std::uint32_t seed = 0x2545F491u;
    double low = 0.0, lowCoeff = 0.0;
    double drawScale = 1.7320508075688772;
    BodyHighPass body;
    void Set(double rate) {
        lowCoeff = 1.0 - std::exp(-2.0 * 3.14159265358979 * 2000.0 / rate);
        // The force noise was voiced at 48 kHz. Preserve its power per Hz:
        // fixed-variance draws otherwise lose 3 dB at each rate doubling
        // through these fixed-Hz filters. Fold the rate scaling into the
        // existing uniform-to-unit-variance factor; 48 kHz stays identical.
        drawScale = 1.7320508075688772 * Sqrt(rate / 48000.0);
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
        const double draw = drawScale * ((seed >> 8) / double(1u << 24) * 2.0 - 1.0);
        low += lowCoeff * (body.Process(draw * sliding) - low);
        return low;
    }
};

} // namespace respira
