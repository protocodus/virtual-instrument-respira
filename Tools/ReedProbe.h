#pragma once

// Offline measurements of single reed cells, for the tuning generator, the
// probe and the tests. Not for the audio thread: it allocates and runs whole
// seconds of simulation.

#include "DSP/ReedBank.h"

#include <array>
#include <cmath>
#include <vector>

namespace respira::probe {

// Rate the tuning table is made at: the engine's internal rate at 48 kHz.
// Pitch is rate-independent here (96, 192 and 768 kHz agree), so the table
// serves every host rate.
constexpr double kRate = 192000.0;

// Frequency of a push reed (with its chamber) speaking at bellows pressure p,
// from the tongue's upward crossings of its mean once it has settled; 0 if it
// does not speak.
inline double SoundingHz(const ReedSpec& reed, const ChamberSpec& chamber, double nominalHz, double p,
                         double rate = kRate) {
    CellState s;
    s.push.x = s.pull.x = reed.rest;
    const double dt = 1.0 / rate;
    const int n = static_cast<int>(rate * std::fmin(1.6, 0.35 + 60.0 / nominalHz));
    const int from = n * 6 / 10;
    std::vector<double> xs(static_cast<std::size_t>(n - from));
    for (int i = 0; i < n; ++i) {
        double air = 0.0;
        StepCell(reed, reed, chamber, MakeVent(chamber, std::fmin(1.0, i / (0.004 * rate)), rate), s, p, dt, rate, 0.0, 0.0, air);
        if (i >= from) xs[static_cast<std::size_t>(i - from)] = s.push.x;
    }
    double mean = 0.0, low = 1.0, high = -1.0;
    for (double x : xs) { mean += x; low = std::fmin(low, x); high = std::fmax(high, x); }
    mean /= static_cast<double>(xs.size());
    if (high - low < 0.2 * reed.plate) return 0.0;
    double first = -1.0, last = -1.0, previous = xs[0];
    int crossings = 0;
    for (std::size_t i = 1; i < xs.size(); ++i) {
        if (previous < mean && xs[i] >= mean) {
            const double t = (static_cast<double>(i) - 1.0 + (mean - previous) / (xs[i] - previous)) / rate;
            if (first < 0.0) first = t;
            last = t;
            ++crossings;
        }
        previous = xs[i];
    }
    return crossings > 2 ? (crossings - 1) / (last - first) : 0.0;
}

// Radiated source strength (m^3/s^2) of one cell over its last `analysed` of
// `seconds` at a steady bellows pressure pb (< 0 draws: the pull reed
// speaks), the pallet opened `opening` over its first 4 ms.
inline std::vector<double> Radiated(const CellSpec& spec, double pb, double opening, double seconds, double analysed) {
    CellState s;
    s.push.x = spec.push.rest;
    s.pull.x = spec.pull.rest;
    const double rate = kRate, dt = 1.0 / rate;
    const int n = static_cast<int>(seconds * rate), from = n - static_cast<int>(analysed * rate);
    std::vector<double> out;
    for (int i = 0; i < n; ++i) {
        double air = 0.0;
        const double radiated = StepCell(spec.push, spec.pull, spec.chamber, MakeVent(spec.chamber, opening * std::fmin(1.0, i / (0.004 * rate)), rate),
                                         s, pb, dt, rate, 0.0, 0.0, air);
        if (i >= from) out.push_back(radiated);
    }
    return out;
}

// Amplitude of the sinusoid at hz in x (sampled at kRate): a Hann-windowed
// DFT, its phasors turned by recurrence.
inline double Amplitude(const std::vector<double>& x, double hz) {
    const double n = static_cast<double>(x.size()), step = 2.0 * kPi * hz / kRate, window = 2.0 * kPi / n;
    const double cs = std::cos(step), sn = std::sin(step), wc = std::cos(window), ws = std::sin(window);
    double re = 0.0, im = 0.0, c = 1.0, s = 0.0, hc = 1.0, hs = 0.0;
    for (const double v : x) {
        const double w = (0.5 - 0.5 * hc) * v;
        re += w * c;
        im -= w * s;
        const double c2 = c * cs - s * sn, h2 = hc * wc - hs * ws;
        s = s * cs + c * sn;
        hs = hs * wc + hc * ws;
        c = c2;
        hc = h2;
    }
    return 4.0 * std::sqrt(re * re + im * im) / n;   // the Hann window sums to n / 2
}

// The sounding pitch near hz: the fundamental's peak, found coarse then fine.
// A free reed's partials are harmonic, so the rest are read at its multiples.
inline double Pitch(const std::vector<double>& x, double hz) {
    for (const double spread : { 0.03, 0.0015 }) {
        double best = 0.0, at = hz;
        for (int k = -20; k <= 20; ++k) {
            const double f = hz * (1.0 + spread * k / 20.0), a = Amplitude(x, f);
            if (a > best) {
                best = a;
                at = f;
            }
        }
        hz = at;
    }
    return hz;
}

// natural / wanted for the push and pull reeds of one cell, found by three
// passes of retuning at kTuningPressure (3x that if a reed will not speak).
// A reed that never speaks keeps ratio 1.
inline std::array<double, 2> TuneCell(int rank, int key) {
    const CellSpec base = MakeCell(rank, key, 0.0, nullptr);
    std::array<double, 2> ratios { 1.0, 1.0 };
    for (int side = 0; side < 2; ++side) {
        const ReedVariation v = Variation(rank, TuningIndex(rank, key), side);
        const double wanted = base.hz * std::pow(2.0, v.tuneCents / 1200.0);
        const double q = ReedQ(base.hz) * v.qScale;
        double natural = wanted;
        for (int pass = 0; pass < 3; ++pass) {
            const ReedSpec reed = MakeReed(natural, q, v.setScale);
            double sounding = SoundingHz(reed, base.chamber, natural, kTuningPressure);
            if (sounding <= 0.0) sounding = SoundingHz(reed, base.chamber, natural, 3.0 * kTuningPressure);
            if (sounding <= 0.0) break;
            natural *= wanted / sounding;
        }
        ratios[static_cast<std::size_t>(side)] = natural / wanted;
    }
    return ratios;
}

} // namespace respira::probe
