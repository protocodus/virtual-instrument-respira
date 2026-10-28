// Compares how single 8' notes' pitch and tone follow their loudness with a
// bandoneon's.
//
//   CompareBandoneon [top pressure Pa]
//
// The measurement: Tools/measure_bandoneon.py on BANDONEON v1 by Miguel Lopez
// (Pianobook), held notes C2..C6 whose loudness the player moves with the
// bellows: per dB of level, how many cents the pitch moves and how many dB the
// odd harmonics from the 3rd to the 15th gain on the fundamental, over the
// loudest 30 dB of each take (and the pitch over the loudest 15 dB). Read against loudness,
// neither needs the bellows pressure, which the recordings do not give, and
// neither depends on the microphone or the room. A bandoneon is not an
// accordion: each note sounds two reeds an octave apart, so only its odd
// harmonics are the note's own reed.
//
// Respira plays each note as CompareHarmonics does, one 8' cell drawing with
// its pallet open, at steady pressures a quarter octave apart up to the top
// pressure (default 1500 Pa, a hard push), and fits the same slopes over the
// steady states within 30 (15) dB of the top one's level: the radiated flow's
// RMS level, the pitch and harmonics over each note's last 0.25 s or 30
// periods. Which pressure the recordings' loudest frames were played at is
// not known, so the top pressure is a choice; the slopes move with it.
#include "ReedProbe.h"
#include "DSP/ReedTuning.h"

#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

using namespace respira;

namespace {

constexpr int kOdd = 7;   // the 3rd, 5th .. 15th harmonics

struct Measured {
    int key;
    double centsPerDb, centsPerDbLoud;
    std::array<double, kOdd> odd;
};

// Tools/measure_bandoneon.py, 2026-10-28.
constexpr std::array<Measured, 15> kMeasured { {
    { 36, -0.59, -1.13, { 0.68, 0.55, 0.73, 0.93, 0.94, 0.64, 1.02 } },
    { 40, -0.39, -0.49, { 0.63, 0.76, 0.76, 0.96, 0.93, 0.50, 0.62 } },
    { 43, -0.42, -0.66, { 0.53, 0.53, 0.73, 0.98, 0.60, 0.62, 0.38 } },
    { 47, -0.34, -0.52, { 0.64, 0.75, 0.48, 0.84, 0.52, 0.24, 0.35 } },
    { 50, -0.62, -0.61, { 0.92, 1.13, 1.03, 1.06, 0.86, 0.97, 1.03 } },
    { 53, -0.49, -0.57, { 0.42, 0.39, 0.69, 0.21, 0.74, 0.78, 0.41 } },
    { 57, -0.42, -0.44, { 0.62, 0.72, 0.43, 0.33, 0.54, 0.35, 0.38 } },
    { 60, -0.39, -0.42, { 0.90, 0.42, 0.55, 0.78, 0.69, 0.60, 0.56 } },
    { 64, -0.29, -0.39, { 0.70, 0.73, 0.57, 0.92, 0.72, 0.66, 0.94 } },
    { 67, -0.24, -0.17, { 0.55, 0.66, 0.84, 0.79, 1.06, 1.19, 1.55 } },
    { 71, -0.28, -0.19, { 0.47, 0.60, 0.63, 0.89, 1.60, 1.47, 0.91 } },
    { 74, -0.30, -0.31, { 0.87, 0.89, 1.06, 1.50, 1.37, 1.66, 1.52 } },
    { 77, -0.33, -0.32, { 0.65, 0.18, 0.67, 1.13, 1.51, 1.40, 1.30 } },
    { 81, -0.36, -0.42, { 1.21, 1.34, 1.72, 1.92, 1.42, 1.67, 2.10 } },
    { 84, -0.37, -0.45, { 0.83, -0.00, 1.05, 1.25, 1.78, 1.99, 1.97 } },
} };

struct State {
    double level, cents;
    std::array<double, kOdd> odd;   // h3, h5 .. h15 re h1, dB
};

std::vector<State> Sweep(int key, double top) {
    const CellSpec spec = MakeCell(kRankM, key, 0.0, kReedTuning[kRankM][TuningIndex(kRankM, key)]);
    const double analysed = std::fmax(0.25, 30.0 / spec.hz);
    std::vector<State> states;
    for (double p = 25.0; p <= top * 1.001; p *= std::pow(2.0, 0.25)) {
        const std::vector<double> x = probe::Radiated(spec, -p, 1.0, 0.6 + analysed + 40.0 / spec.hz, analysed);
        double power = 0.0;
        for (const double v : x) power += v * v;
        if (!(power > 0.0)) continue;
        const double f0 = probe::Pitch(x, spec.hz), h1 = probe::Amplitude(x, f0);
        State s { 10.0 * std::log10(power / static_cast<double>(x.size())), 1200.0 * std::log2(f0 / spec.hz), {} };
        for (int i = 0; i < kOdd; ++i) s.odd[static_cast<std::size_t>(i)] = 20.0 * std::log10(probe::Amplitude(x, (3 + 2 * i) * f0) / h1);
        states.push_back(s);
    }
    return states;
}

// Least-squares slope of y against level over the states within span dB of
// the last (loudest) one.
template <typename Y>
double Slope(const std::vector<State>& states, double span, Y y) {
    const double top = states.back().level;
    double n = 0.0, sx = 0.0, sy = 0.0, sxx = 0.0, sxy = 0.0;
    for (const State& s : states) {
        if (s.level < top - span) continue;
        const double v = y(s);
        n += 1.0;
        sx += s.level;
        sy += v;
        sxx += s.level * s.level;
        sxy += s.level * v;
    }
    return (n * sxy - sx * sy) / (n * sxx - sx * sx);
}

} // namespace

int main(int argc, char** argv) {
    const double top = argc > 1 ? std::atof(argv[1]) : 1500.0;
    if (!(top >= 200.0 && top <= kMaxPressure)) {
        std::fprintf(stderr, "usage: CompareBandoneon [top pressure Pa, 200..%.0f]\n", kMaxPressure);
        return 2;
    }
    std::printf("8' cells up to %.0f Pa against BANDONEON v1 (Miguel Lopez, Pianobook)\n", top);
    std::printf("per dB of level, bandoneon / Respira: the pitch in cents over the loudest 30 and 15 dB,\n"
                "then the 3rd, 5th .. 15th harmonics in dB on the fundamental over the loudest 30 dB\n");
    std::printf("key    c30          c15          h3           h5           h7           h9           h11          h13          h15\n");
    constexpr std::size_t kColumns = 2 + kOdd;
    std::array<double, kColumns> error {}, absError {};
    for (const Measured& m : kMeasured) {
        const std::vector<State> states = Sweep(m.key, top);
        std::array<double, kColumns> model {}, measured {};
        model[0] = Slope(states, 30.0, [](const State& s) { return s.cents; });
        model[1] = Slope(states, 15.0, [](const State& s) { return s.cents; });
        measured[0] = m.centsPerDb;
        measured[1] = m.centsPerDbLoud;
        for (std::size_t i = 0; i < kOdd; ++i) {
            model[2 + i] = Slope(states, 30.0, [i](const State& s) { return s.odd[i]; });
            measured[2 + i] = m.odd[i];
        }
        std::printf("%3d  ", m.key);
        for (std::size_t i = 0; i < kColumns; ++i) {
            std::printf(" %5.2f %5.2f  ", measured[i], model[i]);
            error[i] += (model[i] - measured[i]) / kMeasured.size();
            absError[i] += std::fabs(model[i] - measured[i]) / kMeasured.size();
        }
        std::printf("\n");
    }
    std::printf("mean error   ");
    for (const double e : error) std::printf("    %6.2f   ", e);
    std::printf("\nmean |error| ");
    for (const double e : absError) std::printf("    %6.2f   ", e);
    std::printf("\n");
    return 0;
}
