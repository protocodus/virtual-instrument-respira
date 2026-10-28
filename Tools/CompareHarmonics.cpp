// Compares the first twenty harmonics of single 8' notes with the one
// published harmonic measurement of an accordion found so far.
//
//   CompareHarmonics [pressure Pa]
//
// The measurement: M. J. Elejalde-García, E. Macho-Stadler, R. Llanos-Vázquez,
// "Accordion acoustics: A study on pitch bending", Acoustics in Practice
// AiP-2021-02 (EAA), Fig. 4. A Pigini Sirius concert accordion, right manual,
// the 8' register outside the cassotto, each note A#2..A3 played mezzo forte
// (about 70 dBA) on an opening bellows, a microphone 50 cm away; the sound
// pressure level of each of the first twenty harmonics, averaged over the
// twelve notes and fifteen takes each. kMeasured is the mean of the figure's
// two "original sound" series (panels a and b, the same condition before the
// two kinds of bend), read off the figure to about 0.5 dB, relative to the
// fundamental. The paper gives no bellows pressure for mezzo forte, and does
// not say whether it averaged levels or pressures.
//
// Respira plays the same notes on Clarinet Solo (the 8' rank alone), treble on
// every key, the hand pulling steadily at the speed that holds the bellows at
// the given pressure (default the tuning pressure, 400 Pa; the demo phrases'
// strokes are after 250-700 Pa). Each note is analysed over 1-2 s after the
// hand starts, the key down from 0.3 s, on the sum of both channels: a
// Hann-windowed DFT at each harmonic of the sounding pitch, the levels
// averaged over the notes in dB.
#include "DSP/RespiraEngine.h"
#include "DSP/RespiraPresets.h"

#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

using namespace respira;

namespace {

constexpr int kHarmonics = 20, kLow = 46, kHigh = 57;   // A#2..A3
constexpr std::array<double, kHarmonics> kMeasured { {
    0.0, -3.1, -4.0, -10.0, -2.5, -4.4, -3.0, -7.6, -6.9, -9.7,
    -9.9, -12.7, -12.9, -15.5, -12.8, -16.7, -11.2, -14.8, -12.3, -12.6,
} };
constexpr double kRate = 48000.0, kStart = 0.1, kKeyAt = 0.3, kFrom = 1.0, kTo = 2.0;

struct Take {
    std::vector<double> mono;
    double pressure = 0.0;   // mean |bellows pressure| over the analysed second, Pa
};

Take Play(int key, double speed) {
    Engine engine;
    engine.Prepare(kRate);
    Controls controls = kPresets[0].ToControls();
    controls[kKeyboard] = 1.0;   // split, A#2..B2 would be chord buttons
    engine.SetControls(controls);
    engine.Reset();
    Take take;
    const int keyAt = static_cast<int>(kKeyAt * kRate), from = static_cast<int>(kFrom * kRate), to = static_cast<int>(kTo * kRate);
    for (int i = 0; i < to; ++i) {
        if (i == keyAt) {
            const std::uint8_t on[3] = { 0x90, static_cast<std::uint8_t>(key), 80 };
            engine.Midi(on, 3);
        }
        engine.SetHand(kStart + speed * i / kRate);
        float l = 0.0f, r = 0.0f;
        engine.Render(&l, &r, 1);
        if (i >= from) {
            take.mono.push_back(static_cast<double>(l) + r);
            take.pressure += std::fabs(engine.Diag().pressure);
        }
    }
    take.pressure /= static_cast<double>(to - from);
    return take;
}

// Magnitude of the Hann-windowed DFT of x at hz.
double Magnitude(const std::vector<double>& x, double hz) {
    const double step = 2.0 * kPi * hz / kRate, n = static_cast<double>(x.size());
    double re = 0.0, im = 0.0;
    for (std::size_t i = 0; i < x.size(); ++i) {
        const double w = 0.5 - 0.5 * std::cos(2.0 * kPi * static_cast<double>(i) / n);
        re += w * x[i] * std::cos(step * static_cast<double>(i));
        im -= w * x[i] * std::sin(step * static_cast<double>(i));
    }
    return std::sqrt(re * re + im * im);
}

// The largest magnitude within +-spread of hz, and where it is.
double Peak(const std::vector<double>& x, double hz, double spread, double* at = nullptr) {
    double best = 0.0;
    for (int k = -20; k <= 20; ++k) {
        const double f = hz * (1.0 + spread * k / 20.0), m = Magnitude(x, f);
        if (m > best) {
            best = m;
            if (at) *at = f;
        }
    }
    return best;
}

double Mean(const std::array<double, kHarmonics>& h, int first, int last) {
    double sum = 0.0;
    for (int n = first; n <= last; ++n) sum += h[static_cast<std::size_t>(n - 1)];
    return sum / (last - first + 1);
}

} // namespace

int main(int argc, char** argv) {
    const double target = argc > 1 ? std::atof(argv[1]) : kTuningPressure;
    if (!(target >= 100.0 && target <= 1500.0)) {
        std::fprintf(stderr, "usage: CompareHarmonics [pressure Pa, 100..1500]\n");
        return 2;
    }
    std::array<double, kHarmonics> model {};
    double pressure = 0.0;
    for (int key = kLow; key <= kHigh; ++key) {
        // The bellows pressure grows a little faster than the hand's speed.
        double speed = 0.11 * target / 400.0;
        Take take;
        for (int i = 0; i < 5; ++i) {
            take = Play(key, speed);
            speed *= target / take.pressure;
        }
        pressure += take.pressure / (kHigh - kLow + 1);
        double f0 = 440.0 * std::pow(2.0, (key - 69) / 12.0);
        const double h1 = Peak(take.mono, f0, 0.03, &f0);
        for (int n = 1; n <= kHarmonics; ++n)
            model[static_cast<std::size_t>(n - 1)] += 20.0 * std::log10(Peak(take.mono, n * f0, 0.005) / h1) / (kHigh - kLow + 1);
    }
    std::printf("A#2..A3 on the 8' rank at %.0f Pa (mean over the notes), dB re the fundamental\n", pressure);
    std::printf("harmonic  ");
    for (int n = 1; n <= kHarmonics; ++n) std::printf("%6d", n);
    std::printf("\nmeasured  ");
    for (double v : kMeasured) std::printf("%6.1f", v);
    std::printf("\nRespira   ");
    for (double v : model) std::printf("%6.1f", v);
    std::printf("\ndifference");
    for (int n = 0; n < kHarmonics; ++n) std::printf("%6.1f", model[static_cast<std::size_t>(n)] - kMeasured[static_cast<std::size_t>(n)]);
    const auto bands = [](const char* name, const std::array<double, kHarmonics>& h) {
        std::printf("%-9s%6.1f%6.1f%6.1f%6.1f\n", name, Mean(h, 2, 4), Mean(h, 5, 8), Mean(h, 9, 14), Mean(h, 15, 20));
    };
    std::printf("\n\nmean of     2-4   5-8  9-14 15-20\n");
    bands("measured", kMeasured);
    bands("Respira", model);
    return 0;
}
