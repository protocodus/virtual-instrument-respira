#pragma once

// The room around the microphones: Init and the factory presets play in it at
// 5 %, a touch of air around the instrument (RespiraControls.h). Its late diffuse field comes from a sixteen-line feedback delay network
// (Jot and Chaigne, AES 90, 1991) with a Householder matrix: the two
// microphones each hear eight of the lines, so the field reaching them is
// decorrelated as a real room's is, where the instrument's own sound reaches
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
    // frequency: 1.27 s of delay in all, a mode every 0.8 Hz.
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
    static constexpr int kMaxDiffuser = 2048;   // 5.9 ms at 192 kHz, plus room to spare
    static constexpr double kDiffuserGain = 0.7;

    // The first reflections, by image sources: the player seated on a wooden
    // stage with the bellows 1.0 m above it, the microphones 1 m in front at
    // 1.2 m, in a small hall 12 x 9 m and 4 m high, the player 2 m from the
    // wall behind and midway between the side walls (an estimate of a room
    // that seats a hundred, not a measured one). Each: delay after the direct
    // sound (path difference over 343 m/s), level against it (1/r, times the
    // surface's reflection, 0.9 for the wooden stage and 0.8 for plaster) and
    // azimuth in the player's view (positive to the right). The wall behind
    // the microphones reaches them from behind, where a cardioid hears almost
    // nothing, so it is left out. The stage's reflection, -8 dB at 4 ms, combs
    // the sound by at most +3/-4 dB below 4 kHz, as it does a real recording.
    struct Reflection { double seconds, level, azimuth; };
    static constexpr std::array<Reflection, 5> kReflections { {
        { 0.00407, 0.42 * 0.9, 0.0 },      // stage: 2.42 m against 1.02 m
        { 0.01162, 0.204 * 0.8, 0.0 },     // the wall behind the player: 5.00 m
        { 0.01419, 0.173 * 0.8, 0.0 },     // ceiling: 5.89 m
        { 0.02343, 0.113 * 0.8, -1.46 },   // left wall: 9.06 m, 84 degrees left
        { 0.02343, 0.113 * 0.8, 1.46 },    // right wall: the same, to the right
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
        for (std::size_t i = 0; i < lines.size(); ++i) {
            Line& line = lines[i];
            line.length = Min(kLineSeconds[i] * rate, static_cast<double>(kMaxLine) - drift - 2.0);
            const double seconds = line.length / rate;
            // The lines' drift rates interleave across their lengths, so
            // neighbours do not move together.
            const double share = static_cast<double>((i * 7u) % lines.size()) / (lines.size() - 1);
            const double step = 6.283185307179586 * (kSlowestDrift + share * (kFastestDrift - kSlowestDrift)) / rate;
            line.turnCos = std::cos(step);
            line.turnSin = std::sin(step);
            const double low = std::pow(10.0, -3.0 * seconds / rt60), high = std::pow(10.0, -3.0 * seconds / (rt60 * trebleShare));
            line.pole = (low - high) / (low + high);
            line.gain = low * (1.0 - line.pole);
        }
        for (std::size_t i = 0; i < diffusers.size(); ++i)
            diffusers[i].length = static_cast<int>(Min(kDiffuserSeconds[i] * rate, static_cast<double>(kMaxDiffuser - 1)));
        // An XY pair at 90 degrees, normalised as the instrument is
        // (RespiraEngine.cpp, MicPair): a source straight ahead reaches each
        // microphone at 1/sqrt(2).
        const double axis = 0.7853981633974483, centre = (1.0 + std::cos(axis)) * 1.4142135623730951;
        for (std::size_t i = 0; i < kReflections.size(); ++i) {
            const Reflection& r = kReflections[i];
            early[i].length = static_cast<int>(Min(r.seconds * rate, static_cast<double>(kMaxEarly - 1)));
            early[i].left = r.level * (1.0 + std::cos(r.azimuth + axis)) / centre;
            early[i].right = r.level * (1.0 + std::cos(r.azimuth - axis)) / centre;
        }
        earlyPole = std::exp(-6.283185307179586 * Min(kEarlyCorner, 0.45 * rate) / rate);
        latePole = std::exp(-6.283185307179586 * Min(kLateCorner, 0.45 * rate) / rate);
    }

    void Clear() {
        for (std::size_t i = 0; i < lines.size(); ++i) {
            Line& line = lines[i];
            line.buffer.fill(0.0f);
            line.write = 0;
            line.low = 0.0;
            // Each line starts its drift at its own phase.
            line.cos = std::cos(2.399963229728653 * i);   // the golden angle apart
            line.sin = std::sin(2.399963229728653 * i);
        }
        for (Diffuser& d : diffusers) {
            d.buffer.fill(0.0f);
            d.write = 0;
        }
        earlyBuffer.fill(0.0f);
        earlyWrite = 0;
        earlyL = earlyR = 0.0;
        late = 0.0;
    }

    // Takes the two microphones' direct sound and returns what the room adds
    // to each.
    void Process(double left, double right, double& roomLeft, double& roomRight) {
        std::array<double, kLines> out {};
        double sum = 0.0;
        for (std::size_t i = 0; i < lines.size(); ++i) {
            Line& line = lines[i];
            // The drift's phasor turns by one step; its length is kept at 1 by
            // a first-order correction, so no libm call runs per sample.
            const double c = line.cos * line.turnCos - line.sin * line.turnSin;
            const double s = line.sin * line.turnCos + line.cos * line.turnSin;
            const double norm = 1.5 - 0.5 * (c * c + s * s);
            line.cos = c * norm;
            line.sin = s * norm;
            double read = line.write - line.length - drift * line.sin;
            if (read < 0.0) read += kMaxLine;
            const int at = static_cast<int>(read), next = at + 1 == kMaxLine ? 0 : at + 1;
            const double frac = read - at;
            const double delayed = line.buffer[static_cast<std::size_t>(at)] + frac * (line.buffer[static_cast<std::size_t>(next)] - line.buffer[static_cast<std::size_t>(at)]);
            line.low = line.gain * delayed + line.pole * line.low;
            out[i] = line.low;
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
            int read = d.write - d.length;
            if (read < 0) read += kMaxDiffuser;
            const double delayed = d.buffer[static_cast<std::size_t>(read)];
            const double v = input + kDiffuserGain * delayed;
            d.buffer[static_cast<std::size_t>(d.write)] = static_cast<float>(v);
            if (++d.write == kMaxDiffuser) d.write = 0;
            input = delayed - kDiffuserGain * v;
        }
        const double feedback = 2.0 / kLines * sum;
        // Sixteen lines each fed the sound and each microphone hearing eight
        // put twice the energy of the eight-line network into each channel
        // twice over; the input and outputs each take 1/sqrt(2) of it back.
        input *= 0.7071067811865476;
        roomLeft = earlyL;
        roomRight = earlyR;
        for (std::size_t i = 0; i < lines.size(); ++i) {
            Line& line = lines[i];
            const double sign = (i & 2u) ? -1.0 : 1.0;
            line.buffer[static_cast<std::size_t>(line.write)] = static_cast<float>(out[i] - feedback + sign * input);
            if (++line.write == kMaxLine) line.write = 0;
            ((i & 1u) ? roomRight : roomLeft) += 0.7071067811865476 * sign * out[i];
        }
    }

    void FlushTiny() {
        for (Line& line : lines)
            if (Abs(line.low) < 1.0e-20) line.low = 0.0;
        if (Abs(earlyL) < 1.0e-20) earlyL = 0.0;
        if (Abs(earlyR) < 1.0e-20) earlyR = 0.0;
        if (Abs(late) < 1.0e-20) late = 0.0;
    }

private:
    struct Line {
        std::array<float, kMaxLine> buffer {};
        double length = 1.0;
        int write = 0;
        double gain = 0.0, pole = 0.0, low = 0.0;
        double cos = 1.0, sin = 0.0, turnCos = 1.0, turnSin = 0.0;
    };
    std::array<Line, kLines> lines {};
    double drift = 0.0;
    struct Diffuser {
        std::array<float, kMaxDiffuser> buffer {};
        int length = 1, write = 0;
    };
    std::array<Diffuser, 4> diffusers {};
    struct Tap {
        int length = 1;
        double left = 0.0, right = 0.0;
    };
    std::array<Tap, kReflections.size()> early {};
    std::array<float, kMaxEarly> earlyBuffer {};
    int earlyWrite = 0;
    double earlyPole = 0.0, earlyL = 0.0, earlyR = 0.0;
    double latePole = 0.0, late = 0.0;
};

} // namespace respira
