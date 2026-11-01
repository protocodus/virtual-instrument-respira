#pragma once

#include "DspMath.h"

#include <cmath>
#include <cstdint>

namespace respira {

// The virtual player's slow, aperiodic movement. It does not generate audio:
// effort moves Auto's hand; the other coordinates move the sources relative
// to the fixed microphones. One state serves the entire accordion, including
// held notes. A separate seed leaves the reeds' turbulent excitation alone.
//
// Low-frequency force variation is supported by motor-control measurements
// (Castronovo et al., 2018, doi:10.1152/jn.00093.2018;
// https://pmc.ncbi.nlm.nih.gov/articles/PMC6230772/). This is NOT an accordion
// calibration. The slow rates and user-selected performance extent below are
// authored performer choices, not measured physiological constants.
struct Life {
    static constexpr double kControlRate = 200.0;
    struct Drift {
        double a = 0.0, b = 0.0, step = 0.0, scale = 0.0;
        void Set(double hz) {
            step = 1.0 - std::exp(-2.0 * 3.141592653589793 * hz / kControlRate);
            scale = 1.0 / std::sqrt(CascadeVariance(step));
        }
        double Tick(double draw) {
            a += step * (draw - a);
            b += step * (a - b);
            return Clamp(b * scale, -1.5, 1.5);
        }
        void Clear() { a = b = 0.0; }
    } effortDrift, sideDrift, depthDrift;

    std::uint32_t seed = 0x6c696665u;
    double phase = 0.0, phaseStep = 0.0, glide = 0.0;
    double amount = 0.0, active = 0.0;
    double effort = 0.0, side = 0.0, depth = 0.0;

    void Prepare(double rate) {
        phaseStep = kControlRate / rate;
        glide = 1.0 - std::exp(-1.0 / (0.04 * kControlRate));
        effortDrift.Set(0.65);
        sideDrift.Set(0.27);
        depthDrift.Set(0.37);
    }
    void Reset() {
        seed = 0x6c696665u;
        phase = amount = active = effort = side = depth = 0.0;
        effortDrift.Clear(); sideDrift.Clear(); depthDrift.Clear();
    }
    double Draw() {
        seed = seed * 1664525u + 1013904223u;
        return 1.7320508075688772 * (2.0 * (seed >> 8) / double(1u << 24) - 1.0);
    }
    // Return true only when microphone targets need updating. The counter is
    // sample-driven, independent of host block size and MIDI event splitting.
    bool Step(double target, bool moving) {
        if (target == 0.0 && amount == 0.0) return false;
        phase += phaseStep;
        if (phase < 1.0) return false;
        phase -= 1.0;
        amount += glide * (target - amount);
        if (Abs(amount - target) < 1.0e-9) amount = target;
        active += glide * ((moving ? 1.0 : 0.0) - active);
        effort = effortDrift.Tick(Draw()) * active;
        side = sideDrift.Tick(Draw()) * active;
        depth = depthDrift.Tick(Draw()) * active;
        return true;
    }
};

} // namespace respira
