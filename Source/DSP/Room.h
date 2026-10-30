#pragma once

// The room around the microphones, optional and off by default: Respira is dry
// unless Room is raised. A late diffuse field only, from an eight-line feedback
// delay network (Jot and Chaigne, AES 90, 1991) with a Householder matrix: the
// two microphones each hear four of the lines, so the field reaching them is
// decorrelated as a real room's is, where the instrument's own sound reaches
// both microphones at once. Four Schroeder allpasses spread the sound before it
// enters the lines, so the field is dense from its start: fed straight in, an
// impulse came back as a dozen separate echoes in the first 50 ms, which a
// staccato note flutters through. It is a voiced room, not a model of any one
// room.

#include "DspMath.h"

#include <array>
#include <cmath>

namespace respira {

class Room {
public:
    // Line lengths spread over 30-75 ms, mutually prime at 48 kHz, so the
    // modes they make do not pile up on one frequency.
    static constexpr std::array<double, 8> kLineSeconds { { 0.02971, 0.03713, 0.04111, 0.04373, 0.05333, 0.05927, 0.06709, 0.07369 } };
    static constexpr int kMaxLine = 16384;   // 74 ms at 192 kHz, plus room to spare
    // The diffusers' delays, mutually prime at 48 kHz, and their gain.
    static constexpr std::array<double, 4> kDiffuserSeconds { { 0.00153, 0.00271, 0.00419, 0.00587 } };
    static constexpr int kMaxDiffuser = 2048;   // 5.9 ms at 192 kHz, plus room to spare
    static constexpr double kDiffuserGain = 0.7;

    // `rt60` is the time the field takes to fall 60 dB at low frequencies;
    // it falls steadily faster towards the top, `trebleShare` of it at
    // Nyquist, as a room's walls and air absorb more of the treble (Jot's
    // absorptive one-pole in each line: its gain is each line's decay at DC
    // and at Nyquist).
    void Set(double rt60, double trebleShare, double rate) {
        for (std::size_t i = 0; i < lines.size(); ++i) {
            Line& line = lines[i];
            line.length = static_cast<int>(Min(kLineSeconds[i] * rate, static_cast<double>(kMaxLine - 1)));
            const double seconds = line.length / rate;
            const double low = std::pow(10.0, -3.0 * seconds / rt60), high = std::pow(10.0, -3.0 * seconds / (rt60 * trebleShare));
            line.pole = (low - high) / (low + high);
            line.gain = low * (1.0 - line.pole);
        }
        for (std::size_t i = 0; i < diffusers.size(); ++i)
            diffusers[i].length = static_cast<int>(Min(kDiffuserSeconds[i] * rate, static_cast<double>(kMaxDiffuser - 1)));
    }

    void Clear() {
        for (Line& line : lines) {
            line.buffer.fill(0.0f);
            line.write = 0;
            line.low = 0.0;
        }
        for (Diffuser& d : diffusers) {
            d.buffer.fill(0.0f);
            d.write = 0;
        }
    }

    // Takes the two microphones' direct sound and returns what the room adds
    // to each.
    void Process(double left, double right, double& roomLeft, double& roomRight) {
        std::array<double, 8> out {};
        double sum = 0.0;
        for (std::size_t i = 0; i < lines.size(); ++i) {
            Line& line = lines[i];
            int read = line.write - line.length;
            if (read < 0) read += kMaxLine;
            line.low = line.gain * line.buffer[static_cast<std::size_t>(read)] + line.pole * line.low;
            out[i] = line.low;
            sum += out[i];
        }
        double input = 0.5 * (left + right);
        for (Diffuser& d : diffusers) {
            int read = d.write - d.length;
            if (read < 0) read += kMaxDiffuser;
            const double delayed = d.buffer[static_cast<std::size_t>(read)];
            const double v = input + kDiffuserGain * delayed;
            d.buffer[static_cast<std::size_t>(d.write)] = static_cast<float>(v);
            if (++d.write == kMaxDiffuser) d.write = 0;
            input = delayed - kDiffuserGain * v;
        }
        const double feedback = 0.25 * sum;   // 2/N
        roomLeft = roomRight = 0.0;
        for (std::size_t i = 0; i < lines.size(); ++i) {
            Line& line = lines[i];
            const double sign = (i & 2u) ? -1.0 : 1.0;
            line.buffer[static_cast<std::size_t>(line.write)] = static_cast<float>(out[i] - feedback + sign * input);
            if (++line.write == kMaxLine) line.write = 0;
            ((i & 1u) ? roomRight : roomLeft) += sign * out[i];
        }
    }

    void FlushTiny() {
        for (Line& line : lines)
            if (Abs(line.low) < 1.0e-20) line.low = 0.0;
    }

private:
    struct Line {
        std::array<float, kMaxLine> buffer {};
        int length = 1, write = 0;
        double gain = 0.0, pole = 0.0, low = 0.0;
    };
    std::array<Line, 8> lines {};
    struct Diffuser {
        std::array<float, kMaxDiffuser> buffer {};
        int length = 1, write = 0;
    };
    std::array<Diffuser, 4> diffusers {};
};

} // namespace respira
