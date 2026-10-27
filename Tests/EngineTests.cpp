// Engine contracts: air only from the hand's motion, pitch and pitch bend, rates, routing,
// the air button and Assist, the controller stream, block-partition
// invariance and determinism, release and voice limits.
#include "DSP/RespiraEngine.h"
#include "DSP/RespiraPresets.h"

#include <cmath>
#include <cstdio>
#include <functional>
#include <memory>
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

void SilentWithoutMotion() {
    auto e = Make(48000.0, 3);
    const Take t = Play(*e, 48000.0, 1.0, { { 0.0, { kOn, 69, 100 } }, { 0.0, { kOn, 24, 100 } } }, [](double) { return 0.5; });
    Expect(Peak(t) < 1.0e-6, "SilentWithoutMotion", "a still hand moves no air: peak below -120 dBFS", Peak(t));
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

void StillnessStopsTheSound() {
    const double rate = 48000.0;
    auto e = Make(rate, 3);
    const Take t = Play(*e, rate, 3.5, { { 0.0, { kOn, 69, 100 } } },
                        [](double s) { return s < 1.5 ? 0.3 + 0.2 * s : 0.6; });
    const double moving = Rms(t.left, rate, 0.8, 1.4), still = Rms(t.left, rate, 3.0, 3.5);
    Expect(Db(moving) - Db(still) > 30.0, "StillnessStopsTheSound", "level drop once the hand stops, dB", Db(moving) - Db(still));
}

void PitchAcrossRates() {
    double cents[3] = {};
    const double rates[3] = { 44100.0, 48000.0, 96000.0 };
    for (int i = 0; i < 3; ++i) {
        auto e = Make(rates[i], 0);
        const Take t = Play(*e, rates[i], 2.0, { { 0.0, { kOn, 69, 100 } } }, PullThenPush);
        cents[i] = Cents(PeakHz(t.left, rates[i], 0.8, 1.8, 440.0), 440.0);
    }
    Expect(std::fabs(cents[0] - cents[1]) < 2.0, "PitchAcrossRates", "44.1 kHz against 48 kHz, cents", cents[0] - cents[1]);
    Expect(std::fabs(cents[2] - cents[1]) < 2.0, "PitchAcrossRates", "96 kHz against 48 kHz, cents", cents[2] - cents[1]);
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

void LoudestCase() {
    // Every chamber of the pool sounding (every treble reed of 14 keys, bass
    // and chords), Volume and Bass at their tops, full Assist and a hand
    // slamming the bellows back and forth: the pressure clamp keeps the output
    // bounded. DspFuzz and the parameter matrix allow twice this bound.
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
    Expect(Peak(t) < 32.0, "LoudestCase", "peak stays below +30 dBFS, linear", Peak(t));
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
    AirFromMotion();
    StillnessStopsTheSound();
    PitchAcrossRates();
    MasterTune();
    PitchBend();
    MusetteBeat();
    SplitKeyboardRouting();
    AirButtonVents();
    AssistScalesTheHand();
    WheelPlacementIsOneValue();
    CoarseWheelGivesSteadyAir();
    BlockPartitionInvariance();
    DeterministicAfterReset();
    ReleaseAndAllNotesOff();
    VoiceLimit();
    LoudestCase();
    NonFiniteControls();
    std::printf("%d failure(s)\n", failures);
    return failures == 0 ? 0 : 1;
}
