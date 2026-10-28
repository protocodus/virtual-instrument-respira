// Engine contracts: air only from the hand's motion, the mechanism's knocks, the bellows' turn, pitch and pitch bend, rates and
// Oversampling, routing, register switches on held keys, the 4' rank's break, half-pressed keys,
// the air button and Assist, a hard push on a held key, the controller stream and the arm's momentum, block-partition
// invariance and determinism, release and voice limits, the noise scaling.
#include "DSP/RespiraEngine.h"
#include "DSP/RespiraPresets.h"

#include <array>
#include <cmath>
#include <cstdio>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace {

using namespace respira;

int failures = 0;

void Expect(bool ok, const char* test, const char* what, double value) {
    std::printf("%s %s: %s (%.4g)\n", ok ? "PASS" : "FAIL", test, what, value);
    if (!ok) ++failures;
}

struct Note {
    double seconds;
    std::uint8_t bytes[3];
};

struct Take {
    std::vector<float> left, right;
    std::vector<double> pressure;   // bellows pressure every 32 frames
    int maxCells = 0;
};

// Plays `notes` (in time order) while the hand follows `hand(seconds)`,
// placed through SetHand at every sample, rendering in blocks of `block`.
Take Play(Engine& engine, double rate, double seconds, const std::vector<Note>& notes,
          const std::function<double(double)>& hand, int block = 256) {
    Take take;
    const auto frames = static_cast<std::size_t>(seconds * rate);
    take.left.resize(frames);
    take.right.resize(frames);
    std::size_t next = 0;
    for (std::size_t done = 0; done < frames;) {
        const std::size_t end = std::min(frames, done + static_cast<std::size_t>(block));
        for (; done < end; ++done) {
            while (next < notes.size() && static_cast<std::size_t>(std::llround(notes[next].seconds * rate)) <= done)
                engine.Midi(notes[next++].bytes, 3);
            if (hand) engine.SetHand(hand(static_cast<double>(done) / rate));
            engine.Render(&take.left[done], &take.right[done], 1);
            if (done % 32 == 0) take.pressure.push_back(engine.Diag().pressure);
            take.maxCells = std::max(take.maxCells, engine.Diag().cells);
        }
    }
    return take;
}

std::unique_ptr<Engine> Make(double rate, int preset, const std::function<void(Controls&)>& adjust = {}) {
    auto engine = std::make_unique<Engine>();
    engine->Prepare(rate);
    Controls controls = kPresets[static_cast<std::size_t>(preset)].ToControls();
    if (adjust) adjust(controls);
    engine->SetControls(controls);
    engine->Reset();
    return engine;
}

double Rms(const std::vector<float>& x, double rate, double from, double to) {
    const auto a = static_cast<std::size_t>(from * rate), b = std::min(x.size(), static_cast<std::size_t>(to * rate));
    double sum = 0.0;
    for (std::size_t i = a; i < b; ++i) sum += static_cast<double>(x[i]) * x[i];
    return std::sqrt(sum / static_cast<double>(std::max<std::size_t>(1, b - a)));
}

double Db(double x) { return 20.0 * std::log10(std::max(x, 1.0e-12)); }

double Peak(const Take& t) {
    double peak = 0.0;
    for (std::size_t i = 0; i < t.left.size(); ++i)
        peak = std::max({ peak, std::fabs(static_cast<double>(t.left[i])), std::fabs(static_cast<double>(t.right[i])) });
    return peak;
}

// Reason's VU meter: the louder channel's RMS over a sliding 300 ms, +12 dB,
// at its highest.
double Vu(const Take& t, double rate) {
    const auto w = static_cast<std::size_t>(0.3 * rate);
    double sl = 0.0, sr = 0.0, loudest = 0.0;
    for (std::size_t i = 0; i < t.left.size(); ++i) {
        sl += static_cast<double>(t.left[i]) * t.left[i];
        sr += static_cast<double>(t.right[i]) * t.right[i];
        if (i >= w) {
            sl -= static_cast<double>(t.left[i - w]) * t.left[i - w];
            sr -= static_cast<double>(t.right[i - w]) * t.right[i - w];
        }
        loudest = std::max(loudest, std::max(sl, sr) / static_cast<double>(w));
    }
    return 10.0 * std::log10(std::max(loudest, 1.0e-24)) + 12.0;
}

double MeanPressure(const Take& t, double rate, double from, double to) {
    const auto a = static_cast<std::size_t>(from * rate / 32), b = static_cast<std::size_t>(to * rate / 32);
    double sum = 0.0;
    for (std::size_t i = a; i < b && i < t.pressure.size(); ++i) sum += t.pressure[i];
    return sum / static_cast<double>(b - a);
}

// The strongest frequency within +-3 % of `near`, from a Hann-windowed DFT
// scanned in 0.02 Hz steps.
double PeakHz(const std::vector<float>& x, double rate, double from, double to, double near) {
    const auto a = static_cast<std::size_t>(from * rate), n = static_cast<std::size_t>((to - from) * rate);
    double best = 0.0, bestHz = 0.0;
    for (double hz = near * 0.97; hz <= near * 1.03; hz += 0.02) {
        double re = 0.0, im = 0.0;
        const double w = 2.0 * kPi * hz / rate;
        for (std::size_t i = 0; i < n; ++i) {
            const double window = 0.5 - 0.5 * std::cos(2.0 * kPi * static_cast<double>(i) / static_cast<double>(n));
            re += window * x[a + i] * std::cos(w * static_cast<double>(i));
            im += window * x[a + i] * std::sin(w * static_cast<double>(i));
        }
        if (re * re + im * im > best) { best = re * re + im * im; bestHz = hz; }
    }
    return bestHz;
}

double Cents(double hz, double reference) { return 1200.0 * std::log2(hz / reference); }

// Opening from 0.3 to 0.54 in 2 s, then closing back: a pull and a push at
// a moderate pressure, near the reeds' tuning pressure on a single reed.
double PullThenPush(double t) { return t < 2.0 ? 0.3 + 0.12 * t : std::max(0.3, 0.54 - 0.12 * (t - 2.0)); }

constexpr std::uint8_t kOn = 0x90, kOff = 0x80, kCc = 0xB0;

// The loudest sample from `from` seconds to `to`, and when it is.
std::pair<double, double> PeakIn(const Take& t, double rate, double from, double to) {
    double peak = 0.0, at = from;
    for (auto i = static_cast<std::size_t>(from * rate); i < std::min(t.left.size(), static_cast<std::size_t>(to * rate)); ++i) {
        const double v = std::max(std::fabs(static_cast<double>(t.left[i])), std::fabs(static_cast<double>(t.right[i])));
        if (v > peak) {
            peak = v;
            at = static_cast<double>(i) / rate;
        }
    }
    return { peak, at };
}

void SilentWithoutMotion() {
    // The key and the bass button knock as they land (MechanicsKnock); after
    // that a still hand moves no air, so the reeds stay silent.
    auto e = Make(48000.0, 3);
    const Take t = Play(*e, 48000.0, 1.0, { { 0.0, { kOn, 69, 100 } }, { 0.0, { kOn, 24, 100 } } }, [](double) { return 0.5; });
    const double peak = PeakIn(t, 48000.0, 0.1, 1.0).first;
    Expect(peak < 1.0e-6, "SilentWithoutMotion", "a still hand moves no air: peak from 0.1 s below -120 dBFS", peak);
}

void MechanicsKnock() {
    // With a still hand only the mechanism sounds (Mechanics.h). A key knocks
    // as it lands, when its stroke ends (12 ms at velocity 0 .. 3 ms at 127),
    // its pallet as the spring seats it 8 ms after it is let go, and a
    // faster stroke lands harder. A key let go before it lands, or held half
    // down by channel pressure, knocks only as its pallet seats. Mechanics
    // sets the level in dB, and the knocks peak where a real accordion's
    // mechanism does, 300-500 Hz (Niewczas 2019).
    const double rate = 48000.0;
    const auto key = [&](int velocity, double mechanics, double holdSeconds, int pressure) {
        auto e = Make(rate, 0, [mechanics](Controls& c) { c[kMechanics] = mechanics; });
        return Play(*e, rate, 0.5, { { 0.0, { 0xD0, static_cast<std::uint8_t>(pressure), 0 } }, { 0.05, { kOn, 72, static_cast<std::uint8_t>(velocity) } },
                                     { 0.05 + holdSeconds, { kOff, 72, 0 } } },
                    [](double) { return 0.5; });
    };
    const Take soft = key(20, 0.0, 0.2, 0), hard = key(127, 0.0, 0.2, 0), quieter = key(20, -10.0, 0.2, 0);
    const auto softPress = PeakIn(soft, rate, 0.05, 0.1), hardPress = PeakIn(hard, rate, 0.05, 0.1), softSeat = PeakIn(soft, rate, 0.25, 0.3);
    const double landing = 0.05 + 0.012 - 0.009 * 20.0 / 127.0;
    Expect(softPress.second > landing && softPress.second < landing + 0.004, "MechanicsKnock", "a key knocks as it lands: peak after its stroke, ms",
           (softPress.second - landing) * 1.0e3);
    Expect(softSeat.second > 0.258 && softSeat.second < 0.262, "MechanicsKnock", "its pallet knocks as it seats: peak after the key is let go, ms",
           (softSeat.second - 0.25) * 1.0e3);
    Expect(Db(hardPress.first / softPress.first) > 10.0, "MechanicsKnock", "a hard stroke against a soft one, dB", Db(hardPress.first / softPress.first));
    const double trim = Db(PeakIn(quieter, rate, 0.05, 0.1).first / softPress.first);
    Expect(std::fabs(trim + 10.0) < 0.1, "MechanicsKnock", "Mechanics at -10 dB, dB", trim);
    const Take turned = key(20, 0.0, 0.005, 0), half = key(20, 0.0, 0.2, 127);
    const double turnedPress = PeakIn(turned, rate, 0.05, 0.0575).first, halfPress = PeakIn(half, rate, 0.05, 0.25).first;
    Expect(turnedPress < 1.0e-6, "MechanicsKnock", "a key let go before it lands: peak before its pallet seats", turnedPress);
    Expect(PeakIn(turned, rate, 0.05, 0.1).first > 1.0e-4, "MechanicsKnock", "and then its pallet seats: peak", PeakIn(turned, rate, 0.05, 0.1).first);
    Expect(halfPress < 1.0e-6, "MechanicsKnock", "a key held half down: peak while it is held", halfPress);
    Expect(PeakIn(half, rate, 0.25, 0.3).first > 1.0e-4, "MechanicsKnock", "and its pallet still seats: peak", PeakIn(half, rate, 0.25, 0.3).first);
    const double hz = [&] {
        double best = 0.0, at = 0.0;
        for (double f = 100.0; f <= 4000.0; f *= 1.01) {
            double re = 0.0, im = 0.0;
            for (auto i = static_cast<std::size_t>(0.05 * rate); i < static_cast<std::size_t>(0.1 * rate); ++i) {
                re += soft.left[i] * std::cos(2.0 * kPi * f * static_cast<double>(i) / rate);
                im += soft.left[i] * std::sin(2.0 * kPi * f * static_cast<double>(i) / rate);
            }
            if (re * re + im * im > best) { best = re * re + im * im; at = f; }
        }
        return at;
    }();
    Expect(hz > 210.0 && hz < 600.0, "MechanicsKnock", "a soft key's knock peaks at, Hz", hz);
}

void AirFromMotion() {
    const double rate = 48000.0;
    auto e = Make(rate, 0);
    const Take t = Play(*e, rate, 4.0, { { 0.0, { kOn, 69, 100 } } }, PullThenPush);
    const double pull = MeanPressure(t, rate, 0.5, 1.8), push = MeanPressure(t, rate, 2.5, 3.8);
    Expect(pull < -100.0, "AirFromMotion", "opening draws: mean pressure, Pa", pull);
    Expect(push > 100.0, "AirFromMotion", "closing presses: mean pressure, Pa", push);
    const double pullCents = Cents(PeakHz(t.left, rate, 0.8, 1.8, 440.0), 440.0);
    const double pushCents = Cents(PeakHz(t.left, rate, 2.8, 3.8, 440.0), 440.0);
    Expect(std::fabs(pullCents) < 4.0, "AirFromMotion", "pull reed sounds A4, cents", pullCents);
    Expect(std::fabs(pushCents) < 4.0, "AirFromMotion", "push reed sounds A4, cents", pushCents);
    Expect(Db(Rms(t.left, rate, 2.5, 3.8)) > -50.0, "AirFromMotion", "the push is audible, dBFS RMS", Db(Rms(t.left, rate, 2.5, 3.8)));
}

void TurnWithoutClick() {
    // Turning the bellows passes the pressure through zero, where the sounding
    // reed stops and its partner starts. A click there would stand out in the
    // top octaves, read here as the second difference (rising 12 dB per
    // octave) in 2 ms frames: within 50 ms of the zero crossing they stay
    // below their highest while the note sounds.
    const double rate = 48000.0;
    auto e = Make(rate, 0);
    const Take t = Play(*e, rate, 3.0, { { 0.0, { kOn, 69, 100 } } }, PullThenPush);
    std::size_t cross = 0;
    for (std::size_t i = static_cast<std::size_t>(2.0 * rate / 32); i < t.pressure.size() && cross == 0; ++i)
        if (t.pressure[i] > 0.0) cross = i * 32;
    const auto frame = static_cast<std::size_t>(0.002 * rate), near = static_cast<std::size_t>(0.05 * rate);
    double sounding = 0.0, turn = 0.0;
    for (std::size_t a = 2; a + frame <= t.left.size(); a += frame) {
        double sum = 0.0;
        for (std::size_t i = a; i < a + frame; ++i) {
            const double d = static_cast<double>(t.left[i]) - 2.0 * t.left[i - 1] + t.left[i - 2];
            sum += d * d;
        }
        const double rms = std::sqrt(sum / static_cast<double>(frame)), at = static_cast<double>(a) / rate;
        if (at >= 1.0 && at < 1.8) sounding = std::max(sounding, rms);
        if (cross > 0 && a + frame + near > cross && a < cross + near) turn = std::max(turn, rms);
    }
    Expect(cross > 0, "TurnWithoutClick", "the pressure turns after the hand, s", static_cast<double>(cross) / rate);
    Expect(Db(turn) < Db(sounding), "TurnWithoutClick", "top octaves at the turn against the sounding note, dB", Db(turn) - Db(sounding));
}

void StillnessStopsTheSound() {
    const double rate = 48000.0;
    auto e = Make(rate, 3);
    const Take t = Play(*e, rate, 3.5, { { 0.0, { kOn, 69, 100 } } },
                        [](double s) { return s < 1.5 ? 0.3 + 0.2 * s : 0.6; });
    const double moving = Rms(t.left, rate, 0.8, 1.4), still = Rms(t.left, rate, 3.0, 3.5);
    Expect(Db(moving) - Db(still) > 30.0, "StillnessStopsTheSound", "level drop once the hand stops, dB", Db(moving) - Db(still));
}

void RatesKeepTheSound() {
    // The calibrated level holds at every host rate, as the pitch does.
    double cents[3] = {}, level[3] = {};
    const double rates[3] = { 44100.0, 48000.0, 96000.0 };
    for (int i = 0; i < 3; ++i) {
        auto e = Make(rates[i], 0);
        const Take t = Play(*e, rates[i], 2.0, { { 0.0, { kOn, 69, 100 } } }, PullThenPush);
        cents[i] = Cents(PeakHz(t.left, rates[i], 0.8, 1.8, 440.0), 440.0);
        level[i] = Db(Rms(t.left, rates[i], 0.8, 1.8));
    }
    Expect(std::fabs(cents[0] - cents[1]) < 2.0, "RatesKeepTheSound", "44.1 kHz against 48 kHz, cents", cents[0] - cents[1]);
    Expect(std::fabs(cents[2] - cents[1]) < 2.0, "RatesKeepTheSound", "96 kHz against 48 kHz, cents", cents[2] - cents[1]);
    Expect(std::fabs(level[0] - level[1]) < 0.5, "RatesKeepTheSound", "44.1 kHz against 48 kHz, dB", level[0] - level[1]);
    Expect(std::fabs(level[2] - level[1]) < 0.5, "RatesKeepTheSound", "96 kHz against 48 kHz, dB", level[2] - level[1]);
}

// The largest change between neighbouring samples of the left channel.
double LargestStep(const std::vector<float>& x) {
    double step = 0.0;
    for (std::size_t i = 1; i < x.size(); ++i) step = std::max(step, std::fabs(static_cast<double>(x[i]) - x[i - 1]));
    return step;
}

void OversamplingKeepsTheSound() {
    // Oversampling refines the reeds' time step, not the instrument: pitch and
    // level hold at every setting, and a change while a note sounds is
    // continuous.
    const double rate = 48000.0;
    double cents[3] = {}, level[3] = {};
    Take steady;
    for (int i = 0; i < 3; ++i) {
        auto e = Make(rate, 0, [i](Controls& c) { c[kOversampling] = i; });
        const Take t = Play(*e, rate, 2.0, { { 0.0, { kOn, 69, 100 } } }, PullThenPush);
        cents[i] = Cents(PeakHz(t.left, rate, 0.8, 1.8, 440.0), 440.0);
        level[i] = Db(Rms(t.left, rate, 0.8, 1.8));
        if (i == 0) steady = t;
    }
    for (int i = 1; i < 3; ++i) {
        Expect(std::fabs(cents[i] - cents[0]) < 1.0, "OversamplingKeepsTheSound",
               i == 1 ? "2x against 1x, cents" : "4x against 1x, cents", cents[i] - cents[0]);
        Expect(std::fabs(level[i] - level[0]) < 0.5, "OversamplingKeepsTheSound",
               i == 1 ? "2x against 1x, dB" : "4x against 1x, dB", level[i] - level[0]);
    }
    struct Span { double from, to; int setting; };
    auto e = Make(rate, 0);
    Controls controls = e->GetControls();
    std::vector<Note> notes { { 0.0, { kOn, 69, 100 } } };
    Take changed;
    for (const Span& span : { Span { 0.0, 0.9, 0 }, Span { 0.9, 1.3, 2 }, Span { 1.3, 1.6, 1 }, Span { 1.6, 2.0, 0 } }) {
        controls[kOversampling] = span.setting;
        e->SetControls(controls);
        const Take part = Play(*e, rate, span.to - span.from, notes, [&span](double s) { return PullThenPush(span.from + s); });
        notes.clear();
        changed.left.insert(changed.left.end(), part.left.begin(), part.left.end());
    }
    const double step = LargestStep(changed.left) / LargestStep(steady.left);
    Expect(std::isfinite(step) && step < 1.25, "OversamplingKeepsTheSound",
           "changed mid-note (1x, 4x, 2x, 1x): largest sample step against steady 1x", step);
}

void MasterTune() {
    const double rate = 48000.0;
    auto e = Make(rate, 0);
    e->SetMasterTune(100.0);
    const Take t = Play(*e, rate, 2.0, { { 0.0, { kOn, 69, 100 } } }, PullThenPush);
    auto plain = Make(rate, 0);
    const Take u = Play(*plain, rate, 2.0, { { 0.0, { kOn, 69, 100 } } }, PullThenPush);
    const double cents = Cents(PeakHz(t.left, rate, 0.8, 1.8, 466.16), PeakHz(u.left, rate, 0.8, 1.8, 440.0));
    Expect(std::fabs(cents - 100.0) < 1.0, "MasterTune", "+100 cents against the untuned A4, cents", cents);
}

void PitchBend() {
    // The wheel retunes reeds already sounding: bent fully up half a second
    // into the note, and fully down from before it, against an unbent take.
    const double rate = 48000.0, up = 2.0 * 8191.0 / 8192.0;
    auto plain = Make(rate, 0);
    const Take u = Play(*plain, rate, 2.0, { { 0.0, { kOn, 69, 100 } } }, PullThenPush);
    const double reference = PeakHz(u.left, rate, 0.9, 1.8, 440.0);
    auto e = Make(rate, 0);
    const Take t = Play(*e, rate, 2.0, { { 0.0, { kOn, 69, 100 } }, { 0.5, { 0xE0, 0x7F, 0x7F } } }, PullThenPush);
    const double raised = Cents(PeakHz(t.left, rate, 0.9, 1.8, 440.0 * std::pow(2.0, up / 12.0)), reference);
    Expect(std::fabs(raised - 100.0 * up) < 3.0, "PitchBend", "full up on a sounding reed, cents", raised);
    auto d = Make(rate, 0);
    const Take w = Play(*d, rate, 2.0, { { 0.0, { 0xE0, 0x00, 0x00 } }, { 0.0, { kOn, 69, 100 } } }, PullThenPush);
    const double lowered = Cents(PeakHz(w.left, rate, 0.9, 1.8, 440.0 / std::pow(2.0, 2.0 / 12.0)), reference);
    Expect(std::fabs(lowered + 200.0) < 3.0, "PitchBend", "full down, cents", lowered);
}

void MusetteBeat() {
    // Independent math: M+ sits c cents above M at A4, a beat of
    // 440 (2^(c/1200) - 1) Hz, and M- the same interval below; the beat grows
    // as f^0.7 above A4.
    const double c = 22.0, beatA4 = 440.0 * (std::pow(2.0, c / 1200.0) - 1.0);
    const double up = NoteHz(RankNote(kRankMPlus, 69, c)) - 440.0, down = 440.0 - NoteHz(RankNote(kRankMMinus, 69, c));
    Expect(std::fabs(up - beatA4) < 1.0e-9, "MusetteBeat", "M+ beats against M at A4, Hz", up);
    Expect(std::fabs(down - 440.0 * (1.0 - std::pow(2.0, -c / 1200.0))) < 1.0e-9, "MusetteBeat",
           "M- sits the same interval below, Hz", down);
    const double up81 = NoteHz(RankNote(kRankMPlus, 81, c)) - 880.0;
    Expect(std::fabs(up81 - beatA4 * std::pow(2.0, 0.7)) < 1.0e-9, "MusetteBeat", "an octave up the beat is 2^0.7 faster, Hz", up81);
}

void SplitKeyboardRouting() {
    const auto cellsAfter = [](int preset, double keyboard, std::vector<Note> notes) {
        auto e = Make(48000.0, preset, [keyboard](Controls& c) { c[kKeyboard] = keyboard; });
        return Play(*e, 48000.0, 0.01, notes, {}).maxCells;
    };
    Expect(cellsAfter(0, 0.0, { { 0.0, { kOn, 33, 100 } } }) == 2, "SplitKeyboardRouting", "A1 is a bass button: two bass reeds", 0);
    Expect(cellsAfter(0, 0.0, { { 0.0, { kOn, 45, 100 } } }) == 2, "SplitKeyboardRouting", "A2 is a chord button: two chord reeds", 0);
    Expect(cellsAfter(7, 0.0, { { 0.0, { kOn, 57, 100 } } }) == 5, "SplitKeyboardRouting", "A3 is treble: Master sounds five reeds", 0);
    Expect(cellsAfter(0, 1.0, { { 0.0, { kOn, 33, 100 } } }) == 1, "SplitKeyboardRouting", "unsplit, A1 plays the treble", 0);
    Expect(cellsAfter(0, 1.0, { { 0.0, { kOn + 1, 69, 100 } } }) == 2, "SplitKeyboardRouting", "channel 2 is always bass buttons", 0);
    Expect(cellsAfter(0, 1.0, { { 0.0, { kOn + 2, 69, 100 } } }) == 2, "SplitKeyboardRouting", "channel 3 is always chord buttons", 0);
}

void RegisterActsOnHeldNotes() {
    // The register switches move slides, so they reach keys already held: a
    // held A4 on Clarinet (M) gains the tremolo reeds on Musette (M, M-, M+),
    // then drops to the 16' rank alone on Bassoon, sounding A3. A key let go
    // stays silent through a switch.
    const double rate = 48000.0;
    auto e = Make(rate, 0);
    Controls controls = e->GetControls();
    const auto opening = [](double from) { return [from](double s) { return 0.3 + 0.12 * (from + s); }; };
    Play(*e, rate, 0.6, { { 0.0, { kOn, 69, 100 } } }, opening(0.0));
    const int clarinet = e->Diag().cells;
    controls[kRegister] = 3;
    e->SetControls(controls);
    Play(*e, rate, 0.4, {}, opening(0.6));
    const int musette = e->Diag().cells;
    controls[kRegister] = 1;
    e->SetControls(controls);
    const Take bassoon = Play(*e, rate, 0.8, {}, opening(1.0));
    const int bassoonCells = e->Diag().cells;
    Expect(clarinet == 1 && musette == 3 && bassoonCells == 1, "RegisterActsOnHeldNotes",
           "chambers sounding on Clarinet, Musette, Bassoon: 1, 3, 1", clarinet * 100 + musette * 10 + bassoonCells);
    const double a3 = Cents(PeakHz(bassoon.left, rate, 0.3, 0.8, 220.0), 220.0);
    Expect(std::fabs(a3) < 5.0, "RegisterActsOnHeldNotes", "switched to Bassoon, the held A4 sounds A3, cents", a3);
    Expect(Db(Rms(bassoon.left, rate, 0.3, 0.8)) > -50.0, "RegisterActsOnHeldNotes", "and keeps sounding, dBFS RMS",
           Db(Rms(bassoon.left, rate, 0.3, 0.8)));
    Play(*e, rate, 0.1, { { 0.0, { kOff, 69, 0 } } }, opening(1.8));
    controls[kRegister] = 7;
    e->SetControls(controls);
    Play(*e, rate, 0.05, {}, opening(1.9));
    Expect(e->Diag().cells == 0, "RegisterActsOnHeldNotes", "a released key stays silent through a switch", e->Diag().cells);
}

void TopOctaveSteady() {
    // Above C#6 the 4' rank's reeds would double the 8' in unison and beat
    // slowly against it (RespiraEngine.cpp, Sounds). On Oboe (M, H) an E6 and
    // a C7 on a rising pull follow the pressure smoothly: their 100 ms RMS
    // from 0.6 to 1.8 s stays within 2 dB of a straight line, where the
    // doubled reeds strayed 3 and 8 dB.
    const double rate = 48000.0;
    double worst = 0.0;
    for (int key : { 88, 96 }) {
        auto e = Make(rate, 0, [](Controls& c) { c = Controls {}; c[kRegister] = 6; });
        const Take t = Play(*e, rate, 1.8, { { 0.0, { kOn, static_cast<std::uint8_t>(key), 100 } } },
                            [](double s) { return 0.3 + 0.12 * s; });
        std::vector<double> db;
        for (int i = 0; i < 12; ++i) db.push_back(Db(Rms(t.left, rate, 0.6 + 0.1 * i, 0.7 + 0.1 * i)));
        double sx = 0.0, sy = 0.0, sxx = 0.0, sxy = 0.0;
        for (std::size_t i = 0; i < db.size(); ++i) {
            sx += static_cast<double>(i);
            sy += db[i];
            sxx += static_cast<double>(i * i);
            sxy += static_cast<double>(i) * db[i];
        }
        const double n = static_cast<double>(db.size()), slope = (n * sxy - sx * sy) / (n * sxx - sx * sx), at0 = (sy - slope * sx) / n;
        for (std::size_t i = 0; i < db.size(); ++i) worst = std::max(worst, std::fabs(db[i] - at0 - slope * static_cast<double>(i)));
    }
    Expect(worst < 2.0, "TopOctaveSteady", "a held E6 or C7 on Oboe, 100 ms level off a straight line at most, dB", worst);
}

void HalfPressedKeys() {
    // Channel pressure holds the keys part way down; at full pressure the
    // pallet stays lifted 1 mm, where a strong bellows bends the reed flat and
    // quietens it. Okada measured C4 falling 5.1 Hz per kPa of bellows
    // pressure there (Adachi et al., Forum Acusticum 2026). Played here as a
    // push on Clarinet's C4, the pressure rising as the half-open pallet
    // passes less air.
    const double rate = 48000.0;
    struct Result { double hz, db, pa; };
    const auto push = [rate](double speed, int amount, double releaseAt) {
        auto e = Make(rate, 0);
        const auto channelPressure = static_cast<std::uint8_t>(amount);
        const Take t = Play(*e, rate, 2.0, { { 0.0, { kOn, 60, 100 } }, { 0.0, { 0xD0, channelPressure, 0 } },
                                             { releaseAt, { 0xD0, 0, 0 } } },
                            [speed](double s) { return 0.9 - speed * s; });
        return Result { PeakHz(t.left, rate, 1.2, 2.0, releaseAt < 2.0 || amount == 0 ? 261.63 : 254.0),
                        Db(Rms(t.left, rate, 1.2, 2.0)), MeanPressure(t, rate, 1.2, 2.0) };
    };
    const Result open = push(0.2, 0, 9.0), half = push(0.2, 127, 9.0), gentle = push(0.1, 127, 9.0);
    Expect(Cents(half.hz, open.hz) < -20.0, "HalfPressedKeys", "full channel pressure on a strong push, cents", Cents(half.hz, open.hz));
    Expect(open.db - half.db > 6.0, "HalfPressedKeys", "and quieter, dB", open.db - half.db);
    Expect(half.db > -50.0, "HalfPressedKeys", "but still sounding, dBFS RMS", half.db);
    const double slope = (half.hz - gentle.hz) / ((half.pa - gentle.pa) * 1.0e-3);
    Expect(slope < -0.6 * 5.1 && slope > -1.6 * 5.1, "HalfPressedKeys",
           "half-pressed C4 against bellows pressure, Hz/kPa (Okada: -5.1)", slope);
    const Result released = push(0.2, 127, 0.8);
    Expect(std::fabs(Cents(released.hz, open.hz)) < 3.0, "HalfPressedKeys", "pressure let go: back to the open pitch, cents",
           Cents(released.hz, open.hz));
}

void AirButtonVents() {
    const double rate = 48000.0;
    auto played = Make(rate, 3);
    auto vented = Make(rate, 3, [](Controls& c) { c[kAir] = 1.0; });
    const std::vector<Note> notes { { 0.0, { kOn, 69, 100 } } };
    const Take a = Play(*played, rate, 1.8, notes, PullThenPush), b = Play(*vented, rate, 1.8, notes, PullThenPush);
    const double drop = Db(Rms(a.left, rate, 0.5, 1.8)) - Db(Rms(b.left, rate, 0.5, 1.8));
    Expect(std::fabs(MeanPressure(b, rate, 0.5, 1.8)) < 40.0, "AirButtonVents", "pressure with the air button held, Pa",
           MeanPressure(b, rate, 0.5, 1.8));
    Expect(drop > 20.0, "AirButtonVents", "quieter than the same stroke without it, dB", drop);
    // The pedal does the same through CC64.
    auto pedal = Make(rate, 3);
    const Take c = Play(*pedal, rate, 1.8, { { 0.0, { kCc, 64, 127 } }, { 0.0, { kOn, 69, 100 } } }, PullThenPush);
    Expect(std::fabs(MeanPressure(c, rate, 0.5, 1.8)) < 40.0, "AirButtonVents", "pressure with CC64 held, Pa",
           MeanPressure(c, rate, 0.5, 1.8));
}

void AssistScalesTheHand() {
    const double rate = 48000.0;
    const std::vector<Note> chord { { 0.0, { kOn, 57, 100 } }, { 0.0, { kOn, 61, 100 } }, { 0.0, { kOn, 64, 100 } },
                                    { 0.0, { kOn, 33, 100 } }, { 0.0, { kOn, 45, 100 } } };
    auto strict = Make(rate, 7, [](Controls& c) { c[kAssist] = 0.0; });
    auto assisted = Make(rate, 7, [](Controls& c) { c[kAssist] = 1.0; });
    const auto slow = [](double t) { return 0.4 + 0.05 * t; };
    const double p0 = -MeanPressure(Play(*strict, rate, 1.5, chord, slow), rate, 0.5, 1.5);
    const double p1 = -MeanPressure(Play(*assisted, rate, 1.5, chord, slow), rate, 0.5, 1.5);
    Expect(p1 > 3.0 * p0, "AssistScalesTheHand", "full Assist against none on a full chord, pressure ratio", p1 / std::max(p0, 1.0e-9));
}

void AssistHoldsHalfPressedKeys() {
    // Assist counts a held key in full once its pallet is down, so a C3
    // half-pressed at a steady wheel throttles its pallet and the pressure
    // rises into the bend, as when a player pushes the bellows harder. Chosen
    // by ear over counting the pallet's opening, which took the hand's extra
    // air away and let the pressure fall (Docs/decisions.md).
    const double rate = 48000.0;
    const auto pull = [](double s) { return 0.1 + 0.054 * s; };
    auto open = Make(rate, 0), half = Make(rate, 0);
    const Take a = Play(*open, rate, 1.6, { { 0.0, { kOn, 48, 90 } } }, pull);
    const Take b = Play(*half, rate, 1.6, { { 0.0, { kOn, 48, 90 } }, { 0.6, { 0xD0, 127, 0 } } }, pull);
    const double ratio = MeanPressure(b, rate, 1.2, 1.6) / MeanPressure(a, rate, 1.2, 1.6);
    Expect(ratio > 1.5, "AssistHoldsHalfPressedKeys", "half-pressed against open C3 at one wheel speed, pressure ratio", ratio);
}

void WheelPlacementIsOneValue() {
    // A 14-bit pair arriving together places the hand once: no stroke from the
    // MSB alone, read on the 7-bit scale, to the full value.
    const double rate = 48000.0;
    auto e = Make(rate, 7);
    const Take t = Play(*e, rate, 0.3, { { 0.0, { kOn, 57, 100 } }, { 0.0, { kOn, 33, 100 } },
                                         { 0.01, { kCc, 1, 63 } }, { 0.01, { kCc, 33, 0 } } }, {});
    double worst = 0.0;
    for (double p : t.pressure) worst = std::max(worst, std::fabs(p));
    Expect(worst < 1.0, "WheelPlacementIsOneValue", "largest pressure after the first CC1 + CC33, Pa", worst);
}

void CoarseWheelGivesSteadyAir() {
    // A 7-bit wheel opening slowly sends a step every 40 ms; the hand keeps
    // the speed between steps, so the pull does not stutter.
    const double rate = 48000.0;
    std::vector<Note> notes { { 0.0, { kOn, 69, 100 } } };
    for (int i = 0; i <= 50; ++i) notes.push_back({ 0.04 * i, { kCc, 1, static_cast<std::uint8_t>(30 + i) } });
    auto e = Make(rate, 3);
    const Take t = Play(*e, rate, 2.0, notes, {});
    double low = 1.0e9, sum = 0.0;
    int count = 0;
    for (std::size_t i = static_cast<std::size_t>(0.6 * rate / 32); i < static_cast<std::size_t>(1.9 * rate / 32); ++i) {
        low = std::min(low, -t.pressure[i]);
        sum += -t.pressure[i];
        ++count;
    }
    Expect(sum / count > 100.0, "CoarseWheelGivesSteadyAir", "mean draw, Pa", sum / count);
    Expect(low > 0.5 * sum / count, "CoarseWheelGivesSteadyAir", "weakest moment against the mean", low / (sum / count));
}

struct HandTrace {
    std::vector<double> position, velocity;
};

// The hand's path through Diag while the wheel follows `wheel(seconds)` sample by sample.
HandTrace TraceHand(double rate, double seconds, double momentum, const std::function<double(double)>& wheel) {
    auto e = Make(rate, 3, [&](Controls& c) { c[kMomentum] = momentum; });
    HandTrace trace;
    float l = 0.0f, r = 0.0f;
    for (int i = 0; i < static_cast<int>(seconds * rate); ++i) {
        e->SetHand(wheel(i / rate));
        e->Render(&l, &r, 1);
        trace.position.push_back(e->Diag().hand);
        trace.velocity.push_back(e->Diag().handVelocity);
    }
    return trace;
}

// Seconds the velocity takes, after `start`, from 10 % to 90 % of its swing from `from` to `to`.
double SwingSeconds(const HandTrace& t, double rate, double start, double from, double to) {
    int a = -1, b = -1;
    for (int i = static_cast<int>(start * rate); i < static_cast<int>(t.velocity.size()); ++i) {
        if (a < 0 && (t.velocity[static_cast<std::size_t>(i)] - from) * (to - from) > 0.1 * (to - from) * (to - from)) a = i;
        if (b < 0 && (t.velocity[static_cast<std::size_t>(i)] - from) * (to - from) >= 0.9 * (to - from) * (to - from)) b = i;
    }
    return a < 0 || b < 0 ? 1.0e9 : (b - a) / rate;
}

void MomentumSmoothsTheHand() {
    // The wheel opens at 0.2 a second and turns back instantly at 1 s. The arm
    // follows it as a critically damped mass: in a steady stroke it trails by
    // 2 T times the speed, it turns the bellows round in 3.36 T (10-90 % of
    // the velocity swing) and never travels past the wheel.
    const double rate = 48000.0, speed = 0.2, momentum = 25.0, T = momentum * 1.0e-3;
    const auto wheel = [&](double t) { return t < 1.0 ? 0.3 + speed * t : 0.3 + speed * (2.0 - t); };
    const HandTrace direct = TraceHand(rate, 1.5, 0.0, wheel), arm = TraceHand(rate, 1.5, momentum, wheel);
    const double lag = wheel(0.9) - arm.position[static_cast<std::size_t>(0.9 * rate)];
    Expect(std::fabs(lag / (2.0 * T * speed) - 1.0) < 0.05, "MomentumSmoothsTheHand",
           "steady lag against 2 T speed", lag / (2.0 * T * speed));
    const double turn = SwingSeconds(arm, rate, 0.9, speed, -speed);
    Expect(std::fabs(turn / (3.358 * T) - 1.0) < 0.05, "MomentumSmoothsTheHand", "reversal against 3.36 T", turn / (3.358 * T));
    Expect(SwingSeconds(direct, rate, 0.9, speed, -speed) < 0.002, "MomentumSmoothsTheHand",
           "without Momentum the bellows turn with the wheel, s", SwingSeconds(direct, rate, 0.9, speed, -speed));
    double highest = 0.0;
    for (double p : arm.position) highest = std::max(highest, p);
    Expect(highest <= wheel(1.0), "MomentumSmoothsTheHand", "the arm turns before the wheel's furthest point", highest);
    // Shorter than two samples, the arm is held stable and moves with the wheel.
    const HandTrace brief = TraceHand(rate, 1.5, 0.01, wheel);
    double apart = 0.0;
    for (std::size_t i = 0; i < brief.position.size(); ++i)
        apart = std::max(apart, std::isfinite(brief.position[i]) ? std::fabs(brief.position[i] - direct.position[i]) : 1.0e9);
    Expect(apart < 1.0e-4, "MomentumSmoothsTheHand", "0.01 ms Momentum against none, largest position difference", apart);
}

std::vector<Note> Phrase() {
    std::vector<Note> notes { { 0.0, { kOn, 69, 100 } }, { 0.0, { kOn, 33, 90 } }, { 0.3, { kOn, 45, 80 } },
                              { 0.7, { kOff, 45, 0 } }, { 0.9, { kOn, 72, 127 } }, { 1.4, { kOff, 69, 0 } },
                              { 1.6, { kOff, 72, 0 } }, { 1.6, { kOff, 33, 0 } } };
    // The hand as a 14-bit controller stream, every 5 ms.
    std::vector<Note> wheel;
    for (int i = 0; i * 0.005 < 1.8; ++i) {
        const auto v = static_cast<int>(std::lround(PullThenPush(1.6 * i * 0.005) * 16383));
        wheel.push_back({ i * 0.005, { kCc, 1, static_cast<std::uint8_t>(v >> 7) } });
        wheel.push_back({ i * 0.005, { kCc, 33, static_cast<std::uint8_t>(v & 0x7F) } });
    }
    std::vector<Note> all;
    std::size_t a = 0, b = 0;
    while (a < notes.size() || b < wheel.size())
        all.push_back(b == wheel.size() || (a < notes.size() && notes[a].seconds <= wheel[b].seconds) ? notes[a++] : wheel[b++]);
    return all;
}

void BlockPartitionInvariance() {
    const double rate = 48000.0;
    auto e = Make(rate, 7);
    const Take reference = Play(*e, rate, 2.2, Phrase(), {}, 1);
    bool same = true;
    for (int block : { 7, 64, 256, 512 }) {
        auto f = Make(rate, 7);
        const Take t = Play(*f, rate, 2.2, Phrase(), {}, block);
        same = same && t.left == reference.left && t.right == reference.right;
    }
    Expect(same && Peak(reference) > 1.0e-3, "BlockPartitionInvariance", "blocks of 1, 7, 64, 256 and 512 give the same bits",
           Peak(reference));
}

void DeterministicAfterReset() {
    const double rate = 48000.0;
    auto e = Make(rate, 4);
    const Take first = Play(*e, rate, 2.2, Phrase(), {});
    e->Reset();
    const Take second = Play(*e, rate, 2.2, Phrase(), {});
    Expect(first.left == second.left && first.right == second.right, "DeterministicAfterReset",
           "a reset engine plays the phrase to the same bits", Peak(first));
}

void ReleaseAndAllNotesOff() {
    const double rate = 48000.0;
    auto e = Make(rate, 3);
    const Take t = Play(*e, rate, 2.0, { { 0.0, { kOn, 69, 100 } }, { 0.0, { kOn, 33, 100 } }, { 1.0, { kOff, 69, 0 } },
                                         { 1.0, { kCc, 123, 0 } } }, PullThenPush);
    const double after = Rms(t.left, rate, 1.1, 2.0);
    Expect(Db(after) < -100.0, "ReleaseAndAllNotesOff", "100 ms after note-off and CC123, dBFS RMS", Db(after));
    Expect(e->Diag().cells == 0, "ReleaseAndAllNotesOff", "no chamber left sounding", e->Diag().cells);
}

void VoiceLimit() {
    const double rate = 48000.0;
    std::vector<Note> notes;
    for (int k = 48; k < 88; ++k) notes.push_back({ 0.0, { kOn, static_cast<std::uint8_t>(k), 100 } });
    auto e = Make(rate, 7, [](Controls& c) { c[kAssist] = 1.0; });
    const Take t = Play(*e, rate, 1.0, notes, PullThenPush);
    Expect(t.maxCells <= Engine::kMaxCells, "VoiceLimit", "40 keys on five ranks stay within the chamber pool", t.maxCells);
    Expect(std::isfinite(Peak(t)) && Rms(t.left, rate, 0.5, 1.0) > 1.0e-3, "VoiceLimit", "and still sound, RMS", Rms(t.left, rate, 0.5, 1.0));
}

void PresetsSitTogether() {
    // The reference stroke for the factory bank's levels: a bass and a chord
    // button with a C major triad, the wheel opening 18 % in 2 s (eased) and
    // closing back, as a player tries a patch. Every preset sits within 1 dB of
    // Init in RMS there, and peaks below -1.6 dBFS, under the output stage's
    // -1.5 dBFS ceiling, so its peak limiter has nothing to do.
    const double rate = 48000.0;
    const auto stroke = [](double t) {
        const double s = t < 2.0 ? t / 2.0 : 1.0 - (t - 2.0) / 2.0;
        return 0.41 + 0.18 * (0.5 - 0.5 * std::cos(kPi * s));
    };
    std::vector<Note> notes;
    for (int k : { 24, 36, 60, 64, 67 }) notes.push_back({ 0.0, { kOn, static_cast<std::uint8_t>(k), 100 } });
    const auto level = [&](const Controls& controls, double& peak) {
        Engine e;
        e.Prepare(rate);
        e.SetControls(controls);
        e.Reset();
        const Take t = Play(e, rate, 4.0, notes, stroke);
        peak = Peak(t);
        return 0.5 * (Db(Rms(t.left, rate, 0.5, 4.0)) + Db(Rms(t.right, rate, 0.5, 4.0)));
    };
    double initPeak = 0.0, worstPeak = 0.0, worstApart = 0.0;
    const double init = level(Controls {}, initPeak);
    worstPeak = initPeak;
    for (const Preset& preset : kPresets) {
        double peak = 0.0;
        const double apart = level(preset.ToControls(), peak) - init;
        std::printf("     %-16s %+5.2f dB against Init, peak %+6.2f dBFS\n", preset.name, apart, Db(peak));
        worstApart = std::max(worstApart, std::fabs(apart));
        worstPeak = std::max(worstPeak, peak);
    }
    Expect(worstApart < 1.0, "PresetsSitTogether", "largest preset RMS apart from Init on the reference stroke, dB", worstApart);
    Expect(Db(worstPeak) < -1.6, "PresetsSitTogether", "loudest peak there, dBFS", Db(worstPeak));
}

void RegistersSitTogether() {
    // Switching a register changes the colour, not the level: on Init's
    // controls and the reference stroke, a C3, a C4-E4-G4 triad and a C6
    // average within 1 dB of the Musette register's (RespiraEngine.cpp,
    // kRegisterTrimDb).
    const double rate = 48000.0;
    const auto stroke = [](double t) {
        const double s = t < 2.0 ? t / 2.0 : 1.0 - (t - 2.0) / 2.0;
        return 0.41 + 0.18 * (0.5 - 0.5 * std::cos(kPi * s));
    };
    const std::vector<std::vector<int>> gestures { { 48 }, { 60, 64, 67 }, { 84 } };
    std::array<double, 8> level {};
    for (int reg = 0; reg < 8; ++reg) {
        for (const auto& keys : gestures) {
            auto e = Make(rate, 0, [reg](Controls& c) { c = Controls {}; c[kRegister] = reg; });
            std::vector<Note> notes;
            for (int k : keys) notes.push_back({ 0.0, { kOn, static_cast<std::uint8_t>(k), 100 } });
            const Take t = Play(*e, rate, 4.0, notes, stroke);
            level[static_cast<std::size_t>(reg)] += 0.5 * (Db(Rms(t.left, rate, 0.5, 4.0)) + Db(Rms(t.right, rate, 0.5, 4.0))) / 3.0;
        }
    }
    double worst = 0.0;
    for (int reg = 0; reg < 8; ++reg) {
        const double apart = level[static_cast<std::size_t>(reg)] - level[3];
        std::printf("     %-10s %+5.2f dB against Musette\n", kRegisters[static_cast<std::size_t>(reg)].name, apart);
        worst = std::max(worst, std::fabs(apart));
    }
    Expect(worst < 1.0, "RegistersSitTogether", "largest register RMS apart from Musette, dB", worst);
}

void TypicalLevel() {
    // The output reference puts ordinary playing at -9 on Reason's VU meter:
    // the demo phrases' notes read -9.0 VU at their median (measured
    // 2026-10-28). Here a C3, a C4-E4-G4 triad, a C6 and the reference chord
    // on the reference stroke and Init's controls stand in for them; that
    // stroke is firmer than most of the phrases', and their median reads
    // -5.8 VU. A change to the output reference, the output stage or the reed
    // model re-checks both. Key velocity only sets how fast a pallet lifts, so
    // it does not move these levels.
    const double rate = 48000.0;
    const auto stroke = [](double t) {
        const double s = t < 2.0 ? t / 2.0 : 1.0 - (t - 2.0) / 2.0;
        return 0.41 + 0.18 * (0.5 - 0.5 * std::cos(kPi * s));
    };
    const std::vector<std::vector<int>> gestures { { 48 }, { 60, 64, 67 }, { 84 }, { 24, 36, 60, 64, 67 } };
    std::vector<double> levels;
    for (const auto& keys : gestures) {
        auto e = Make(rate, 0, [](Controls& c) { c = Controls {}; });
        std::vector<Note> notes;
        for (int k : keys) notes.push_back({ 0.0, { kOn, static_cast<std::uint8_t>(k), 100 } });
        levels.push_back(Vu(Play(*e, rate, 4.0, notes, stroke), rate));
        std::printf("     %zu keys from %d: %+5.1f VU\n", keys.size(), keys.front(), levels.back());
    }
    std::sort(levels.begin(), levels.end());
    const double median = 0.5 * (levels[levels.size() / 2 - 1] + levels[levels.size() / 2]);
    Expect(std::fabs(median - -5.8) < 1.0, "TypicalLevel", "median VU of the reference gestures", median);
}

void HardStrokesFit() {
    // The reference chord (a bass and a chord button with a C major triad)
    // struck hard, as Reason's Recon test played it: the wheel swept at 60 %
    // of its range per second, and the whole wheel rocked every half second.
    // Both drive the bellows to the arm's full pressure, some 23 dB above
    // ordinary playing; on every preset the output stage keeps the meter out
    // of the red and the peaks at its -1.5 dBFS ceiling.
    const double rate = 48000.0;
    std::vector<Note> notes;
    for (int k : { 24, 36, 60, 64, 67 }) notes.push_back({ 0.0, { kOn, static_cast<std::uint8_t>(k), 127 } });
    const auto sweep = [](double t) { const double p = std::fmod(t, 2.0); return p < 1.0 ? 0.2 + 0.6 * p : 1.4 - 0.6 * p; };
    const auto rock = [](double t) { const double p = std::fmod(t, 1.0); return p < 0.5 ? 2.0 * p : 2.0 - 2.0 * p; };
    double worstPeak = 0.0, worstVu = -100.0, leastPressure = 1.0e9;
    for (int preset = -1; preset < static_cast<int>(kPresets.size()); ++preset) {
        const Controls controls = preset < 0 ? Controls {} : kPresets[static_cast<std::size_t>(preset)].ToControls();
        for (const auto& hand : { std::function<double(double)>(sweep), std::function<double(double)>(rock) }) {
            Engine e;
            e.Prepare(rate);
            e.SetControls(controls);
            e.Reset();
            const Take t = Play(e, rate, 2.0, notes, hand);
            double pressure = 0.0;
            for (double p : t.pressure) pressure = std::max(pressure, std::fabs(p));
            worstPeak = std::max(worstPeak, Peak(t));
            worstVu = std::max(worstVu, Vu(t, rate));
            leastPressure = std::min(leastPressure, pressure);
        }
    }
    Expect(leastPressure >= 2499.0, "HardStrokesFit", "every stroke reaches the arm's full pressure, Pa", leastPressure);
    Expect(worstVu < -1.0, "HardStrokesFit", "loudest on Reason's VU meter, VU", worstVu);
    Expect(Db(worstPeak) < -1.4, "HardStrokesFit", "loudest peak, dBFS", Db(worstPeak));
}

void HardPushSpeaks() {
    // A key held first, then the whole wheel swept in under half a second, on
    // a pull and on a push: the bellows reaches the arm's full pressure before
    // the reed swings. A tongue that pressure could blow into its slot would
    // sit there silent (ReedModel.h, the tip weights); C3-G#4 on Bassoon
    // stayed at -61 to -68 dBFS so. Every one speaks within 0.45 s.
    const double rate = 48000.0;
    double quietest = 0.0;
    for (int key = 48; key <= 68; ++key)
        for (const double direction : { 1.0, -1.0 }) {
            auto e = Make(rate, 1);
            const Take t = Play(*e, rate, 0.45, { { 0.0, { kOn, static_cast<std::uint8_t>(key), 100 } } },
                                [direction](double s) { return 0.5 - direction * (0.4 - 2.0 * s); });
            quietest = std::min(quietest, Db(Rms(t.left, rate, 0.25, 0.45)));
        }
    Expect(quietest > -50.0, "HardPushSpeaks", "quietest held key on a hard pull or push, dBFS RMS", quietest);
}

void LoudestCase() {
    // Every chamber of the pool sounding (every treble reed of 14 keys, bass
    // and chords), Volume and Bass at their tops, full Assist and a hand
    // slamming the bellows back and forth: the output stage still holds the
    // peaks at -1.5 dBFS. DspFuzz and the parameter matrix allow full scale.
    const double rate = 48000.0;
    std::vector<Note> notes;
    for (int k = 60; k < 74; ++k) notes.push_back({ 0.0, { kOn, static_cast<std::uint8_t>(k), 127 } });
    for (int k : { 24, 28, 31, 36, 40, 43 }) notes.push_back({ 0.0, { kOn, static_cast<std::uint8_t>(k), 127 } });
    auto e = Make(rate, 7, [](Controls& c) {
        c[kAssist] = 1.0;
        c[kVolume] = 6.0;
        c[kBass] = 6.0;
        c[kBellows] = 8.0;
    });
    const Take t = Play(*e, rate, 2.0, notes, [](double s) { return 0.5 + 0.5 * std::sin(2.0 * kPi * 3.0 * s); });
    double worst = 0.0;
    for (double p : t.pressure) worst = std::max(worst, std::fabs(p));
    Expect(worst >= 2499.0, "LoudestCase", "the hand reaches the pressure clamp, Pa", worst);
    Expect(Db(Peak(t)) < -1.4, "LoudestCase", "loudest peak, dBFS", Db(Peak(t)));
}

void WidthGlides() {
    // Width moves the pan gains over the levels' 20 ms, so automating it never
    // steps: right after a jump from 0 to 1 the difference it makes is still
    // small. The output stage delays everything by its look-ahead.
    const double rate = 48000.0, at = 0.5;
    auto still = Make(rate, 0, [](Controls& c) { c[kWidth] = 0.0; });
    auto moved = Make(rate, 0, [](Controls& c) { c[kWidth] = 0.0; });
    const std::vector<Note> chord { { 0.0, { kOn, 64, 100 } }, { 0.0, { kOn, 76, 100 } }, { 0.0, { kOn, 36, 100 } } };
    const Take a = Play(*still, rate, at, chord, PullThenPush), b = Play(*moved, rate, at, chord, PullThenPush);
    Controls wide = moved->GetControls();
    wide[kWidth] = 1.0;
    moved->SetControls(wide);
    const auto opening = [at](double s) { return PullThenPush(at + s); };
    const Take c = Play(*still, rate, 0.1, {}, opening), d = Play(*moved, rate, 0.1, {}, opening);
    std::vector<float> diff(c.left.size());
    for (std::size_t i = 0; i < diff.size(); ++i) diff[i] = d.left[i] - c.left[i];
    const double lag = moved->LatencySamples() / rate;
    const double first = Db(Rms(diff, rate, lag, lag + 0.001)), settled = Db(Rms(diff, rate, 0.06, 0.1));
    Expect(a.left == b.left && settled > -60.0, "WidthGlides", "the step changes the left channel, dBFS RMS", settled);
    Expect(first - settled < -15.0, "WidthGlides", "its first ms against its settled change, dB", first - settled);
}

void CascadeVarianceScales() {
    // The breath and the valve hiss divide their two-stage noise by the square
    // root of CascadeVariance, so it has to match the cascade on the engine's
    // own draw, from long eddies to short.
    for (const double c : { 0.02, 0.2, 0.7 }) {
        std::uint32_t noise = 1u;
        double a = 0.0, b = 0.0, sum = 0.0, squares = 0.0;
        const int warm = 10000, count = 4000000;
        for (int i = 0; i < warm + count; ++i) {
            noise = noise * 1664525u + 1013904223u;
            a += c * (1.7320508075688772 * Unit(noise) - a);
            b += c * (a - b);
            if (i < warm) continue;
            sum += b;
            squares += b * b;
        }
        const double variance = squares / count - (sum / count) * (sum / count);
        const double ratio = variance / CascadeVariance(c);
        Expect(std::fabs(ratio - 1.0) < 0.02, "CascadeVarianceScales", "measured over predicted variance", ratio);
    }
}

void NonFiniteControls() {
    const double rate = 48000.0;
    auto e = Make(rate, 3);
    Controls broken;
    for (int i = 0; i < kControlCount; ++i) broken[i] = std::nan("");
    e->SetControls(broken);
    e->SetHand(std::nan(""));
    e->SetMasterTune(std::nan(""));
    e->SetPitchBend(std::nan(""));
    bool defaults = true;
    for (int i = 0; i < kControlCount; ++i) defaults = defaults && e->GetControls()[i] == kControlInfo[static_cast<std::size_t>(i)].initial;
    const Take t = Play(*e, rate, 1.0, { { 0.0, { kOn, 69, 100 } } }, PullThenPush);
    Expect(defaults, "NonFiniteControls", "NaN controls fall back to their defaults", 0);
    Expect(std::isfinite(Peak(t)) && Peak(t) > 1.0e-3, "NonFiniteControls", "and the engine still plays, peak", Peak(t));
}

} // namespace

int main() {
    SilentWithoutMotion();
    MechanicsKnock();
    AirFromMotion();
    TurnWithoutClick();
    StillnessStopsTheSound();
    RatesKeepTheSound();
    OversamplingKeepsTheSound();
    MasterTune();
    PitchBend();
    MusetteBeat();
    SplitKeyboardRouting();
    RegisterActsOnHeldNotes();
    TopOctaveSteady();
    HalfPressedKeys();
    AirButtonVents();
    AssistScalesTheHand();
    AssistHoldsHalfPressedKeys();
    WheelPlacementIsOneValue();
    CoarseWheelGivesSteadyAir();
    MomentumSmoothsTheHand();
    BlockPartitionInvariance();
    DeterministicAfterReset();
    ReleaseAndAllNotesOff();
    VoiceLimit();
    PresetsSitTogether();
    RegistersSitTogether();
    TypicalLevel();
    HardStrokesFit();
    HardPushSpeaks();
    LoudestCase();
    WidthGlides();
    CascadeVarianceScales();
    NonFiniteControls();
    std::printf("%d failure(s)\n", failures);
    return failures == 0 ? 0 : 1;
}
