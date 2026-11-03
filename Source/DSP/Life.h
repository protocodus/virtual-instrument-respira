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
    // Upper-limb isometric/isotonic force steadiness exhibits both slow (<1 Hz)
    // common-drive drift and a physiological stretch-reflex / motor-unit firing
    // tremor band around 6-8 Hz (Castronovo et al., 2018; Enoka & Farina,
    // J Physiol 599, 2021). A bilinear-prewarped bandpass at 6.5 Hz (Q = 3.5),
    // driven by the same white draw as effortDrift, adds zero-DC micro-shiver
    // to the player's effort without shifting bar-level air consumption.
    static constexpr double kTremorShare = 0.12;
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
    struct Tremor {
        double b0 = 0.0, a1 = 0.0, a2 = 0.0, z1 = 0.0, z2 = 0.0, scale = 0.0;
        void Set(double hz, double q) {
            const double k = std::tan(3.141592653589793 * hz / kControlRate);
            const double norm = 1.0 / (1.0 + k / q + k * k);
            b0 = (k / q) * norm;
            a1 = 2.0 * (k * k - 1.0) * norm;
            a2 = (1.0 - k / q + k * k) * norm;
            // Discrete-time white-noise variance of a biquad bandpass H(z) =
            // b0 (1 - z^-2) / (1 + a1 z^-1 + a2 z^-2) is b0^2 (1 + a2) /
            // ((1 - a2) ((1 + a2)^2 - a1^2)); normalize back to unit RMS.
            const double denom = (1.0 - a2) * ((1.0 + a2) * (1.0 + a2) - a1 * a1);
            scale = denom > 1.0e-12 ? 1.0 / std::sqrt(b0 * b0 * (1.0 + a2) / denom) : 0.0;
        }
        double Tick(double draw) {
            const double y = b0 * draw + z1;
            z1 = -a1 * y + z2;
            z2 = -b0 * draw - a2 * y;
            return Clamp(y * scale, -1.5, 1.5);
        }
        void Clear() { z1 = z2 = 0.0; }
    } effortTremor;

    std::uint32_t seed = 0x6c696665u;
    double phase = 0.0, phaseStep = 0.0, glide = 0.0;
    double amount = 0.0, active = 0.0;
    double effort = 0.0, side = 0.0, depth = 0.0;

    void Prepare(double rate) {
        phaseStep = kControlRate / rate;
        glide = 1.0 - std::exp(-1.0 / (0.04 * kControlRate));
        effortDrift.Set(0.65);
        effortTremor.Set(6.5, 3.5);
        sideDrift.Set(0.27);
        depthDrift.Set(0.37);
    }
    void Reset() {
        seed = 0x6c696665u;
        phase = amount = active = effort = side = depth = 0.0;
        effortDrift.Clear(); effortTremor.Clear(); sideDrift.Clear(); depthDrift.Clear();
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
        const double effortDraw = Draw();
        effort = Clamp(effortDrift.Tick(effortDraw) + kTremorShare * effortTremor.Tick(effortDraw), -1.5, 1.5) * active;
        side = sideDrift.Tick(Draw()) * active;
        depth = depthDrift.Tick(Draw()) * active;
        return true;
    }
};

} // namespace respira
