// Independent microphone oracle: two spherical-wave pressure samples and
// an internal acoustic delay (Zadvitskiy, DAGA 2024, equations 2-8). Shrink
// the port spacing to 0.2 mm to test the point-gradient limit used by the
// engine, rather than repeating its pole/filter calculation. Compare the
// measured complex response at 32-512 Hz; the 1e-4 tolerance includes finite
// port spacing, bilinear frequency warping and residual settling error.
#include "DSP/MicrophoneMotion.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdio>
#include <cstdlib>

namespace {
constexpr double pi = 3.14159265358979323846;
using Complex = std::complex<double>;
void Require(bool ok, const char* what) {
    if (!ok) { std::fprintf(stderr, "FAIL MicrophoneMotion: %s\n", what); std::exit(1); }
}
Complex Capsule(double distance, double cosine, double hz) {
    constexpr double d = 0.0002, soundSpeed = 343.0;
    const double front = std::sqrt(distance * distance + d * d / 4.0 - distance * d * cosine);
    const double rear = std::sqrt(distance * distance + d * d / 4.0 + distance * d * cosine);
    const double k = 2.0 * pi * hz / soundSpeed;
    return distance / front * std::exp(Complex(0.0, -k * (front - distance)))
         - distance / rear * std::exp(Complex(0.0, -k * (rear - distance + d)));
}
void GeometryPredictsTheMeasuredResponse() {
    double worst = 0.0, strongest = 0.0;
    bool crossedNinetyDegrees = false;
    for (double rate : { 22050.0, 48000.0, 96000.0 })
        for (double width : { 0.0, 0.6, 1.0 })
            for (double x0 : { 0.20, -0.25 })
              for (double channel : { -1.0, 1.0 }) {
                const double angle = width * 75.0 * pi / 180.0;
                const double r0 = std::hypot(x0, 1.0), c0 = (std::cos(angle) + channel * x0 * std::sin(angle)) / r0;
                // Both directions, including the bass end's existing travel.
                for (double direction : { -1.0, 1.0 }) {
                    const double x = x0 + direction * (x0 < 0.0 ? 0.0885 : 0.05), z = 1.0 + direction * 0.08;
                    const double r = std::hypot(x, z), c = (z * std::cos(angle) + channel * x * std::sin(angle)) / r;
                    crossedNinetyDegrees = crossedNinetyDegrees || c < 0.0;
                    for (double hz : { 32.0, 64.0, 128.0, 256.0, 512.0 }) {
                        respira::MicrophoneMotion mic;
                        mic.SetReference(r0, c0, rate); mic.Move(r, c); mic.TakeTarget();
                        Require(mic.reference > 0.0, "every reference pole must remain stable");
                        // Integer numbers of cycles give orthogonal quadratures.
                        const int frames = static_cast<int>(rate);
                        const int warm = static_cast<int>(std::ceil(24.0 / mic.reference) * rate);
                        Complex measured = 0.0;
                        for (int i = 0; i < warm + frames; ++i) {
                            const double phase = 2.0 * pi * hz * i / rate;
                            const double input = std::cos(phase), out = input + mic.Correction(input, 1.0);
                            Require(std::isfinite(out), "finite output");
                            if (i >= warm) measured += out * std::exp(Complex(0.0, -phase));
                        }
                        measured *= 2.0 / frames;
                        const Complex expected = Capsule(r, c, hz) / Capsule(r0, c0, hz) * (1.0 + c0) / (1.0 + c);
                        worst = std::max(worst, std::abs(measured - expected));
                        strongest = std::max(strongest, std::abs(expected - Complex(1.0, 0.0)));
                    }
                }
            }
    std::printf("MicrophoneMotion two-port oracle: worst complex error %.9g; largest effect %.6g\n", worst, strongest);
    Require(crossedNinetyDegrees, "both channels must include off-axis bass movement at maximum Width");
    Require(strongest > 0.03, "the fixture must expose the missing proximity response");
    Require(worst < 1.0e-4, "rendered amplitude and phase agree with independent capsule geometry");
}
void NeutralIsExactAndClearRestoresSilence() {
    respira::MicrophoneMotion mic;
    mic.SetReference(1.02, 0.7, 48000.0);
    for (int i = 0; i < 4800; ++i) {
        const double x = std::sin(i * .03);
        Require(mic.Correction(x, .001) == 0.0, "reference position adds exactly zero");
    }
    mic.Move(.92, .72); mic.TakeTarget();
    Require(mic.Correction(.25, 1.0) != 0.0, "moved microphone has an active correction");
    mic.Clear();
    for (int i = 0; i < 4800; ++i) Require(mic.Correction(0.0, .001) == 0.0, "clear leaves no stored sound");
}
void ReturningToReferenceSettlesExactly() {
    // Life can be turned off during an indefinitely held note. The geometry
    // glide must finish at the neutral response instead of retaining a
    // subnormal coefficient; a sounding cell does not take the idle flush.
    for (double rate : { 22050.0, 48000.0, 96000.0 }) {
        respira::MicrophoneMotion mic;
        mic.SetReference(1.02, 0.7, rate);
        mic.Move(.92, .72); mic.TakeTarget();
        const double glide = 1.0 - std::exp(-1.0 / (.02 * rate));
        mic.RestoreReference();
        for (int i = 0; i < static_cast<int>(2.0 * rate); ++i)
            Require(std::isfinite(mic.Correction(std::sin(i * .03), glide)), "finite reference transition");
        Require(mic.delta == 0.0, "reference transition ends exactly, without a denormal coefficient");
        Require(mic.Correction(.25, glide) == 0.0, "held input is neutral after Life turns off");
    }
}
}
int main() {
    GeometryPredictsTheMeasuredResponse();
    NeutralIsExactAndClearRestoresSilence();
    ReturningToReferenceSettlesExactly();
    std::puts("PASS MicrophoneMotion: independent geometry, rates, exact reference, reset and held-note bypass");
}
