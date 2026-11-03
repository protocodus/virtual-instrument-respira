// Playing contracts, not a claim to match one measured accordion. The paper
// comparison and its acknowledged low-pressure gap are recorded separately.
#include "DSP/RespiraEngine.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <vector>

using namespace respira;
namespace {
constexpr int kRate = 48000;
void Require(bool ok, const char* message) {
    if (!ok) { std::fprintf(stderr, "FAIL Touch: %s\n", message); std::exit(1); }
}
void Note(Engine& e, int key, int velocity, bool on = true) {
    const std::uint8_t m[] = { static_cast<std::uint8_t>(on ? 0x90 : 0x80),
                              static_cast<std::uint8_t>(key), static_cast<std::uint8_t>(velocity) };
    e.Midi(m, 3);
}
std::unique_ptr<Engine> Make(double touch, bool automatic = true) {
    auto e = std::make_unique<Engine>(); e->Prepare(kRate);
    Controls c; c[kTouch] = touch; c[kLife] = 0.0; c[kAuto] = automatic ? 1.0 : 0.0;
    c[kKeyboard] = 1.0; c[kRoom] = 0.0; c[kMechanics] = -40.0;
    // In Auto mode, wheel=0 is the full 400 Pa baseline (raising the wheel
    // slows the bellows down).
    e->SetControls(c); e->Reset(); e->SetHand(automatic ? 0.0 : 0.2);
    Transport t; t.bpm = 40.0; e->SetTransport(t);
    return e;
}
struct Take { std::vector<float> l, r; std::vector<double> p; };
Take Hold(Engine& e, int velocity, double seconds = 3.0) {
    Take t; const int frames = static_cast<int>(seconds * kRate);
    t.l.resize(frames); t.r.resize(frames); t.p.reserve(frames / 32);
    Note(e, 69, velocity);
    for (int at = 0; at < frames; at += 32) {
        const int count = std::min(32, frames - at);
        e.Render(t.l.data() + at, t.r.data() + at, count);
        t.p.push_back(std::fabs(e.Diag().pressure));
    }
    return t;
}
double Rms(const std::vector<float>& a, double begin, double end) {
    double sum = 0.0;
    const int lo = static_cast<int>(begin * kRate), hi = static_cast<int>(end * kRate);
    for (int i = lo; i < hi; ++i) sum += double(a[i]) * a[i];
    return std::sqrt(sum / (hi - lo));
}
double MeanPressure(const Take& t, double begin, double end) {
    const int lo = static_cast<int>(begin * kRate / 32), hi = static_cast<int>(end * kRate / 32);
    double sum = 0.0; for (int i = lo; i < hi; ++i) sum += t.p[i];
    return sum / (hi - lo);
}
double Pitch(const std::vector<float>& x) {
    // Half-second Hann sinusoid projection at 0.1 Hz resolution around A4.
    // It is a settled pitch check, not a tracker of the noisy first cycles.
    const int begin = 2 * kRate, size = kRate / 2;
    double bestPower = 0.0, best = 0.0;
    for (int bin = 4300; bin <= 4500; ++bin) {
        const double hz = bin * 0.1, phase = 2.0 * kPi * hz / kRate;
        const double cr = std::cos(phase), ci = std::sin(phase);
        double re = 1.0, im = 0.0, real = 0.0, imag = 0.0;
        for (int i = 0; i < size; ++i) {
            const double value = x[begin + i] * (0.5 - 0.5 * std::cos(2.0 * kPi * i / size));
            real += value * re; imag += value * im;
            const double r = re * cr - im * ci; im = im * cr + re * ci; re = r;
        }
        const double power = real * real + imag * imag;
        if (power > bestPower) { bestPower = power; best = hz; }
    }
    return best;
}
void TouchDoesNotOverrideTheManualHand() {
    auto a = Make(0.0, false), b = Make(1.0, false);
    float al[32], ar[32], bl[32], br[32]; double peak = 0.0;
    for (int at = 0; at < 2 * kRate; at += 32) {
        if (at % 24000 == 0) {
            a->AllNotesOff(); b->AllNotesOff();
            const int velocity = at % 48000 == 0 ? 16 : 127;
            Note(*a, 69, velocity); Note(*b, 69, velocity);
        }
        const double hand = 0.2 + 0.12 * at / kRate;
        a->SetHand(hand); b->SetHand(hand);
        a->Render(al, ar, 32); b->Render(bl, br, 32);
        Require(a->Diag().pressure == b->Diag().pressure && a->Diag().handVelocity == b->Diag().handVelocity,
                "Touch must not supply, modulate or replace the manual hand's air");
        for (int i = 0; i < 32; ++i) {
            Require(al[i] == bl[i] && ar[i] == br[i], "manual audio is bit-identical at Touch 0/100%");
            peak = std::max(peak, std::fabs(double(al[i])));
        }
    }
    Require(peak > 0.01, "manual bypass comparison is audible");
    std::printf("PASS Touch manual hand/audio exact bypass\n");
}
void SlowKeysChangeAttackWithoutPermanentGainOrTuning() {
    auto a = Make(0.0), b = Make(0.0);
    // A finger attack requires pre-tensioned bellows. Starting both hand and
    // key from rest tests a bellows attack instead; its changing pressure can
    // make the slow valve's first noise transient louder, not softer.
    for (Engine* e : {a.get(), b.get()}) {
        Hold(*e, 100, 0.6); Note(*e, 69, 0, false);
        std::vector<float> l(9600), r(9600); e->Render(l.data(), r.data(), 9600);
        // At wheel=0 (400 Pa baseline), the preloaded bellows sits near 400 Pa.
        Require(std::fabs(e->Diag().pressure) > 350.0, "finger-attack fixture preloads the bellows");
    }
    const Take soft = Hold(*a, 8), hard = Hold(*b, 127);
    const double first = Rms(soft.l, 0.002, 0.025) / Rms(hard.l, 0.002, 0.025);
    const double settledDb = 20.0 * std::log10(Rms(soft.l, 2.0, 2.5) / Rms(hard.l, 2.0, 2.5));
    const double pressureRatio = MeanPressure(soft, 2.0, 2.5) / MeanPressure(hard, 2.0, 2.5);
    const double cents = 1200.0 * std::log2(Pitch(soft.l) / Pitch(hard.l));
    std::printf("Touch velocity 8/127: early RMS ratio %.4f, settled %.4f dB, pressure %.6f, pitch %.3f cents\n",
                first, settledDb, pressureRatio, cents);
    Require(first < 0.6, "a deliberately slow key changes the initial reed attack");
    Require(std::fabs(settledDb) < 0.5 && std::fabs(pressureRatio - 1.0) < 0.01,
            "fully open keys at matched bellows pressure have no lasting velocity gain");
    Require(std::fabs(cents) < 1.0, "strike velocity must not leave a pitch offset");
}
void OneSharedGestureDoesNotStackOrLatch() {
    // The virtual performer, independent of the physical air load of a chord:
    // unison ranks and note ordering must never multiply the effort request.
    Touch one, chord; one.Prepare(kRate); chord.Prepare(kRate);
    for (int i = 0; i < kRate; ++i) { one.Step(1.0, true); chord.Step(1.0, true); }
    one.Press(127);
    for (const int v : { 30, 100, 127, 64, 80 }) chord.Press(v);
    for (int i = 0; i < 2 * kRate; ++i) {
        one.Step(1.0, true); chord.Step(1.0, true);
        Require(one.Effort() == chord.Effort(), "chord emphasis is strongest touch, not summed impulses");
    }
    Require(std::fabs(one.Effort()) < 0.0001, "the emphasis relaxes instead of altering a held note forever");

    auto a = Make(0.0), b = Make(0.5), c = Make(1.0);
    const Take off = Hold(*a, 127), half = Hold(*b, 127), full = Hold(*c, 127);
    const double po = MeanPressure(off, 0.08, 0.30), ph = MeanPressure(half, 0.08, 0.30), pf = MeanPressure(full, 0.08, 0.30);
    std::printf("Touch 0/50/100 strong-note pressure: %.2f / %.2f / %.2f Pa\n", po, ph, pf);
    Require(ph > po * 1.02 && pf > ph * 1.02, "Touch reaches Auto's physical bellows, with intermediate depth");
    Require(std::fabs(MeanPressure(full, 2.0, 2.5) / MeanPressure(off, 2.0, 2.5) - 1.0) < 0.01,
            "held-note pressure returns to the wheel's request after the accent");
}
void EventsAndResetAreIndependentOfBlockSize() {
    const auto play = [](Engine& e, int block) {
        constexpr int frames = 2 * kRate;
        std::vector<float> out(frames * 2); float l[127], r[127];
        int at = 0, event = 0;
        while (at < frames) {
            if (at == event * 6001 && event < 15) {
                e.AllNotesOff(); Note(e, 60 + event % 12, 8 + event * 8); ++event;
            }
            const int boundary = event < 15 ? event * 6001 : frames;
            const int n = std::min({block, frames - at, boundary - at});
            e.Render(l, r, n);
            for (int i = 0; i < n; ++i) { out[2 * (at + i)] = l[i]; out[2 * (at + i) + 1] = r[i]; }
            at += n;
        }
        return out;
    };
    auto a = Make(1.0), b = Make(1.0);
    const auto x = play(*a, 64), y = play(*b, 17);
    Require(x == y, "gesture timing is sample-driven across event/block partitions");
    a->Reset(); a->SetHand(0.0);
    Require(x == play(*a, 127), "Reset clears all touch history");
    std::printf("PASS Touch block partition and reset identity\n");
}
void OrdinaryStaccatoSpeaks() {
    auto e = Make(1.0);
    Controls c = e->GetControls(); c[kLife] = 1.0; e->SetControls(c);
    Hold(*e, 100, 0.8); e->AllNotesOff();
    std::vector<float> l(4800), r(4800); e->Render(l.data(), r.data(), 4800);
    double weakest = 1.0;
    for (int i = 0; i < 24; ++i) {
        Note(*e, 64 + i % 18, 64 + (i * 7) % 64);
        e->Render(l.data(), r.data(), 2400); // 50 ms key, followed by 20 ms silence
        weakest = std::min(weakest, Rms(l, 0.02, 0.05));
        e->AllNotesOff(); e->Render(l.data(), r.data(), 960);
    }
    std::printf("Touch 24 staccato notes: weakest final-30-ms RMS %.6f\n", weakest);
    Require(weakest > 0.002, "ordinary fast notes remain audible at full Life/Touch");
}
void SoftStaccatoSpeaks() {
    // A soft strike still completes an ordinary short keystroke. Test actual
    // audio through the public engine with the stronger Touch/Life defaults;
    // do not merely assert the travel formula. 0.1.5's inverse-velocity
    // extrapolation could keep the pallet nearly shut for the whole 50 ms.
    // This is a finger-attack regression at normal preloaded bellows pressure,
    // not a claim that fresh reeds at barely sustaining pressure start fast.
    for (const int velocity : {1, 8, 16}) {
        auto e = Make(1.0);
        Controls c = e->GetControls(); c[kLife] = 1.0; e->SetControls(c);
        Hold(*e, 100, 0.8); e->AllNotesOff();
        std::vector<float> l(4800), r(4800); e->Render(l.data(), r.data(), 4800);
        double weakest = 1.0;
        for (int i = 0; i < 24; ++i) {
            Note(*e, 64 + i % 18, velocity);
            e->Render(l.data(), r.data(), 2400);
            weakest = std::min(weakest, Rms(l, 0.02, 0.05));
            e->AllNotesOff(); e->Render(l.data(), r.data(), 960);
        }
        std::printf("Touch soft staccato v%d: weakest final-30-ms RMS %.6f\n", velocity, weakest);
        Require(weakest > 0.002, "soft short notes must pass air and remain audible");
    }
}
void VelocityAndWheelTransitionBothWays() {
    // 1. High velocity -> low via mod wheel (0 -> 1), and low velocity -> high
    // via mod wheel (0 -> 1).
    {
        auto loud = Make(1.0), soft = Make(1.0);
        Note(*loud, 69, 115); Note(*soft, 69, 16);
        std::vector<float> ll(2 * kRate), lr(2 * kRate), sl(2 * kRate), sr(2 * kRate);
        for (int i = 0; i < 2 * kRate; i += 32) {
            if (i == kRate) { loud->SetHand(1.0); soft->SetHand(1.0); }
            loud->Render(ll.data() + i, lr.data() + i, 32);
            soft->Render(sl.data() + i, sr.data() + i, 32);
        }
        const double loudToSoftDb = 20.0 * std::log10(Rms(ll, 0.4, 0.9) / Rms(ll, 1.4, 1.9));
        const double softToLoudDb = 20.0 * std::log10(Rms(sl, 1.4, 1.9) / Rms(sl, 0.4, 0.9));
        std::printf("Touch wheel transitions: high->low %.2f dB, low->high %.2f dB\n", loudToSoftDb, softToLoudDb);
        Require(loudToSoftDb > 12.0, "raising the wheel on a strong note slows the bellows down");
        Require(softToLoudDb > 4.5, "raising the wheel on a soft note swells the bellows up");
    }
    // 2. Legato velocity transitions: soft held note -> loud note -> release
    // back to the held soft note.
    {
        auto e = Make(1.0);
        std::vector<float> l(3 * kRate), r(3 * kRate);
        Note(*e, 69, 16);
        for (int i = 0; i < 3 * kRate; i += 32) {
            if (i == kRate) Note(*e, 72, 115);
            if (i == 2 * kRate) Note(*e, 72, 0, false);
            e->Render(l.data() + i, r.data() + i, 32);
        }
        const double upDb = 20.0 * std::log10(Rms(l, 1.4, 1.9) / Rms(l, 0.5, 0.9));
        const double downDb = 20.0 * std::log10(Rms(l, 1.4, 1.9) / Rms(l, 2.5, 2.9));
        std::printf("Touch legato velocity low->high->low: +%.2f dB then -%.2f dB\n", upDb, downDb);
        Require(upDb > 4.5 && downDb > 4.5, "legato velocity changes bellows speed both ways");
    }
    // 3. Manual mode: gentle wheel speed (pp) catches promptly and stays
    // clearly audible, while speeding up and slowing down on a stroke changes
    // dynamic level both ways.
    {
        auto m = Make(1.0, false);
        std::vector<float> l(3 * kRate), r(3 * kRate);
        double hand = 0.2;
        m->SetHand(hand);
        Note(*m, 69, 20);
        for (int i = 0; i < 3 * kRate; i += 32) {
            const double t = double(i) / kRate;
            const double speed = t < 1.0 ? 0.045 : (t < 2.0 ? 0.11 : 0.040);
            hand += speed * (32.0 / kRate);
            m->SetHand(hand);
            m->Render(l.data() + i, r.data() + i, 32);
        }
        const double ppEarlyDb = 20.0 * std::log10(std::max(Rms(l, 0.25, 0.9), 1.0e-12));
        const double ffMidDb = 20.0 * std::log10(std::max(Rms(l, 1.35, 1.95), 1.0e-12));
        const double ppLateDb = 20.0 * std::log10(std::max(Rms(l, 2.35, 2.95), 1.0e-12));
        std::printf("Manual slow->fast->slow: %.2f dBFS -> %.2f dBFS -> %.2f dBFS\n", ppEarlyDb, ffMidDb, ppLateDb);
        Require(ppEarlyDb > -30.0 && ppLateDb > -31.0, "manual low speed remains clearly audible");
        Require(ffMidDb - ppEarlyDb > 4.0 && ffMidDb - ppLateDb > 4.0, "manual wheel speed changes level both ways");
    }
}
} // namespace

int main() {
    TouchDoesNotOverrideTheManualHand();
    SlowKeysChangeAttackWithoutPermanentGainOrTuning();
    OneSharedGestureDoesNotStackOrLatch();
    EventsAndResetAreIndependentOfBlockSize();
    OrdinaryStaccatoSpeaks();
    SoftStaccatoSpeaks();
    VelocityAndWheelTransitionBothWays();
    return 0;
}
