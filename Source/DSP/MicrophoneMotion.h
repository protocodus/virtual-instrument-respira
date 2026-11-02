#pragma once

#include "DspMath.h"

namespace respira {

// Relative near-field response of an ideal cardioid to a spherical wave.
// Linear momentum gives u/p = (1 + c/(s*r))/(rho*c); combining pressure
// and its axial gradient gives H = 1 + cos(theta) + c*cos(theta)/(s*r).
// Removing the broadband factor already handled by MicPair, and dividing
// by the response at the existing reference position, leaves
//                Hmoving / Hreference = (s + a) / (s + a0),
//                a = c*cos(theta)/(r*(1 + cos(theta))).
// No LF corner or boost is chosen by ear. The reference response is the
// existing voiced instrument, not a measured microphone calibration.
//
// This is the small-capsule, low-frequency limit of the two-port geometry
// in Zadvitskiy, DAGA 2024, equations 2-8:
// https://pub.dega-akustik.de/DAGA_2024/files/upload/paper/351.pdf
// DPA also documents the dependence on distance, angle and source geometry:
// https://www.dpamicrophones.com/mic-university/background-knowledge/proximity-effect-in-microphones-explained/
// Real capsule construction and an accordion's distributed radiation are
// not identified here. Only the existing two point sources' movement is
// considered; the reference response and movement excursions stay fixed.
struct MicrophoneMotion {
    static constexpr double kSoundSpeed = 343.0;
    double reference = 0.0, step = 0.0;
    double delta = 0.0, target = 0.0, integral = 0.0;

    static double Pole(double distance, double incidenceCosine) {
        return kSoundSpeed * incidenceCosine / (distance * (1.0 + incidenceCosine));
    }
    // Both reference sources remain in the front hemisphere at every Width,
    // so reference > 0: a passive, stable reference inverse, even when a
    // moved source briefly crosses a microphone's 90-degree direction.
    void SetReference(double distance, double incidenceCosine, double rate) {
        reference = Pole(distance, incidenceCosine);
        step = 1.0 / (2.0 * rate + reference);
    }
    void Move(double distance, double incidenceCosine) {
        target = Pole(distance, incidenceCosine) - reference;
    }
    void RestoreReference() { target = 0.0; }
    void TakeTarget() { delta = target; }
    void Clear() { delta = target = integral = 0.0; }
    void FlushTiny() { if (Abs(integral) < 1.0e-20) integral = 0.0; }

    // Bilinear one-pole integral, with its physical state retained when
    // Width changes. Return only the correction to the direct sound:
    // Life=0 adds exactly zero, and Room never receives this near-field term.
    double Correction(double input, double glide) {
        delta += glide * (target - delta);
        // Turning Life off must settle to the exact reference, even while
        // reeds keep sounding. Otherwise this glide eventually denormalizes.
        if (target == 0.0 && Abs(delta) < 1.0e-20) delta = 0.0;
        const double v = (input - reference * integral) * step;
        const double low = integral + v;
        integral = low + v;
        return delta * low;
    }
};

} // namespace respira
