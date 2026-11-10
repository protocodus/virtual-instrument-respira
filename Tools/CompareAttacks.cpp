// Reproducible single-cell finger-attack measurements, not a realism test.
//
//   CompareAttacks [--note A2|A3|A4|A5] [--target 55|70]
//                  [--seconds 6] [--pallet-ms 3] [--trace-dir directory]
//   CompareAttacks --self-test
//
// Reference: Llanos-Vazquez et al., "Physical and Psychoacoustic
// Characterization of the Different Types of Attacks on the Accordion",
// Acta Acustica united with Acustica 100 (2014), 375-384, Table I.
// DOI: https://doi.org/10.3813/AAA.918716
// Primary text: https://dael.euracoustics.org/bin/EAA/aaua_dl?document_id=45451
// The Pigini Sirius's outer 8' right-hand register was played on the pull,
// at 55/70 dBA about 0.5 m in front. For finger attacks the player first
// tensioned the bellows, then pressed the key as fast as possible. Harmonic
// attack time is the interval from -50 to -5 dB relative to that harmonic's
// maximum, excluding the trigger-to--50 dB delay. Time precision was 10 ms;
// at least 15 takes were made. The paper does not specify the temporal FFT
// window/overlap, so an exact replication of its estimator is not possible.
//
// This tool uses the current tuned M cell, at the tuning rate, with fixed negative
// bellows pressure, its chamber initially at that pressure (Engine::StartCell),
// no turbulence, and the engine's raised-cosine pallet opening. The default
// 3 ms is the engine's fastest stroke (velocity 127), not a measured pallet
// speed. Slow-pallet experiments accept up to 500 ms; this is an input
// sweep, not a measured mechanical range. --pallet-ms 6 reproduces the travel duration of earlier repo probes.
// It is a deterministic cell experiment, not the full engine: no mechanical
// output, grille, room, output compression, or bellows-pressure feedback.
//
// Each note is separately pressure-calibrated from its final 0.5 s, using
// ReedProbe's Hann harmonic amplitudes and the same free-field monopole
// reference as CompareHarmonics. A-weighted power sums up to 40 harmonics
// below 20 kHz. This inferred source SPL is NOT a measured SPL of Respira or
// the reference accordion, whose body/room/directivity also affect level.
// The preceding half-second's level difference exposes incomplete settling.
//
// The fundamental tracker is a centered, zero-padded Hann DFT at the final
// sounding pitch, sampled every 1 ms, with 2/4/8-cycle windows. Timing is at
// the window center; onset can consequently appear before the key. Report
// that apparent latency, and the window-sensitivity range, rather than
// interpreting a smoothed tracker as exact physical onset. Thresholds use
// the maximum tracked fundamental over the whole take. Both first -50 dB
// crossing and the last upward -50 dB crossing before the first -5 dB are
// reported: their difference exposes a separate opening transient or dip.
// No assertion requires the model to match the reference instrument.

#include "ReedProbe.h"
#include "DSP/ReedTuning.h"

#include <algorithm>
#include <array>
#include <complex>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace respira;

namespace {

constexpr double kDistance = 0.5, kAnalysed = 0.5, kHop = 0.001;
struct Reference { const char* name; int key; double pianoMs, forteMs; };
// Table I's finger columns, p then mf, in ms (not its bellows columns).
constexpr std::array<Reference, 4> kReferences {{{"A2", 45, 110, 80},
    {"A3", 57, 90, 110}, {"A4", 69, 90, 60}, {"A5", 81, 60, 50}}};

struct Options {
    double seconds = 6.0, palletMs = 3.0, target = 0.0;
    std::string note, traceDir;
    bool selfTest = false;
};

std::vector<double> Play(int key, double pressure, const Options& options) {
    const CellSpec spec = MakeCell(kRankM, key, 0.0,
        kReedTuning[kRankM][TuningIndex(kRankM, key)], probe::kRate);
    CellState state;
    state.push.x = spec.push.rest;
    state.pull.x = spec.pull.rest;
    for (double& p : state.p) p = -pressure;
    const int count = static_cast<int>(options.seconds * probe::kRate);
    const double travelSamples = options.palletMs * 0.001 * probe::kRate;
    std::vector<double> out(static_cast<std::size_t>(count));
    Vent vent;
    for (int i = 0; i < count; ++i) {
        if (i <= travelSamples) {
            const double travel = std::min(1.0, (i + 1.0) / travelSamples);
            vent = MakeVent(spec.chamber, 0.5 - 0.5 * std::cos(kPi * travel), probe::kRate);
        }
        double air = 0.0;
        const double value = StepCell(spec.push, spec.pull, spec.chamber, vent,
            state, -pressure, 1.0 / probe::kRate, probe::kRate, 0.0, 0.0, air);
        if (!std::isfinite(value)) throw std::runtime_error("nonfinite cell output");
        out[static_cast<std::size_t>(i)] = value;
    }
    return out;
}

double AWeight(double f) {
    const double f2 = f * f;
    const double ra = 12194.0 * 12194.0 * f2 * f2 /
        ((f2 + 20.6 * 20.6) * std::sqrt((f2 + 107.7 * 107.7) *
        (f2 + 737.9 * 737.9)) * (f2 + 12194.0 * 12194.0));
    return 20.0 * std::log10(ra) + 2.0;
}

double Level(const std::vector<double>& x, double hz) {
    double power = 0.0;
    for (int h = 1; h <= 40 && h * hz <= 20000.0; ++h) {
        const double rms = kRho / (4.0 * kPi * kDistance) *
            probe::Amplitude(x, h * hz) / std::sqrt(2.0);
        power += rms * rms * std::pow(10.0, AWeight(h * hz) / 10.0);
    }
    return power > 0.0 ? 10.0 * std::log10(power / (20.0e-6 * 20.0e-6)) : -300.0;
}

struct Take { std::vector<double> samples; double pressure, hz, dbA, settlingDb; };

Take Measure(int key, double pressure, const Options& options) {
    Take take {Play(key, pressure, options), pressure, 0.0, 0.0, 0.0};
    const auto count = static_cast<std::ptrdiff_t>(kAnalysed * probe::kRate);
    const std::vector<double> last(take.samples.end() - count, take.samples.end());
    const std::vector<double> previous(take.samples.end() - 2 * count, take.samples.end() - count);
    take.hz = probe::Pitch(last, 440.0 * std::pow(2.0, (key - 69) / 12.0));
    take.dbA = Level(last, take.hz);
    take.settlingDb = take.dbA - Level(previous, take.hz);
    return take;
}

Take Calibrate(int key, double target, const Options& options) {
    // Find the first ascending bracket, rather than assuming a high pressure
    // still sounds: a sufficiently hard pull can close a reed permanently.
    double low = 1.0, high = 25.0;
    Take best = Measure(key, low, options);
    if (best.dbA > target) throw std::runtime_error("target below calibration range");
    for (;;) {
        Take next = Measure(key, high, options);
        const bool bracketed = next.dbA >= target;
        if (std::fabs(next.dbA - target) < std::fabs(best.dbA - target)) best = std::move(next);
        if (bracketed) break;
        low = high;
        if (high >= 2000.0) throw std::runtime_error("target not bracketed below 2000 Pa");
        high = std::min(2000.0, high * 1.5);
    }
    for (int i = 0; i < 16 && std::fabs(best.dbA - target) > 0.02; ++i) {
        const double pressure = std::sqrt(low * high);
        Take next = Measure(key, pressure, options);
        (next.dbA < target ? low : high) = pressure;
        if (std::fabs(next.dbA - target) < std::fabs(best.dbA - target)) best = std::move(next);
    }
    if (std::fabs(best.dbA - target) > 0.05)
        throw std::runtime_error("pressure calibration did not converge within 0.05 dB");
    return best;
}

struct Point { double seconds, amplitude; };

std::vector<Point> Envelope(const std::vector<double>& x, double hz, int cycles) {
    // Three prefix DFT sums give an exact centered Hann at every requested
    // time in O(samples), without a long convolution at low fundamentals.
    using Complex = std::complex<double>;
    const int width = 2 * static_cast<int>(std::lround(cycles * probe::kRate / hz / 2.0));
    const int radius = width / 2, count = static_cast<int>(x.size());
    const int hop = static_cast<int>(std::lround(kHop * probe::kRate));
    const double omega = 2.0 * kPi * hz / probe::kRate, window = 2.0 * kPi / width;
    std::array<std::vector<Complex>, 3> prefix;
    for (int band = 0; band < 3; ++band) {
        auto& p = prefix[static_cast<std::size_t>(band)];
        p.resize(x.size() + 1);
        const double frequency = omega + (band - 1) * window;
        const Complex step = std::polar(1.0, -frequency);
        Complex phase = 1.0;
        for (std::size_t n = 0; n < x.size(); ++n) {
            // Periodic re-anchoring bounds recurrence drift on long takes.
            if (n % 4096 == 0) phase = std::polar(1.0, -frequency * n);
            p[n + 1] = p[n] + x[n] * phase;
            phase *= step;
        }
    }
    std::vector<Point> out;
    for (int center = -radius; center + radius < count; center += hop) {
        const auto left = static_cast<std::size_t>(std::max(0, center - radius));
        const auto right = static_cast<std::size_t>(std::max(0, center + radius));
        const Complex turn = std::polar(1.0, window * center);
        const Complex sum = 0.5 * (prefix[1][right] - prefix[1][left]) +
            0.25 * std::conj(turn) * (prefix[0][right] - prefix[0][left]) +
            0.25 * turn * (prefix[2][right] - prefix[2][left]);
        out.push_back({center / probe::kRate, 4.0 * std::abs(sum) / width});
    }
    return out;
}

struct Attack { double first50, last50, first5, peak; int crossings50; };

Attack Thresholds(const std::vector<Point>& envelope) {
    Attack a {0.0, 0.0, 0.0, 0.0, 0};
    for (const Point& p : envelope) a.peak = std::max(a.peak, p.amplitude);
    if (!(a.peak > 0.0)) throw std::runtime_error("silent fundamental tracker");
    const double low = a.peak * std::pow(10.0, -50.0 / 20.0);
    const double high = a.peak * std::pow(10.0, -5.0 / 20.0);
    const auto crossing = [](const Point& p, const Point& q, double level) {
        // Interpolate log amplitude, the coordinate used for the thresholds.
        const double lp = std::log(std::max(p.amplitude, 1.0e-300));
        const double lq = std::log(std::max(q.amplitude, 1.0e-300));
        return p.seconds + (q.seconds - p.seconds) * (std::log(level) - lp) / (lq - lp);
    };
    for (std::size_t i = 1; i < envelope.size(); ++i) {
        const Point& p = envelope[i - 1];
        const Point& q = envelope[i];
        if (p.amplitude < low && q.amplitude >= low) {
            a.last50 = crossing(p, q, low);
            if (++a.crossings50 == 1) a.first50 = a.last50;
        }
        if (p.amplitude < high && q.amplitude >= high) {
            a.first5 = crossing(p, q, high);
            if (a.crossings50 == 0) throw std::runtime_error("missing -50 dB crossing");
            return a;
        }
    }
    throw std::runtime_error("missing -5 dB crossing");
}

void SelfTest() {
    // Known exponential sinusoidal rise: 45 dB in 200 ms, deliberately
    // separated from the trigger and from both ends of the analysis window.
    // Also exercise absolute-gain invariance and opening-transient detection.
    std::vector<double> x(static_cast<std::size_t>(probe::kRate));
    for (std::size_t i = 0; i < x.size(); ++i) {
        const double t = i / probe::kRate;
        const double db = std::min(0.0, (t - 0.6) * 45.0 / 0.2);
        x[i] = std::pow(10.0, db / 20.0) * std::sin(2.0 * kPi * 440.0 * t);
    }
    for (const int cycles : {2, 4, 8}) {
        const auto envelope = Envelope(x, 440.0, cycles);
        // Check the prefix implementation against the literal Hann DFT,
        // including a window straddling the zero-padded beginning.
        const int width = 2 * static_cast<int>(std::lround(cycles * probe::kRate / 440.0 / 2.0));
        for (std::size_t index : {std::size_t(1), envelope.size() / 3, envelope.size() - 1}) {
            const Point& point = envelope[index];
            const int center = static_cast<int>(std::lround(point.seconds * probe::kRate));
            std::complex<double> sum = 0.0;
            for (int n = std::max(0, center - width / 2); n < center + width / 2; ++n) {
                const double window = 0.5 + 0.5 * std::cos(2.0 * kPi * (n - center) / width);
                sum += window * x[static_cast<std::size_t>(n)] *
                    std::polar(1.0, -2.0 * kPi * 440.0 * n / probe::kRate);
            }
            if (std::fabs(point.amplitude - 4.0 * std::abs(sum) / width) > 1.0e-10)
                throw std::runtime_error("prefix Hann differs from direct DFT");
        }
        const Attack a = Thresholds(envelope);
        if (std::fabs(a.first5 - a.first50 - 0.2) > 0.001)
            throw std::runtime_error("synthetic exponential rise failed");
        for (double& sample : x) sample *= 0.1;
        const Attack b = Thresholds(Envelope(x, 440.0, cycles));
        if (std::fabs(a.first50 - b.first50) > 1.0e-7 ||
            std::fabs(a.first5 - b.first5) > 1.0e-7)
            throw std::runtime_error("gain invariance failed");
    }
    const std::vector<Point> separated {{0, 0}, {0.01, 0.01}, {0.02, 0.0001},
        {0.1, 0.001}, {0.2, 0.01}, {0.3, 1}};
    if (Thresholds(separated).crossings50 != 2)
        throw std::runtime_error("separate transient detection failed");
    std::fprintf(stderr, "CompareAttacks estimator self-test passed\n");
}

Options Parse(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--self-test") { options.selfTest = true; continue; }
        if (i + 1 == argc) throw std::runtime_error("missing argument value");
        const std::string value = argv[++i];
        if (arg == "--note") options.note = value;
        else if (arg == "--trace-dir") options.traceDir = value;
        else if (arg == "--seconds" || arg == "--pallet-ms" || arg == "--target") {
            std::size_t used = 0;
            const double number = std::stod(value, &used);
            if (used != value.size() || !std::isfinite(number)) throw std::runtime_error("invalid number");
            if (arg == "--seconds") options.seconds = number;
            if (arg == "--pallet-ms") options.palletMs = number;
            if (arg == "--target") {
                if (number != 55.0 && number != 70.0)
                    throw std::runtime_error("target must be 55 or 70 dBA");
                options.target = number;
            }
        } else throw std::runtime_error("unknown option: " + arg);
    }
    if (options.seconds < 2.0 || options.seconds > 30.0 ||
        options.palletMs < 3.0 || options.palletMs > 500.0 ||
        (options.target != 0.0 && options.target != 55.0 && options.target != 70.0))
        throw std::runtime_error("range: seconds 2..30, pallet-ms 3..500, target 55 or 70");
    if (!options.note.empty() && std::none_of(kReferences.begin(), kReferences.end(),
        [&](const Reference& r) { return options.note == r.name; }))
        throw std::runtime_error("note must be A2, A3, A4, or A5");
    return options;
}

} // namespace

int main(int argc, char** argv) {
    try {
        const Options options = Parse(argc, argv);
        if (options.selfTest) { SelfTest(); return 0; }
        if (!options.traceDir.empty()) std::filesystem::create_directories(options.traceDir);
        std::fprintf(stderr, "Bare pulled M cell, %.0f kHz, %.1f s, %.1f ms pallet, harmonic source SPL at 0.5 m.\n"
            "Centered Hann; apparent key latencies include window smear. Reference precision 10 ms.\n", probe::kRate / 1000.0, options.seconds, options.palletMs);
        std::printf("note,midi,target_dba,pressure_pa,source_dba,settling_db,f0_hz,seconds,pallet_ms,window_cycles,window_ms,first_minus50_ms,last_minus50_ms,first_minus5_ms,first_rise_ms,last_rise_ms,crossings_minus50,paper_finger_ms,first_difference_ms,last_difference_ms\n");
        for (const Reference& ref : kReferences) {
            if (!options.note.empty() && options.note != ref.name) continue;
            for (double target : {55.0, 70.0}) {
                if (options.target != 0.0 && options.target != target) continue;
                std::fprintf(stderr, "Calibrating %s at %.0f dBA...\n", ref.name, target);
                const Take take = Calibrate(ref.key, target, options);
                if (std::fabs(take.settlingDb) > 0.1)
                    std::fprintf(stderr, "WARNING: %s %.0f dBA last half-seconds differ %.3f dB; rerun with --seconds 12.\n", ref.name, target, take.settlingDb);
                for (int cycles : {2, 4, 8}) {
                    const std::vector<Point> envelope = Envelope(take.samples, take.hz, cycles);
                    const Attack a = Thresholds(envelope);
                    const double firstRise = 1000.0 * (a.first5 - a.first50);
                    const double lastRise = 1000.0 * (a.first5 - a.last50);
                    const double reference = target == 55.0 ? ref.pianoMs : ref.forteMs;
                    std::printf("%s,%d,%.0f,%.6f,%.4f,%.4f,%.5f,%.2f,%.2f,%d,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%d,%.0f,%.3f,%.3f\n",
                        ref.name, ref.key, target, take.pressure, take.dbA, take.settlingDb, take.hz,
                        options.seconds, options.palletMs, cycles, 1000.0 * cycles / take.hz,
                        a.first50 * 1000.0, a.last50 * 1000.0, a.first5 * 1000.0,
                        firstRise, lastRise, a.crossings50, reference, firstRise - reference, lastRise - reference);
                    std::fflush(stdout);
                    if (!options.traceDir.empty()) {
                        const std::string name = std::string(ref.name) + "-" + std::to_string(static_cast<int>(target)) +
                            "dba-" + std::to_string(cycles) + "cycles.csv";
                        std::ofstream file(std::filesystem::path(options.traceDir) / name);
                        file << "time_ms,fundamental_relative_db\n";
                        for (const Point& p : envelope)
                            file << p.seconds * 1000.0 << ',' << 20.0 * std::log10(std::max(p.amplitude / a.peak, 1.0e-15)) << '\n';
                        if (!file) throw std::runtime_error("cannot write envelope trace");
                    }
                }
            }
        }
        return 0;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "CompareAttacks: %s\n", e.what());
        return 1;
    }
}
