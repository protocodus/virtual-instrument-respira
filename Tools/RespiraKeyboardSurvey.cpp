// Every key of the treble keyboard, the chord buttons and the bass buttons
// through every factory preset, at three bellows pressures and both bellows
// directions: how loud, how soon, how in tune and how bright each key is, and
// the pressure at which it starts to speak.
//
//   RespiraKeyboardSurvey <out.csv> [first preset] [last preset]
//
// Protocol. Each preset plays as it ships, on Auto and on the split keyboard
// (C1-B1 the bass buttons, C2-B2 the chord buttons, C3-C8 the treble), with
// these changes so that a key is heard alone and the same every time: Room 0,
// Mechanics at its lowest (no knock in the level or the onset), Life 0 and
// Touch 0 (Auto holds one pressure). The wheel sets Auto's pressure: 100, 200
// and 400 Pa asked for, the last Auto's own (wheel down) and the reeds' tuning
// pressure; the stroke's own level makes it a few per cent less, and the
// pressure measured is in the table. C#0 turns the bellows to push before the
// take.
//
// A take: another key of the same manual (an octave away in the treble, a
// fourth up on the buttons) sounds from 0.1 to 0.5 s and brings the bellows to
// pressure, which Auto then holds; the surveyed key opens at 0.9 s, so its
// reeds start from rest in bellows already at pressure, and is held to 2.5 s.
//   level     RMS of both channels over 2.0-2.4 s, dBFS, through the output stage
//   speaks    the key is within 30 dB of its own level at 400 Pa (and above
//             -60 dBFS there); the measures below are of speaking keys only
//   onset     ms from the key to the first 10 ms whose RMS is within 6 dB of that level
//   cents     the strongest of the key's octave, the one below and the one
//             above (which rank that is depends on the register; a tremolo
//             rank shows its detuning), against equal temperament at A4 =
//             440 Hz, which ReedTuning.h tunes each tongue to at 400 Pa; from
//             the peak of a Hann-windowed transform of 2.0-2.4 s
//   centroid  amplitude-weighted mean frequency, 20 Hz-20 kHz, of the same 0.4 s
// Speaking pressure: the key held from the start while the wheel comes down
// over 4 s, Auto's pressure rising from its softest (60 Pa asked) to 400; the
// pressure at which the key's component first comes within 30 dB of its
// amplitude at 400 Pa. A slow reed is still rising as the pressure passes, so
// this is the pressure of a 4 s crescendo, an upper bound on the reed's own.
//
// The summary on stdout gives, per preset, manual, pressure and direction,
// each measure's range and its largest step away from the median of the three
// keys either side: a key that stands out from its neighbours, which a smooth
// instrument does not have.
#include "DSP/RespiraEngine.h"
#include "DSP/RespiraPresets.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

namespace {

using namespace respira;

constexpr double kRate = 48000.0, kTwoPi = 6.283185307179586;
struct Manual {
    const char* name;
    int low, high;
};
// The split keyboard (RespiraEngine.cpp, GroupOf).
constexpr Manual kManuals[] = { { "treble", 48, 108 }, { "chord", 36, 47 }, { "bass", 24, 35 } };
constexpr std::uint8_t kPushSwitch = 13;
constexpr double kPressures[] = { 100.0, 200.0, 400.0 };
constexpr double kAutoPa = 400.0, kSoftShare = 0.15;   // RespiraEngine.cpp: Auto's pressure and its share with the wheel up

// The wheel that asks Auto for `pascal`.
double WheelFor(double pascal) { return (1.0 - pascal / kAutoPa) / (1.0 - kSoftShare); }

struct Take {
    std::vector<float> mono, left, right;
    std::vector<double> pressure;   // every sample
};

struct Event {
    double seconds;
    std::uint8_t status, key, velocity;
};

Take Play(Engine& engine, double seconds, const std::vector<Event>& events, double (*wheel)(double)) {
    Take t;
    const auto frames = static_cast<std::size_t>(seconds * kRate);
    t.mono.resize(frames);
    t.left.resize(frames);
    t.right.resize(frames);
    t.pressure.resize(frames);
    engine.Reset();
    std::size_t next = 0;
    for (std::size_t i = 0; i < frames; ++i) {
        while (next < events.size() && static_cast<std::size_t>(std::llround(events[next].seconds * kRate)) <= i) {
            const std::uint8_t bytes[3] { events[next].status, events[next].key, events[next].velocity };
            engine.Midi(bytes, 3);
            ++next;
        }
        engine.SetHand(wheel(static_cast<double>(i) / kRate));
        engine.Render(&t.left[i], &t.right[i], 1);
        t.mono[i] = 0.5f * (t.left[i] + t.right[i]);
        t.pressure[i] = engine.Diag().pressure;
    }
    return t;
}

double Db(double x) { return 20.0 * std::log10(std::max(x, 1.0e-12)); }

double Rms(const Take& t, double from, double to) {
    double sum = 0.0;
    const auto a = static_cast<std::size_t>(from * kRate), b = std::min(t.left.size(), static_cast<std::size_t>(to * kRate));
    for (std::size_t i = a; i < b; ++i) sum += 0.5 * (static_cast<double>(t.left[i]) * t.left[i] + static_cast<double>(t.right[i]) * t.right[i]);
    return std::sqrt(sum / static_cast<double>(std::max<std::size_t>(1, b - a)));
}

// The amplitude of the component at `hz` in `count` samples from `start`, Hann-windowed.
double Component(const std::vector<float>& x, std::size_t start, std::size_t count, double hz) {
    const std::complex<double> turn = std::polar(1.0, -kTwoPi * hz / kRate);
    std::complex<double> phase = 1.0, sum = 0.0;
    double window = 0.0;
    for (std::size_t i = 0; i < count; ++i) {
        const double w = 0.5 - 0.5 * std::cos(kTwoPi * static_cast<double>(i) / static_cast<double>(count));
        sum += w * static_cast<double>(x[start + i]) * phase;
        window += w;
        phase *= turn;
    }
    return 2.0 * std::abs(sum) / window;
}

// The strongest component within a semitone of `near`: its frequency and amplitude.
// The window's main lobe is 4 / (count / rate) Hz wide, so 2 Hz steps cannot
// step over it; two parabolas through the log amplitude then place the peak.
std::pair<double, double> PeakNear(const std::vector<float>& x, std::size_t start, std::size_t count, double near) {
    const double low = near * std::pow(2.0, -100.0 / 1200.0), high = near * std::pow(2.0, 100.0 / 1200.0);
    const int steps = std::max(20, static_cast<int>(std::ceil((high - low) / 2.0)));
    double hz = near, best = -1.0;
    for (int i = 0; i <= steps; ++i) {
        const double f = low + (high - low) * i / steps, a = Component(x, start, count, f);
        if (a > best) {
            best = a;
            hz = f;
        }
    }
    for (const double d : { 1.0, 0.25 }) {
        const double l = std::log(std::max(Component(x, start, count, hz - d), 1.0e-30)),
                     m = std::log(std::max(Component(x, start, count, hz), 1.0e-30)),
                     r = std::log(std::max(Component(x, start, count, hz + d), 1.0e-30));
        const double curve = l - 2.0 * m + r;
        if (curve < 0.0) hz += std::clamp(0.5 * (l - r) / curve, -1.0, 1.0) * d;
    }
    return { hz, Component(x, start, count, hz) };
}

void Fft(std::vector<std::complex<double>>& a) {
    const std::size_t n = a.size();
    for (std::size_t i = 1, j = 0; i < n; ++i) {
        std::size_t bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) std::swap(a[i], a[j]);
    }
    for (std::size_t len = 2; len <= n; len <<= 1) {
        const std::complex<double> step = std::polar(1.0, -kTwoPi / static_cast<double>(len));
        for (std::size_t i = 0; i < n; i += len) {
            std::complex<double> w = 1.0;
            for (std::size_t k = 0; k < len / 2; ++k) {
                const std::complex<double> u = a[i + k], v = a[i + k + len / 2] * w;
                a[i + k] = u + v;
                a[i + k + len / 2] = u - v;
                w *= step;
            }
        }
    }
}

double Centroid(const std::vector<float>& x, std::size_t start) {
    constexpr std::size_t n = 16384;
    std::vector<std::complex<double>> a(n);
    for (std::size_t i = 0; i < n; ++i)
        a[i] = (0.5 - 0.5 * std::cos(kTwoPi * static_cast<double>(i) / static_cast<double>(n))) * static_cast<double>(x[start + i]);
    Fft(a);
    double weighted = 0.0, sum = 0.0;
    for (std::size_t k = 1; k < n / 2; ++k) {
        const double hz = static_cast<double>(k) * kRate / static_cast<double>(n);
        if (hz < 20.0 || hz > 20000.0) continue;
        weighted += hz * std::abs(a[k]);
        sum += std::abs(a[k]);
    }
    return sum > 0.0 ? weighted / sum : 0.0;
}

std::string NoteName(int key) {
    static const char* const names[] { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    return names[key % 12] + std::to_string(key / 12 - 1);
}

// The pitch a key is surveyed at: its own in the treble, the 8' rank's of its
// buttons otherwise (chords F#3..F4, basses E2..D#3: ReedBank.h, RankNote).
double KeyHz(int key) {
    if (key >= kManuals[0].low) return NoteHz(key);
    return key >= kManuals[1].low ? NoteHz(54 + PitchClass(key - 54)) : NoteHz(40 + PitchClass(key - 40));
}

int Primer(int key) {
    if (key >= kManuals[0].low) return key + 12 <= kManuals[0].high ? key + 12 : key - 12;
    const int low = key >= kManuals[1].low ? kManuals[1].low : kManuals[2].low;
    return low + (key - low + 5) % 12;
}

struct Row {
    int key = 0;
    double pressure = 0.0, level = 0.0, onset = 0.0, cents = 0.0, hz = 0.0, centroid = 0.0, amplitude = 0.0;
    bool speaks = false;
};

double gWheel = 0.0;
double HeldWheel(double) { return gWheel; }
// Down over 4 s from 0.3 s: 60 Pa rising to 400.
double FallingWheel(double seconds) { return std::clamp(1.0 - (seconds - 0.3) / 4.0, 0.0, 1.0); }

Row Survey(Engine& engine, int key, double pascal, bool push) {
    std::vector<Event> events;
    if (push) {
        events.push_back({ 0.0, 0x90, kPushSwitch, 100 });
        events.push_back({ 0.05, 0x80, kPushSwitch, 0 });
    }
    const auto primer = static_cast<std::uint8_t>(Primer(key));
    events.push_back({ 0.1, 0x90, primer, 100 });
    events.push_back({ 0.5, 0x80, primer, 0 });
    events.push_back({ 0.9, 0x90, static_cast<std::uint8_t>(key), 100 });
    gWheel = WheelFor(pascal);
    const Take t = Play(engine, 2.5, events, HeldWheel);
    Row row;
    row.key = key;
    const auto from = static_cast<std::size_t>(2.0 * kRate), count = static_cast<std::size_t>(0.4 * kRate);
    for (std::size_t i = from; i < from + count; ++i) row.pressure += t.pressure[i] / static_cast<double>(count);
    const double level = Rms(t, 2.0, 2.4);
    row.level = Db(level);
    row.onset = 1600.0;
    for (double at = 0.9; at < 2.5 - 0.01; at += 0.001)
        if (Rms(t, at, at + 0.01) >= 0.5 * level) {
            row.onset = (at - 0.9) * 1000.0;
            break;
        }
    const double hz = KeyHz(key);
    for (const double octave : { 0.5, 1.0, 2.0 }) {
        const auto [at, amplitude] = PeakNear(t.mono, from, count, hz * octave);
        if (amplitude > row.amplitude) {
            row.amplitude = amplitude;
            row.hz = at;
            row.cents = 1200.0 * std::log2(at / (hz * octave));
        }
    }
    row.centroid = Centroid(t.mono, from);
    return row;
}

// The bellows pressure at which the key's component at `hz` first comes
// within 30 dB of `full`, its amplitude at 400 Pa.
double SpeaksAt(Engine& engine, int key, bool push, double hz, double full) {
    std::vector<Event> events;
    if (push) {
        events.push_back({ 0.0, 0x90, kPushSwitch, 100 });
        events.push_back({ 0.05, 0x80, kPushSwitch, 0 });
    }
    events.push_back({ 0.1, 0x90, static_cast<std::uint8_t>(key), 100 });
    const Take t = Play(engine, 4.6, events, FallingWheel);
    // Frames of four periods, at least 20 ms, every 10 ms.
    const auto frame = static_cast<std::size_t>(std::max(0.02, 4.0 / hz) * kRate), hop = static_cast<std::size_t>(0.01 * kRate);
    for (std::size_t start = static_cast<std::size_t>(0.1 * kRate); start + frame < t.mono.size(); start += hop)
        if (Component(t.mono, start, frame, hz) >= full * std::pow(10.0, -30.0 / 20.0)) return std::fabs(t.pressure[start + frame / 2]);
    return 0.0;   // never
}

double Median(std::vector<double> v) {
    std::sort(v.begin(), v.end());
    return v[v.size() / 2];
}

// The largest step of v[i] away from the median of the three values either
// side of it (so not at the ends, where a slope would read as a step).
std::pair<double, int> Standout(const std::vector<double>& v) {
    double worst = 0.0;
    int at = -1;
    for (int i = 3; i + 3 < static_cast<int>(v.size()); ++i) {
        std::vector<double> around(v.begin() + i - 3, v.begin() + i + 4);
        around.erase(around.begin() + 3);
        std::sort(around.begin(), around.end());
        const double step = v[static_cast<std::size_t>(i)] - 0.5 * (around[2] + around[3]);
        if (std::fabs(step) > std::fabs(worst)) {
            worst = step;
            at = i;
        }
    }
    return { worst, at };
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: RespiraKeyboardSurvey <out.csv> [first preset] [last preset]\n");
        return 2;
    }
    const int first = argc > 2 ? std::atoi(argv[2]) : 0;
    const int last = argc > 3 ? std::atoi(argv[3]) : static_cast<int>(kPresets.size()) - 1;
    std::FILE* csv = std::fopen(argv[1], "w");
    if (!csv) {
        std::fprintf(stderr, "cannot write %s\n", argv[1]);
        return 1;
    }
    std::fprintf(csv, "preset,manual,key,note,direction,target_pa,pressure_pa,level_dbfs,onset_ms,cents,hz,centroid_hz,speaks_pa\n");
    auto engine = std::make_unique<Engine>();
    engine->Prepare(kRate);
    for (int preset = first; preset <= last && preset < static_cast<int>(kPresets.size()); ++preset) {
        Controls controls = kPresets[static_cast<std::size_t>(preset)].ToControls();
        controls[kRoom] = 0.0;
        controls[kMechanics] = kControlInfo[kMechanics].min;
        controls[kLife] = 0.0;
        controls[kTouch] = 0.0;
        controls[kAuto] = 1.0;
        engine->SetControls(controls);
        std::printf("\n## %s\n\n", kPresets[static_cast<std::size_t>(preset)].name);
        std::printf("| Manual | Bellows | Pa | Silent keys | Level dBFS | stands out | Cents | Onset ms: median, slowest | stands out | Centroid stands out | Speaks above 65 Pa |\n");
        std::printf("| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |\n");
        for (const Manual& manual : kManuals) {
            const int low = manual.low, high = manual.high;
            for (const bool push : { false, true }) {
                std::vector<std::vector<Row>> rows(3);
                for (int key = low; key <= high; ++key)
                    for (int p = 0; p < 3; ++p) rows[static_cast<std::size_t>(p)].push_back(Survey(*engine, key, kPressures[p], push));
                std::vector<double> speaks;
                int late = 0, lateKey = 0;
                double latest = 0.0;
                for (int key = low; key <= high; ++key) {
                    const Row& full = rows[2][static_cast<std::size_t>(key - low)];
                    for (int p = 0; p < 3; ++p) {
                        Row& r = rows[static_cast<std::size_t>(p)][static_cast<std::size_t>(key - low)];
                        r.speaks = full.level > -60.0 && r.level > full.level - 30.0;
                    }
                    speaks.push_back(full.speaks ? SpeaksAt(*engine, key, push, full.hz, full.amplitude) : 0.0);
                    // 65 Pa: the wheel has begun to come down from Auto's softest.
                    if (speaks.back() == 0.0 || speaks.back() > 65.0) {
                        ++late;
                        if (speaks.back() == 0.0 || speaks.back() > latest) {
                            latest = speaks.back() == 0.0 ? 1.0e9 : speaks.back();
                            lateKey = key;
                        }
                    }
                }
                for (int p = 0; p < 3; ++p) {
                    std::vector<double> level, cents, onset, logOnset, centroid;
                    std::vector<int> keys;
                    double pressure = 0.0;
                    int silent = 0, firstSilent = 0;
                    for (const Row& r : rows[static_cast<std::size_t>(p)]) {
                        pressure += r.pressure / static_cast<double>(high - low + 1);
                        std::fprintf(csv, "%s,%s,%d,%s,%s,%.0f,%.1f,%.2f,", kPresets[static_cast<std::size_t>(preset)].name, manual.name, r.key,
                                     NoteName(r.key).c_str(), push ? "push" : "pull", kPressures[p], r.pressure, r.level);
                        if (r.speaks) std::fprintf(csv, "%.0f,%.2f,%.3f,%.0f,", r.onset, r.cents, r.hz, r.centroid);
                        else std::fprintf(csv, ",,,,");
                        std::fprintf(csv, "%.0f\n", speaks[static_cast<std::size_t>(r.key - low)]);
                        if (!r.speaks) {
                            if (silent++ == 0) firstSilent = r.key;
                            continue;
                        }
                        keys.push_back(r.key);
                        level.push_back(r.level);
                        cents.push_back(r.cents);
                        onset.push_back(r.onset);
                        logOnset.push_back(std::log2(std::max(r.onset, 1.0)));
                        centroid.push_back(1200.0 * std::log2(std::max(r.centroid, 1.0)));
                    }
                    std::printf("| %s | %s | %.0f | ", manual.name, push ? "push" : "pull", std::fabs(pressure));
                    if (silent == 0) std::printf("none | ");
                    else std::printf("%d, from %s | ", silent, NoteName(firstSilent).c_str());
                    if (keys.empty()) {
                        std::printf("| | | | | | |\n");
                        continue;
                    }
                    const auto name = [&](int at) { return at < 0 ? std::string("-") : NoteName(keys[static_cast<std::size_t>(at)]); };
                    const auto [levelStep, levelAt] = Standout(level);
                    const auto [onsetStep, onsetAt] = Standout(logOnset);
                    const auto [brightStep, brightAt] = Standout(centroid);
                    const auto slowest = static_cast<int>(std::max_element(onset.begin(), onset.end()) - onset.begin());
                    std::printf("%.1f..%.1f | %+.1f dB at %s | ", *std::min_element(level.begin(), level.end()),
                                *std::max_element(level.begin(), level.end()), levelStep, name(levelAt).c_str());
                    std::printf("%+.1f..%+.1f | %.0f, %.0f at %s | ", *std::min_element(cents.begin(), cents.end()),
                                *std::max_element(cents.begin(), cents.end()), Median(onset), onset[static_cast<std::size_t>(slowest)], name(slowest).c_str());
                    std::printf("x%.2f at %s | %+.0f cents at %s | ", std::pow(2.0, onsetStep), name(onsetAt).c_str(), brightStep, name(brightAt).c_str());
                    if (p != 2) std::printf(" |\n");
                    else if (late == 0) std::printf("none |\n");
                    else if (latest > 1.0e8) std::printf("%d keys, %s not by 400 Pa |\n", late, NoteName(lateKey).c_str());
                    else std::printf("%d keys, the last %s at %.0f Pa |\n", late, NoteName(lateKey).c_str(), latest);
                }
                std::fflush(stdout);
                std::fflush(csv);
            }
        }
    }
    std::fclose(csv);
    return 0;
}
