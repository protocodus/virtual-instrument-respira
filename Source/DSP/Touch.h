#pragma once

#include "DspMath.h"

#include <cmath>

namespace respira {

// MIDI velocity is a controller convention, not a measured finger speed.
// Keep the established ordinary/fast strokes (64..127, 7.46..3 ms). Below
// the midpoint, let speed fall in proportion to velocity: fixed travel at
// that speed takes T(64)*64/v. This opens access to deliberate slow presses
// (v16: 30 ms; v1: 478 ms) without slowing normal passagework.
//
// Motivation: Llanos-Vazquez et al., Acta Acustica 100 (2014), pp. 380-381,
// doi:10.3813/AAA.918716, section 4 / Table II: slowly opening the valve
// changes both harmonic growth and attack duration. Their measured 60/80 ms
// A4 mf attacks are AUDIO rise times, not key travel and not MIDI calibration.
// This mapping is an authored playing interface. It does not fit those times
// or change the vent geometry, reed constants, sustained gain or tuning.
inline double KeyOpeningSeconds(int velocity) {
    // Velocity zero is the existing register-slide gesture, never a key-on.
    if (velocity <= 0) return 0.012;
    if (velocity >= 64) return 0.012 - 0.009 * velocity / 127.0;
    return (0.012 - 0.009 * 64.0 / 127.0) * 64.0 / velocity;
}

// Optional virtual-player interpretation of key emphasis, separate from the
// valve mechanism above. A real accordion's sustained dynamics come from the
// bellows, not strike velocity (same source, sections 2-4). Touch asks Auto's
// HAND to make one shared effort gesture; it never scales voice audio or pitch.
// Its +/-30% extent and 200 ms relaxation are authored performance choices,
// NOT measured human constants. The existing arm and pressure feedback still
// determine the actual response. Manual hand control bypasses it entirely.
struct Touch {
    static constexpr double kControlRate = 200.0;
    double phase = 0.0, phaseStep = 0.0, release = 0.0, glide = 0.0;
    double amount = 0.0, gesture = 0.0;
    int pending = 0, strongest = 0, cluster = 0;

    void Prepare(double rate) {
        phaseStep = kControlRate / rate;
        release = std::exp(-1.0 / (0.2 * kControlRate));
        glide = 1.0 - std::exp(-1.0 / (0.04 * kControlRate));
    }
    void ClearGesture() { gesture = 0.0; pending = strongest = cluster = 0; }
    void Reset() { phase = amount = 0.0; ClearGesture(); }
    void Press(int velocity) { if (velocity > pending) pending = velocity; }
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
        if (cluster > 0) --cluster;
        if (pending > 0) {
            // One strongest intent for a chord/short roll, never a sum over
            // keys, ranks or channels. Same-sample MIDI order is immaterial.
            // A 10 ms collection span does not delay the keys or the audio;
            // the causal player updates at most 5 ms after the first press.
            if (cluster == 0) { strongest = pending; cluster = 2; }
            else if (pending > strongest) strongest = pending;
            gesture = (strongest - 64.0) / 63.0;
            pending = 0;
        }
    }
    double Effort() const { return 0.30 * amount * gesture; }
};

} // namespace respira
