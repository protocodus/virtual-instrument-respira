#pragma once

#include "DspMath.h"

#include <cmath>

namespace respira {

// MIDI velocity is a controller convention, not a measured finger speed.
// Use the established bounded 12..3 ms travel across the whole MIDI range.
// 0.1.5 extrapolated speed proportionally to velocity below 64, giving a
// 478 ms travel at v1. A short soft note could then release its key before
// the pallet passed enough air to start the reed. A small MIDI velocity
// is not evidence that the player intended a half-second key movement.
//
// Motivation: Llanos-Vazquez et al., Acta Acustica 100 (2014), pp. 380-381,
// doi:10.3813/AAA.918716, section 4 / Table II: slowly opening the valve
// changes both harmonic growth and attack duration. Their measured 60/80 ms
// A4 mf attacks are AUDIO rise times, not key travel and not MIDI calibration.
// This mapping is an authored playing interface, restored from 0.1.4; it
// is NOT a fit to the measured audio rise times. Velocity 64..127 and the
// shared Touch bellows gesture are unchanged. Bellows pressure still governs
// the reed's growth, colour, pitch and sustained level.
inline double KeyOpeningSeconds(int velocity) {
    // Velocity zero is the existing register-slide gesture, never a key-on.
    if (velocity <= 0) return 0.012;
    return 0.012 - 0.009 * velocity / 127.0;
}

// Optional virtual-player interpretation of key velocity, separate from the
// valve mechanism above. A real accordion's sustained dynamics come from the
// bellows, not strike velocity (same source, sections 2-4). Touch asks Auto's
// HAND to adjust its shared effort: strong strikes add an initial accent and
// sustain at full playing speed, while soft strikes catch the reed and settle
// to a slower sustaining bellows speed; the mod wheel slows either down. Manual
// hand control bypasses it entirely.
struct Touch {
    static constexpr double kControlRate = 200.0;
    // Below v=100, sustained velocity slows the bellows down toward 45% (180 Pa)
    // at v=1 (~6-8 dB softer than ff, keeping every treble register speaking
    // clearly on fast melodies), while starting a fresh soft press at >=68%
    // for 35 ms so short staccato reeds catch immediately.
    static constexpr int kFullVelocity = 100;
    static constexpr double kSoftSustainReduction = 0.55;
    static constexpr double kSoftCatchFloor = -0.32;
    double phase = 0.0, phaseStep = 0.0, release = 0.0, glide = 0.0, sustainGlide = 0.0;
    double amount = 0.0, gesture = 0.0, sustain = 0.0, sustainTarget = 0.0;
    int pending = 0, heldPending = 0, strongest = 0, cluster = 0, catchTicks = 0;

    void Prepare(double rate) {
        phaseStep = kControlRate / rate;
        release = std::exp(-1.0 / (0.2 * kControlRate));
        glide = 1.0 - std::exp(-1.0 / (0.04 * kControlRate));
        sustainGlide = 1.0 - std::exp(-1.0 / (0.08 * kControlRate));
    }
    void ClearGesture() {
        gesture = sustain = sustainTarget = 0.0;
        pending = heldPending = strongest = cluster = catchTicks = 0;
    }
    void Reset() { phase = amount = 0.0; ClearGesture(); }
    void Press(int velocity) { if (velocity > pending) pending = velocity; }
    void Hold(int velocity) { if (velocity > 0) heldPending = velocity; }
    void Step(double target, bool automatic) {
        if (!automatic) ClearGesture();
        if (target == 0.0 && amount == 0.0) { ClearGesture(); return; }
        phase += phaseStep;
        if (phase < 1.0) return;
        phase -= 1.0;
        amount += glide * (target - amount);
        if (Abs(amount - target) < 1.0e-9) amount = target;
        gesture *= release;
        if (Abs(gesture) < 1.0e-12) gesture = 0.0;
        if (catchTicks > 0) {
            --catchTicks;
        } else {
            sustain += sustainGlide * (sustainTarget - sustain);
            if (Abs(sustain - sustainTarget) < 1.0e-12) sustain = sustainTarget;
        }
        if (cluster > 0) --cluster;
        if (pending > 0) {
            // One strongest intent for a chord/short roll, never a sum over
            // keys, ranks or channels. Same-sample MIDI order is immaterial.
            // A 10 ms collection span does not delay the keys or the audio;
            // the causal player updates at most 5 ms after the first press.
            if (cluster == 0) { strongest = pending; cluster = 2; }
            else if (pending > strongest) strongest = pending;
            gesture = strongest > 64 ? (strongest - 64.0) / 63.0 : 0.0;
            if (strongest >= kFullVelocity) {
                sustain = sustainTarget = 0.0;
                catchTicks = 0;
            } else {
                const double u = (kFullVelocity - strongest) / (kFullVelocity - 1.0);
                sustainTarget = -kSoftSustainReduction * u;
                // Hold the reed-catch floor for 35 ms on a fresh press so
                // 50 ms staccato notes speak, then glide (80 ms) to
                // sustainTarget; within a soft phrase keep the soft level.
                if (sustain > -0.25) {
                    sustain = Max(sustain, Max(sustainTarget, kSoftCatchFloor));
                    catchTicks = 7;
                } else {
                    sustain = Max(sustain, Max(sustainTarget, -0.45));
                    catchTicks = 2;
                }
            }
            pending = heldPending = 0;
        } else if (heldPending > 0 && cluster == 0) {
            strongest = heldPending;
            if (strongest >= kFullVelocity) {
                sustainTarget = 0.0;
            } else {
                const double u = (kFullVelocity - strongest) / (kFullVelocity - 1.0);
                sustainTarget = -kSoftSustainReduction * u;
            }
            heldPending = 0;
        }
    }
    double Effort() const { return amount * (0.30 * gesture + sustain); }
};

} // namespace respira
