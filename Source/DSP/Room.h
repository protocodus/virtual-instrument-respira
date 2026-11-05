#pragma once

// The room around the microphones: Init and the factory presets play in it at
// 0.5 %, a touch of air around the instrument (RespiraControls.h). Its late diffuse field comes from a sixteen-line feedback delay network
// (Jot and Chaigne, AES 90, 1991) with a Householder matrix: the two
// microphones decode two approximately independent field components with the
// diffuse-field correlation of coincident cardioids, where the direct sound reaches
// both microphones at once. Four Schroeder allpasses spread the sound before it
// enters the lines, so the field is dense from its start: fed straight in, an
// impulse came back as a dozen separate echoes in the first 50 ms, which a
// staccato note flutters through. Each line's length drifts by an eighth of a
// millisecond, slowly and at its own rate, so the field's modes never stand
// still: a fixed network rings metallic on a held chord, its modes 2.5 Hz
// apart beating against the reeds. Before the late field come the first
// reflections of a small hall, each from its own direction, so the pair
// hears them by level as it hears the instrument. It is a voiced room, not a
// model of any one room.

#include "DspMath.h"

#include <array>
#include <cmath>

namespace respira {

class Room {
public:
    // Line lengths spread geometrically over 23-83 ms, each a prime number of
    // samples at 48 kHz, so the modes they make do not pile up on one
    // frequency: 0.76 s of delay in all, a mode every 1.3 Hz.
    static constexpr int kLines = 16;
    static constexpr std::array<double, kLines> kLineSeconds { {
        0.023396, 0.025354, 0.027646, 0.030146, 0.032729, 0.035604, 0.038771, 0.042229,
        0.045896, 0.049854, 0.054354, 0.059021, 0.064229, 0.069979, 0.076229, 0.083104 } };
    static constexpr int kMaxLine = 16384;   // 83 ms at 192 kHz, plus the drift
    // The drift: 0.125 ms each way, at rates from 0.13 to 0.97 Hz spread over
    // the lines; a held note's tail moves by under a cent.
    static constexpr double kDriftSeconds = 0.000125, kSlowestDrift = 0.13, kFastestDrift = 0.97;
    // The diffusers' delays, mutually prime at 48 kHz, and their gain.
    static constexpr std::array<double, 4> kDiffuserSeconds { { 0.00153, 0.00271, 0.00419, 0.00587 } };
    static constexpr int kMaxDiffuser = 2048;   // 10.7 ms at 192 kHz, against the longest delay's 5.9 ms
    static constexpr double kDiffuserGain = 0.7;

    // The first reflections, by image sources: the player seated on a wooden
    // stage with the bellows 1.0 m above it, the microphones 1 m in front at
    // 1.2 m, in a small hall 12 x 9 m and 4 m high, the player 2 m from the
    // wall behind and midway between the side walls (an estimate of a room
    // that seats a hundred, not a measured one). Each: delay after the direct
    // sound (path difference over 343 m/s), level against it (1/r, times the
    // surface's reflection, 0.9 for the wooden stage and 0.8 for plaster) and
    // direction from the microphones (forward, right, up in metres). The wall behind
    // the microphones reaches them from behind, where a cardioid hears almost
    // nothing, so it is left out. The stage arrives about 4 ms after the
    // direct sound; its steep elevation reduces pickup at horizontal cardioids.
    struct Reflection { double seconds, level, forward, right, up; };
    static constexpr std::array<Reflection, 5> kReflections { {
        { 0.00407, 0.42 * 0.9, 1.0, 0.0, -2.2 },    // stage: 2.42 m against 1.02 m
        { 0.01162, 0.204 * 0.8, 5.0, 0.0, -0.2 },   // wall behind the player: 5.00 m
        { 0.01419, 0.173 * 0.8, 1.0, 0.0, 5.8 },    // ceiling: 5.89 m
        { 0.02343, 0.113 * 0.8, 1.0, -9.0, -0.2 },  // left wall: 9.06 m
        { 0.02343, 0.113 * 0.8, 1.0, 9.0, -0.2 },   // right wall
    } };
    static constexpr int kMaxEarly = 8192;      // 42 ms at 192 kHz
    // The audience's clothes and the walls' own absorption take the treble
    // off each reflection: a one-pole at 2 kHz on their sum, voiced dark
    // (owner, 2026-10-31: "quite dark").
    static constexpr double kEarlyCorner = 2000.0;
    // The same darkness on what enters the late field: a one-pole at 3 kHz
    // before the diffusers, voiced, so the hall's tail is warm from its
    // start rather than only darkening as it decays.
    static constexpr double kLateCorner = 3000.0;

    // A hundred people sit in it (owner, 2026-10-31: "as if in a room with
    // 100 people"). The hall above, 432 m3 with 384 m2 of surface, is taken
    // as bare plaster and wood at a mean coefficient of 0.10 (as the living
    // room before it was), and each listener adds the average absorption per
    // person Chagok et al. (University of Jos) measured in an occupied hall:
    // 0.29, 0.43, 0.51, 0.68, 0.71 and 0.73 m2 at 125 Hz to 4 kHz. The air
    // takes 4mV more (ISO 9613-1 at 20 C and 50 % RH: 1.1, 3.9 and 11.8 m2 at
    // 1, 2 and 4 kHz). Sabine then gives 1.03, 0.85, 0.77, 0.64, 0.61 and
    // 0.56 s; the one-pole's two settings below fit those bands best in log
    // time, within 21 % (its bass short, its 1-2 kHz long).
    static constexpr double kDecaySeconds = 0.825, kTrebleShare = 0.250;

    // `rt60` is the time the field takes to fall 60 dB at low frequencies;
    // it falls steadily faster towards the top, `trebleShare` of it at
    // Nyquist, as a room's walls and air absorb more of the treble (Jot's
    // absorptive one-pole in each line: its gain is each line's decay at DC
    // and at Nyquist).
    void Set(double rt60, double trebleShare, double rate) {
        drift = kDriftSeconds * rate;
        for (std::size_t i = 0; i < kLines; ++i) {
            lineLength[i] = Min(kLineSeconds[i] * rate, static_cast<double>(kMaxLine) - drift - 2.0);
            const double seconds = lineLength[i] / rate;
            // The lines' drift rates interleave across their lengths, so
            // neighbours do not move together.
            const double share = static_cast<double>((i * 7u) % kLines) / (kLines - 1);
            const double step = 6.283185307179586 * (kSlowestDrift + share * (kFastestDrift - kSlowestDrift)) / rate;
            turnCos[i] = std::cos(step);
            turnSin[i] = std::sin(step);
            const double low = std::pow(10.0, -3.0 * seconds / rt60), high = std::pow(10.0, -3.0 * seconds / (rt60 * trebleShare));
            linePole[i] = (low - high) / (low + high);
            lineGain[i] = low * (1.0 - linePole[i]);
        }
        for (std::size_t i = 0; i < diffusers.size(); ++i)
            diffusers[i].length = static_cast<int>(Min(kDiffuserSeconds[i] * rate, static_cast<double>(kMaxDiffuser - 1)));
        for (std::size_t i = 0; i < kReflections.size(); ++i) {
            const Reflection& r = kReflections[i];
            early[i].length = static_cast<int>(Min(r.seconds * rate, static_cast<double>(kMaxEarly - 1)));
        }
        widthSmoothing = 1.0 - std::exp(-1.0 / (0.02 * rate));
        widthSettleSamples = static_cast<int>(std::ceil(0.64 * rate)); // 32 time constants: below 1.3e-14
        SetWidth(width, true);
        earlyPole = std::exp(-6.283185307179586 * Min(kEarlyCorner, 0.45 * rate) / rate);
        latePole = std::exp(-6.283185307179586 * Min(kLateCorner, 0.45 * rate) / rate);
    }

    // Same horizontal XY axes as the direct microphones. A reflection keeps
    // its 3D arrival angle; its gain is relative to the centre direct ray
    // (1 m forward, 0.2 m down), rather than re-normalising every reflection.
    void SetWidth(double next, bool immediate = false) {
        next = next == next ? Clamp(next, 0.0, 1.0) : 0.6;
        if (next == width && widthTargetsReady) {
            if (immediate) TakeWidthTargets();
            return;
        }
        width = next;
        const double axis = width * 1.3089969389957472; // 0..75 degrees each side
        const double forward = std::cos(axis), side = std::sin(axis);
        const double centre = (1.0 + forward / std::sqrt(1.04)) * 1.4142135623730951;
        for (std::size_t i = 0; i < kReflections.size(); ++i) {
            const Reflection& r = kReflections[i];
            const double inverseDistance = 1.0 / std::sqrt(r.forward * r.forward + r.right * r.right + r.up * r.up);
            early[i].leftTarget = r.level * (1.0 + (r.forward * forward - r.right * side) * inverseDistance) / centre;
            early[i].rightTarget = r.level * (1.0 + (r.forward * forward + r.right * side) * inverseDistance) / centre;
        }
        // Isotropic 3D field, ideal cardioids separated by theta=2*axis:
        // rho=(3+cos(theta))/4. Equal-power independent mid/side components
        // give L=a*M+b*S, R=a*M-b*S, a^2+b^2=1 and a^2-b^2=rho.
        // Schoeps, Wittek et al., Double M/S (2010), section 3.2.1.
        const double rho = (3.0 + std::cos(2.0 * axis)) * 0.25;
        lateMidTarget = std::sqrt((1.0 + rho) * 0.5);
        lateSideTarget = std::sqrt((1.0 - rho) * 0.5);
        widthTargetsReady = true;
        widthRemaining = widthSettleSamples;
        if (immediate) TakeWidthTargets();
    }

    void Clear() {
        for (std::size_t i = 0; i < kLines; ++i) {
            lineBuffer[i].fill(0.0f);
            lineLow[i] = 0.0;
            // Each line starts its drift at its own phase.
            driftCos[i] = std::cos(2.399963229728653 * i);   // the golden angle apart
            driftSin[i] = std::sin(2.399963229728653 * i);
        }
        lineWrite = 0;
        for (Diffuser& d : diffusers) d.buffer.fill(0.0f);
        diffuserWrite = 0;
        earlyBuffer.fill(0.0f);
        earlyWrite = 0;
        earlyL = earlyR = 0.0;
        late = 0.0;
        TakeWidthTargets();
        // Every buffer holds silence: rest at once.
        resting = true;
        quietFor = skipped = 0;
    }

    // Takes the two microphones' direct sound and returns what the room adds
    // to each.
    void Process(double left, double right, double& roomLeft, double& roomRight) {
        if (widthRemaining > 0) {
            for (Tap& tap : early) {
                tap.left += widthSmoothing * (tap.leftTarget - tap.left);
                tap.right += widthSmoothing * (tap.rightTarget - tap.right);
            }
            lateMid += widthSmoothing * (lateMidTarget - lateMid);
            lateSide += widthSmoothing * (lateSideTarget - lateSide);
            if (--widthRemaining == 0) TakeWidthTargets();
        }
        // At rest every buffer and state is zero, and zero in gives exactly
        // zero out and zero back into every buffer, whatever the zeros' signs.
        // Only the drift's phasors and the write positions move on: the
        // phasors here, the positions when the room wakes.
        if (resting) {
            if (left == 0.0 && right == 0.0) {
                TurnDrift();
                ++skipped;
                roomLeft = roomRight = 0.0;
                return;
            }
            Wake();
        }
        std::array<double, kLines> out {};
        double sum = 0.0;
        TurnDrift();
        for (std::size_t i = 0; i < kLines; ++i) {
            const auto& buffer = lineBuffer[i];
            double read = lineWrite - lineLength[i] - drift * driftSin[i];
            if (read < 0.0) read += kMaxLine;
            const int at = static_cast<int>(read), next = at + 1 == kMaxLine ? 0 : at + 1;
            const double frac = read - at;
            const double delayed = buffer[static_cast<std::size_t>(at)] + frac * (buffer[static_cast<std::size_t>(next)] - buffer[static_cast<std::size_t>(at)]);
            lineLow[i] = lineGain[i] * delayed + linePole[i] * lineLow[i];
            out[i] = lineLow[i];
            sum += out[i];
        }
        double input = 0.5 * (left + right);
        earlyBuffer[static_cast<std::size_t>(earlyWrite)] = static_cast<float>(input);
        double reflectedL = 0.0, reflectedR = 0.0;
        for (const Tap& tap : early) {
            int read = earlyWrite - tap.length;
            if (read < 0) read += kMaxEarly;
            const double x = earlyBuffer[static_cast<std::size_t>(read)];
            reflectedL += tap.left * x;
            reflectedR += tap.right * x;
        }
        if (++earlyWrite == kMaxEarly) earlyWrite = 0;
        earlyL = reflectedL + earlyPole * (earlyL - reflectedL);
        earlyR = reflectedR + earlyPole * (earlyR - reflectedR);
        late = input + latePole * (late - input);
        input = late;
        for (Diffuser& d : diffusers) {
            int read = diffuserWrite - d.length;
            if (read < 0) read += kMaxDiffuser;
            const double delayed = d.buffer[static_cast<std::size_t>(read)];
            const double v = input + kDiffuserGain * delayed;
            d.buffer[static_cast<std::size_t>(diffuserWrite)] = Settled(v);
            input = delayed - kDiffuserGain * v;
        }
        const int diffuserWritten = diffuserWrite;
        if (++diffuserWrite == kMaxDiffuser) diffuserWrite = 0;
        const double feedback = 2.0 / kLines * sum;
        // Preserve the network's established input normalization. The output
        // projections below preserve its former per-channel coefficient norm.
        input *= 0.7071067811865476;
        roomLeft = earlyL;
        roomRight = earlyR;
        double sideField = 0.0;
        for (std::size_t i = 0; i < kLines; ++i) {
            const double sign = (i & 2u) ? -1.0 : 1.0;
            lineBuffer[i][static_cast<std::size_t>(lineWrite)] = Settled(out[i] - feedback + sign * input);
            // Constant and parity-of-three-bits Hadamard rows: orthogonal
            // readouts, each also orthogonal to the alternating-pair input.
            // The finite FDN only approximates independent diffuse components;
            // impulse/noise-tail covariance is checked across sample rates.
            const bool negative = ((i ^ (i >> 1u) ^ (i >> 2u)) & 1u) != 0u;
            sideField += negative ? -out[i] : out[i];
        }
        if (++lineWrite == kMaxLine) lineWrite = 0;
        // Coefficient norm 16*(1/2)^2=4, as in each former eight-line output
        // (8*(1/sqrt(2))^2). No fitted gain or adaptive normalisation.
        const double mid = 0.5 * lateMid * sum, side = 0.5 * lateSide * sideField;
        roomLeft += mid + side;
        roomRight += mid - side;
        // Silence in and silence written everywhere (each line writes its
        // out[i] - feedback + sign * input) for as long as the longest buffer
        // takes to wrap means every buffer holds only zeros: rest.
        if (left != 0.0 || right != 0.0 || input != 0.0 || late != 0.0 || earlyL != 0.0 || earlyR != 0.0) {
            quietFor = 0;
            return;
        }
        for (std::size_t i = 0; i < kLines; ++i)
            if (out[i] != 0.0) {
                quietFor = 0;
                return;
            }
        for (const Diffuser& d : diffusers)
            if (d.buffer[static_cast<std::size_t>(diffuserWritten)] != 0.0f) {
                quietFor = 0;
                return;
            }
        if (++quietFor >= kMaxLine) {
            resting = true;
            quietFor = 0;
            lineLow.fill(0.0);
            earlyL = earlyR = late = 0.0;
        }
    }

    void FlushTiny() {
        for (double& low : lineLow)
            if (Abs(low) < 1.0e-20) low = 0.0;
        if (Abs(earlyL) < 1.0e-20) earlyL = 0.0;
        if (Abs(earlyR) < 1.0e-20) earlyR = 0.0;
        if (Abs(late) < 1.0e-20) late = 0.0;
    }

private:
    // What a buffer stores: silence below the smallest normal float (-758
    // dB). Subnormals would otherwise round back up through the feedback
    // gains and circulate for ever, and the room would never rest.
    static constexpr double kSettled = 1.1754943508222875e-38;
    static float Settled(double x) { return Abs(x) < kSettled ? 0.0f : static_cast<float>(x); }
    // The drifts' phasors turn by one step; their length is kept at 1 by a
    // first-order correction, so no libm call runs per sample.
    void TurnDrift() {
        for (std::size_t i = 0; i < kLines; ++i) {
            const double c = driftCos[i] * turnCos[i] - driftSin[i] * turnSin[i];
            const double s = driftSin[i] * turnCos[i] + driftCos[i] * turnSin[i];
            const double norm = 1.5 - 0.5 * (c * c + s * s);
            driftCos[i] = c * norm;
            driftSin[i] = s * norm;
        }
    }
    // The write positions catch up with the samples spent at rest.
    void Wake() {
        lineWrite = static_cast<int>((lineWrite + skipped) % kMaxLine);
        diffuserWrite = static_cast<int>((diffuserWrite + skipped) % kMaxDiffuser);
        earlyWrite = static_cast<int>((earlyWrite + skipped) % kMaxEarly);
        resting = false;
        skipped = 0;
    }
    void TakeWidthTargets() {
        for (Tap& tap : early) {
            tap.left = tap.leftTarget;
            tap.right = tap.rightTarget;
        }
        lateMid = lateMidTarget;
        lateSide = lateSideTarget;
        widthRemaining = 0;
    }
    // The lines, a quantity to an array, so that a pass over them reads
    // neighbouring values rather than one value 64 kB from the next. Every
    // line writes at the same place, as every diffuser does.
    static constexpr std::array<double, kLines> Filled(double value) {
        std::array<double, kLines> a {};
        for (double& v : a) v = value;
        return a;
    }
    std::array<std::array<float, kMaxLine>, kLines> lineBuffer {};
    std::array<double, kLines> lineLength = Filled(1.0), lineGain {}, linePole {}, lineLow {};
    std::array<double, kLines> driftCos = Filled(1.0), driftSin {}, turnCos = Filled(1.0), turnSin {};
    int lineWrite = 0;
    double drift = 0.0;
    struct Diffuser {
        std::array<float, kMaxDiffuser> buffer {};
        int length = 1;
    };
    std::array<Diffuser, 4> diffusers {};
    int diffuserWrite = 0;
    struct Tap {
        int length = 1;
        double left = 0.0, right = 0.0;
        double leftTarget = 0.0, rightTarget = 0.0;
    };
    std::array<Tap, kReflections.size()> early {};
    std::array<float, kMaxEarly> earlyBuffer {};
    int earlyWrite = 0;
    double earlyPole = 0.0, earlyL = 0.0, earlyR = 0.0;
    double latePole = 0.0, late = 0.0;
    double width = 0.6, widthSmoothing = 0.0;
    bool widthTargetsReady = false;
    int widthSettleSamples = 0, widthRemaining = 0;
    double lateMid = 0.0, lateSide = 0.0, lateMidTarget = 0.0, lateSideTarget = 0.0;
    bool resting = false;
    int quietFor = 0;             // samples of silence in and written everywhere
    long long skipped = 0;        // samples spent resting, by which the writes catch up
};

} // namespace respira
