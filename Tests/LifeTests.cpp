#include "DSP/RespiraEngine.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <vector>

using namespace respira;
namespace {
void Require(bool ok, const char* what) {
    if (!ok) { std::fprintf(stderr, "FAIL Life: %s\n", what); std::exit(1); }
}
void Note(Engine& e, bool on) {
    const std::uint8_t m[] = { static_cast<std::uint8_t>(on ? 0x90 : 0x80), 69, 100 };
    e.Midi(m, 3);
}
std::unique_ptr<Engine> Make(double amount, bool automatic = true, double width = 0.6) {
    auto e = std::make_unique<Engine>();
    e->Prepare(48000.0);
    Controls c; c[kLife] = amount; c[kAuto] = automatic ? 1.0 : 0.0;
    c[kTouch] = 0.0; // isolate Life; TouchTests and musical renders cover their combination
    c[kWidth] = width; c[kRoom] = 0.0;
    e->SetControls(c); e->Reset();
    // In Auto mode, wheel=0 is the full 400 Pa baseline (raising the wheel
    // slows the bellows down).
    e->SetHand(automatic ? 0.0 : 0.25);
    return e;
}
struct Take { std::vector<float> l, r; std::vector<double> p; };
Take Hold(Engine& e, int block, int frames = 8 * 48000) {
    Take t; t.l.resize(frames); t.r.resize(frames);
    Note(e, true);
    for (int at = 0; at < frames; at += block) {
        const int n = std::min(block, frames - at);
        e.Render(t.l.data() + at, t.r.data() + at, n);
        if (at >= 2 * 48000 && at < 7 * 48000) t.p.push_back(std::fabs(e.Diag().pressure));
    }
    return t;
}
double Variation(const std::vector<double>& x) {
    double mean = 0.0, variance = 0.0;
    for (double v : x) mean += v / x.size();
    for (double v : x) variance += (v - mean) * (v - mean) / x.size();
    Require(mean > 350.0 && mean < 900.0, "held note retains moderate bellows pressure");
    return std::sqrt(variance) / mean;
}
void HeldNotesKeepMoving() {
    auto a = Make(0.0), b = Make(1.0), middle = Make(0.5);
    const Take old = Hold(*a, 64), live = Hold(*b, 64), half = Hold(*middle, 64);
    const double va = Variation(old.p), vb = Variation(live.p), vm = Variation(half.p);
    Require(vm > va && vm < vb, "half Life gives an intermediate depth on the same held note");
    Require(vb > 1.4 * va && vb < 0.4, "Life increases bounded, ongoing pressure variation without new notes");
    Require(*std::min_element(live.p.begin(), live.p.end()) > 200.0, "Life never stops the held reed's air supply");
    std::printf("PASS Life held-note pressure CV: %.4f -> %.4f\n", va, vb);
}
void BlocksAndResetDoNotChangeThePlayer() {
    auto a = Make(0.7), b = Make(0.7);
    const Take x = Hold(*a, 64, 3 * 48000), y = Hold(*b, 17, 3 * 48000);
    Require(x.l == y.l && x.r == y.r, "same audio at different host block sizes");
    a->Reset(); a->SetHand(0.0);
    const Take z = Hold(*a, 127, 3 * 48000);
    Require(x.l == z.l && x.r == z.r, "reset repeats the virtual player exactly");
}
void ManualControlKeepsItsAirAndMonoStaysMono() {
    auto a = Make(0.0, false), b = Make(1.0, false);
    Note(*a, true); Note(*b, true);
    float al[64], ar[64], bl[64], br[64]; double difference = 0.0;
    for (int at = 0; at < 3 * 48000; at += 64) {
        const double hand = 0.25 + 0.14 * at / 48000.0;
        a->SetHand(hand); b->SetHand(hand);
        a->Render(al, ar, 64); b->Render(bl, br, 64);
        Require(a->Diag().hand == b->Diag().hand && a->Diag().handVelocity == b->Diag().handVelocity
                && std::fabs(a->Diag().pressure - b->Diag().pressure) < 80.0,
                "manual bellows trajectory is preserved; pressure differs only by the physical reed tolerance");
        for (int i = 0; i < 64; ++i) difference += std::fabs(al[i] - bl[i]) + std::fabs(ar[i] - br[i]);
    }
    Require(difference > 0.1, "Life movement and reed tolerances reach the manual-mode output");
    auto mono = Make(1.0, true, 0.0);
    const Take m = Hold(*mono, 64, 2 * 48000);
    Require(m.l == m.r, "coincident parallel microphones remain mono while the source moves");
}
void LifeScalesReedAndChamberImperfections() {
    Require(kControlInfo[kLife].initial == 0.5, "default Life is 0.5 (standard well-made unit)");
    const ReedVariation v0 = Variation(kRankM, 33, 0, 0.0);
    const ReedVariation vMid = Variation(kRankM, 33, 0, 0.5);
    const ReedVariation vMax = Variation(kRankM, 33, 0, 1.0);
    Require(v0.setScale == 1.0 && v0.qScale == 1.0 && v0.tuneCents == 0.0
            && v0.clearanceScale == 1.0 && v0.plateScale == 1.0 && v0.profileScale == 1.0
            && v0.chamberScale == 1.0 && v0.ventScale == 1.0,
            "Life 0 means perfection (zero reed/chamber variation)");
    Require(std::fabs((vMax.tuneCents) - 2.0 * (vMid.tuneCents)) < 1.0e-12
            && std::fabs((vMax.clearanceScale - 1.0) - 2.0 * (vMid.clearanceScale - 1.0)) < 1.0e-12,
            "Life 1.0 over-emphasizes variation to 2x the standard Life 0.5 unit");
    const CellSpec c0 = MakeCell(kRankM, 69, 22.0, nullptr, 192000.0, 0.0);
    Require(c0.voiceGain == 1.0 && c0.push.rest == c0.pull.rest && c0.push.clearArea == c0.pull.clearArea
            && c0.push.vortexShare == 0.0,
            "Life 0 produces identical push/pull reeds with zero vortex imperfection");
}
void NoHandStillMeansNoAir() {
    auto e = Make(1.0, false);
    const Take t = Hold(*e, 64, 2 * 48000);
    Require(e->Diag().pressure == 0.0 && e->Diag().handVelocity == 0.0,
            "Life cannot supply air to a stationary manual bellows");
    double tail = 0.0;
    for (std::size_t i = 48000; i < t.l.size(); ++i) tail = std::max(tail, std::fabs(double(t.l[i])));
    Require(tail < 1.0e-9, "held keys without hand movement remain silent after their mechanical knock");
}
void SwellsNeverRunTheBellowsOut() {
    // Five keys and no Assist at full Life: the old bellows-capacity witness,
    // which turned on beats to spare its 6 L. Auto's bellows are endless now,
    // so its strongest swells never force a turn.
    auto e = Make(1.0);
    Controls c = e->GetControls(); c[kAssist] = 0.0; e->SetControls(c); e->SetHand(0.0);
    for (const int key : { 60, 64, 67, 36, 24 }) {
        const std::uint8_t note[] = { 0x90, static_cast<std::uint8_t>(key), 100 }; e->Midi(note, 3);
    }
    double previous = 0.0; int turns = 0;
    float l[32], r[32];
    for (int frame = 0; frame < int(8.6 * 48000); frame += 32) {
        e->Render(l, r, 32);
        const double pressure = e->Diag().pressure, seconds = (frame + 32) / 48000.0;
        if (seconds > 0.5 && (previous < 0.0) != (pressure < 0.0)) ++turns;
        previous = pressure;
    }
    std::printf("Life swells: %d turns in 8.6 s\n", turns);
    Require(turns == 0, "Life's swells never run the endless bellows out");
}
void AKeyswitchInARestPreparesTheNextNote() {
    // C#0 (MIDI 13) in a short rest turns the pressure with the pallets shut,
    // so the next note speaks pushing at once.
    for (const double amount : { 0.0, 1.0 }) {
        auto e = Make(amount); Note(*e, true);
        float l[48], r[48]; double beforeNote = 0.0, energy = 0.0;
        for (int ms = 0; ms < 8100; ++ms) {
            if (ms == 7820) Note(*e, false);
            if (ms == 7900) { const std::uint8_t push[] = { 0x90, 13, 100 }; e->Midi(push, 3); }
            if (ms == 8000) { beforeNote = e->Diag().pressure; Note(*e, true); }
            e->Render(l, r, 48);
            if (ms >= 8020) for (float x : l) energy += x * x;
        }
        std::printf("Life %.0f rest: %.1f Pa before the next note, %.1f dBFS short-note RMS\n",
                    amount, beforeNote, 10.0 * std::log10(energy / (80 * 48)));
        Require(beforeNote > 200.0, "a keyswitch in the rest turns the physical pressure before the note");
        Require(energy / (80 * 48) > 1.0e-5, "the first 100 ms note after the turn remains audible");
    }
}
// A bass, a chord and a triad: open reeds that ask Assist for several times
// one reed's air.
void Chord(Engine& e, bool on) {
    const std::uint8_t s = on ? 0x90 : 0x80;
    for (const auto& n : { std::array<std::uint8_t, 2>{ s, 60 }, { s, 64 }, { s, 67 },
                           { std::uint8_t(s | 1), 36 }, { std::uint8_t(s | 2), 48 } }) {
        const std::uint8_t m[] = { n[0], n[1], 90 }; e.Midi(m, 3);
    }
}
void ATurnInAShortRestArrivesAtItsPressure() {
    // The chord is let go, C#0 turns the bellows 30 ms later and the chord
    // comes back 30 ms after that, while the arm still swings. Assist once
    // multiplied the swinging arm's motion by the chord's demand the moment
    // its keys went down: 4.5 times the pressure it settled at (2 kPa).
    auto e = Make(0.0);
    Chord(*e, true);
    float l[48], r[48]; double peak = 0.0, settled = 0.0;
    for (int ms = 0; ms < 4200; ++ms) {
        if (ms == 3000) Chord(*e, false);
        if (ms == 3030) { const std::uint8_t push[] = { 0x90, 13, 100 }; e->Midi(push, 3); }
        if (ms == 3060) Chord(*e, true);
        e->Render(l, r, 48);
        const double p = std::fabs(e->Diag().pressure);
        if (ms >= 3060 && ms < 3460) peak = std::max(peak, p);
        if (ms >= 3700) settled += p / 500.0;
    }
    std::printf("Life short-rest turn: peak %.0f Pa, settled %.0f Pa\n", peak, settled);
    Require(peak < 1.3 * settled, "a turn in a short rest arrives at its pressure, not several times over it");
}
void AFirstNoteSpeaksAtOnce() {
    // Auto's first stroke finds no pressure, and swings it up at the hand's
    // full speed as a turn does; closing on it in a reaction time took 120 ms.
    auto e = Make(0.0);
    float l[48], r[48]; std::vector<double> p;
    Note(*e, true);
    for (int ms = 0; ms < 1500; ++ms) { e->Render(l, r, 48); p.push_back(std::fabs(e->Diag().pressure)); }
    double steady = 0.0; for (int ms = 800; ms < 1500; ++ms) steady += p[ms] / 700.0;
    int rise = 0; while (rise < 1500 && p[rise] < 0.9 * steady) ++rise;
    std::printf("Life first note: 90 %% of its pressure in %d ms\n", rise);
    Require(rise <= 40, "the first note's bellows reach its pressure within 40 ms");
}
void TheTremorHasUnitRms() {
    // Life.h scales the 6.5 Hz tremor to unit RMS before kTremorShare weights
    // it. Only the +-1.5 clamp takes a little off a unit-variance, nearly
    // Gaussian signal (0.88 RMS for a Gaussian); the scale once left it at 0.4.
    Life life; life.Prepare(48000.0);
    double sum = 0.0;
    const int n = 200000;
    for (int i = 0; i < n; ++i) { const double y = life.effortTremor.Tick(life.Draw()); sum += y * y; }
    const double rms = std::sqrt(sum / n);
    Require(rms > 0.85 && rms < 0.92, "the tremor is scaled to unit RMS");
}
}

int main() {
    TheTremorHasUnitRms();
    HeldNotesKeepMoving(); BlocksAndResetDoNotChangeThePlayer();
    ManualControlKeepsItsAirAndMonoStaysMono(); NoHandStillMeansNoAir();
    LifeScalesReedAndChamberImperfections();
    SwellsNeverRunTheBellowsOut();
    AKeyswitchInARestPreparesTheNextNote();
    ATurnInAShortRestArrivesAtItsPressure(); AFirstNoteSpeaksAtOnce();
    std::puts("PASS Life: held notes, deterministic blocks/reset, manual air, moving pickup, mono and silence");
}
