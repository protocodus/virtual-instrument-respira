// Compares the first twenty harmonics of single 8' notes with the one
// published harmonic measurement of an accordion found so far.
//
//   CompareHarmonics [pressure Pa]
//
// The measurement: M. J. Elejalde-García, E. Macho-Stadler, R. Llanos-Vázquez,
// "Accordion acoustics: A study on pitch bending", Acoustics in Practice
// AiP-2021-02 (EAA), Fig. 4. A Pigini Sirius concert accordion, right manual,
// the 8' register outside the cassotto, each note A#2..A3 played mezzo forte
// (about 70 dBA on a sound level meter 50 cm away) on an opening bellows, a
// microphone 50 cm away; the sound pressure level of each of the first twenty
// harmonics, averaged over the twelve notes and fifteen takes each. kMeasured
// is the mean of the figure's two "original sound" series (panels a and b,
// the same condition before the two kinds of bend), read off the figure to
// about 0.5 dB, relative to the fundamental. The paper gives no bellows
// pressure, and does not say whether it averaged levels or pressures.
//
// Respira plays each note as ProbeReed does: one 8' cell, its pallet open, at
// a steady bellows pressure, here drawing (the pull reed speaks). Without an
// argument the pressure is the one at which the twelve notes read 70 dBA at
// 50 cm on average, the radiated flow's rate of change taken as a small
// source in free field (the engine's output reference, 1 m away, is the same
// source); a real accordion's body and room would read louder at the same
// pressure. Each note is analysed over its last half second: a Hann-windowed
// DFT at each harmonic of the sounding pitch, the levels averaged in dB.
//
// The same paper bent those notes by closing the pallet part way at a steady
// bellows pressure ("only finger", Tables 1 and 2): 16.1 cents flat and
// 11.1 dB quieter on average, the level being the mean over the first twenty
// harmonics. Respira's cells are bent at the same pressure with the pallet
// opened 0.2, as full channel pressure holds it (kHalfPressed, RespiraEngine.cpp).
#include "ReedProbe.h"
#include "DSP/ReedTuning.h"

#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

using namespace respira;

namespace {

constexpr int kHarmonics = 20, kLevelHarmonics = 40, kLow = 46, kHigh = 57;   // A#2..A3
constexpr std::array<double, kHarmonics> kMeasured { {
    0.0, -3.1, -4.0, -10.0, -2.5, -4.4, -3.0, -7.6, -6.9, -9.7,
    -9.9, -12.7, -12.9, -15.5, -12.8, -16.7, -11.2, -14.8, -12.3, -12.6,
} };
constexpr double kMeasuredDbA = 70.0, kDistance = 0.5, kSeconds = 2.0, kAnalysed = 0.5;
constexpr double kMeasuredBendCents = -16.1, kMeasuredBendDb = -11.1, kHalfPressedOpening = 0.2;

// Radiated source strength (m^3/s^2) of `key`'s 8' cell over its last
// kAnalysed seconds at bellows pressure -pressure, the pallet opened `opening`.
std::vector<double> Play(int key, double pressure, double opening) {
    const CellSpec spec = MakeCell(kRankM, key, 0.0, kReedTuning[kRankM][TuningIndex(kRankM, key)]);
    return probe::Radiated(spec, -pressure, opening, kSeconds, kAnalysed);
}

using probe::Amplitude;
using probe::Pitch;

// IEC 61672 A-weighting, dB.
double AWeight(double f) {
    const double f2 = f * f;
    const double ra = 12194.0 * 12194.0 * f2 * f2
                    / ((f2 + 20.6 * 20.6) * std::sqrt((f2 + 107.7 * 107.7) * (f2 + 737.9 * 737.9)) * (f2 + 12194.0 * 12194.0));
    return 20.0 * std::log10(ra) + 2.0;
}

struct Note {
    std::array<double, kHarmonics> harmonics {};   // dB re the fundamental
    double dbA = 0.0;                               // at kDistance in free field
    double cents = 0.0;                             // sounding pitch re the key's
    double level = 0.0;                             // mean of the harmonics' levels, dB
};

Note Analyse(int key, double pressure, double opening) {
    const std::vector<double> x = Play(key, pressure, opening);
    const double nominal = 440.0 * std::pow(2.0, (key - 69) / 12.0);
    const double f0 = Pitch(x, nominal), h1 = Amplitude(x, f0);
    Note note;
    note.cents = 1200.0 * std::log2(f0 / nominal);
    double power = 0.0;
    for (int n = 1; n <= kLevelHarmonics; ++n) {
        const double a = Amplitude(x, n * f0);
        if (n <= kHarmonics) {
            note.harmonics[static_cast<std::size_t>(n - 1)] = 20.0 * std::log10(a / h1);
            note.level += 20.0 * std::log10(a) / kHarmonics;
        }
        const double rms = kRho / (4.0 * kPi * kDistance) * a / std::sqrt(2.0);
        power += rms * rms * std::pow(10.0, AWeight(n * f0) / 10.0);
    }
    note.dbA = 10.0 * std::log10(power / (20.0e-6 * 20.0e-6));
    return note;
}

Note AnalyseAll(double pressure, double opening = 1.0) {
    Note mean;
    for (int key = kLow; key <= kHigh; ++key) {
        const Note note = Analyse(key, pressure, opening);
        for (int n = 0; n < kHarmonics; ++n)
            mean.harmonics[static_cast<std::size_t>(n)] += note.harmonics[static_cast<std::size_t>(n)] / (kHigh - kLow + 1);
        mean.dbA += note.dbA / (kHigh - kLow + 1);
        mean.cents += note.cents / (kHigh - kLow + 1);
        mean.level += note.level / (kHigh - kLow + 1);
    }
    return mean;
}

double Mean(const std::array<double, kHarmonics>& h, int first, int last) {
    double sum = 0.0;
    for (int n = first; n <= last; ++n) sum += h[static_cast<std::size_t>(n - 1)];
    return sum / (last - first + 1);
}

} // namespace

int main(int argc, char** argv) {
    double pressure = argc > 1 ? std::atof(argv[1]) : 0.0;
    if (argc > 1 && !(pressure >= 100.0 && pressure <= 2000.0)) {
        std::fprintf(stderr, "usage: CompareHarmonics [pressure Pa, 100..2000]\n");
        return 2;
    }
    Note model;
    if (argc > 1) {
        model = AnalyseAll(pressure);
    } else {
        // Bisect in log pressure for the measurement's loudness.
        double low = 50.0, high = 2000.0;
        for (int i = 0; i < 8; ++i) {
            pressure = std::sqrt(low * high);
            model = AnalyseAll(pressure);
            (model.dbA < kMeasuredDbA ? low : high) = pressure;
        }
    }
    std::printf("A#2..A3 on the 8' rank at %.0f Pa, %.1f dBA at %.1f m; dB re the fundamental\n", pressure, model.dbA, kDistance);
    std::printf("harmonic  ");
    for (int n = 1; n <= kHarmonics; ++n) std::printf("%6d", n);
    std::printf("\nmeasured  ");
    for (double v : kMeasured) std::printf("%6.1f", v);
    std::printf("\nRespira   ");
    for (double v : model.harmonics) std::printf("%6.1f", v);
    std::printf("\ndifference");
    for (int n = 0; n < kHarmonics; ++n)
        std::printf("%6.1f", model.harmonics[static_cast<std::size_t>(n)] - kMeasured[static_cast<std::size_t>(n)]);
    const auto bands = [](const char* name, const std::array<double, kHarmonics>& h) {
        std::printf("%-9s%6.1f%6.1f%6.1f%6.1f\n", name, Mean(h, 2, 4), Mean(h, 5, 8), Mean(h, 9, 14), Mean(h, 15, 20));
    };
    std::printf("\n\nmean of     2-4   5-8  9-14 15-20\n");
    bands("measured", kMeasured);
    bands("Respira", model.harmonics);
    const Note bent = AnalyseAll(pressure, kHalfPressedOpening);
    std::printf("\nbent, pallet opened %.1f   cents    dB\n", kHalfPressedOpening);
    std::printf("%-23s%8.1f%6.1f\n", "measured (only finger)", kMeasuredBendCents, kMeasuredBendDb);
    std::printf("%-23s%8.1f%6.1f\n", "Respira", bent.cents - model.cents, bent.level - model.level);
    return 0;
}
