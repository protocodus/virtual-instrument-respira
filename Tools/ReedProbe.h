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
        StepCell(reed, reed, chamber, s, p, std::fmin(1.0, i / (0.004 * rate)), dt, rate, 0.0, 0.0, air);
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
