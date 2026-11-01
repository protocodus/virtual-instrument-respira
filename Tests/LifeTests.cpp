#include "DSP/RespiraEngine.h"

#include <algorithm>
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
    Transport t; t.bpm = 40.0; e->SetTransport(t);
    e->SetHand(automatic ? 0.2 : 0.25);
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
    a->Reset(); a->SetHand(0.2);
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
        Require(a->Diag().pressure == b->Diag().pressure && a->Diag().handVelocity == b->Diag().handVelocity,
                "manual bellows performance is preserved; Life changes only pickup there");
        for (int i = 0; i < 64; ++i) difference += std::fabs(al[i] - bl[i]) + std::fabs(ar[i] - br[i]);
    }
    Require(difference > 0.1, "Life movement reaches the manual-mode microphone pickup");
    auto mono = Make(1.0, true, 0.0);
    const Take m = Hold(*mono, 64, 2 * 48000);
    Require(m.l == m.r, "coincident parallel microphones remain mono while the source moves");
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
void SwellsReserveEnoughAir() {
    // Fixed 120 BPM, five keys and no Assist: the original bellows-capacity
    // witness. Full Life formerly hit the end stop 269 ms before a bar line.
    // Life's stronger swells require beat boundaries when a whole bar no
    // longer fits. Keep the 80 ms turn window against that musical grid.
    auto e = Make(1.0);
    Controls c = e->GetControls(); c[kAssist] = 0.0; e->SetControls(c); e->SetHand(0.0);
    for (const int key : { 60, 64, 67, 36, 24 }) {
        const std::uint8_t note[] = { 0x90, static_cast<std::uint8_t>(key), 100 }; e->Midi(note, 3);
    }
    double previous = 0.0, worst = 0.0; int turns = 0;
    float l[32], r[32];
    for (int frame = 0; frame < int(8.6 * 48000); frame += 32) {
        Transport t; t.playing = true; t.bpm = 120.0;
        t.position = frame / 24000.0; t.barStart = 4.0 * std::floor(t.position / 4.0);
        e->SetTransport(t); e->Render(l, r, 32);
        const double pressure = e->Diag().pressure, seconds = (frame + 32) / 48000.0;
        if (seconds > 0.5 && (previous < 0.0) != (pressure < 0.0)) {
            ++turns; worst = std::max(worst, 0.5 * std::ceil(seconds / 0.5) - seconds);
        }
        previous = pressure;
    }
    std::printf("Life air reserve: %d turns, furthest %.4f s before a beat\n", turns, worst);
    Require(turns >= 3 && worst < 0.08, "Life swells turn on beats before exhausting the bellows");
}
void AShortRestPreparesTheNextNote() {
    for (const double amount : { 0.0, 1.0 }) {
        auto e = Make(amount); Note(*e, true);
        float l[48], r[48]; double beforeNote = 0.0, energy = 0.0;
        for (int ms = 0; ms < 8100; ++ms) {
            Transport t; t.playing = true; t.bpm = 120.0;
            t.position = ms / 500.0; t.barStart = 4.0 * std::floor(t.position / 4.0);
            e->SetTransport(t);
            if (ms == 7820) Note(*e, false);
            if (ms == 8000) { beforeNote = e->Diag().pressure; Note(*e, true); }
            e->Render(l, r, 48);
            if (ms >= 8020) for (float x : l) energy += x * x;
        }
        std::printf("Life %.0f rest: %.1f Pa before the next note, %.1f dBFS short-note RMS\n",
                    amount, beforeNote, 10.0 * std::log10(energy / (80 * 48)));
        Require(beforeNote > 200.0, "Auto reverses physical pressure during the rest before the note");
        Require(energy / (80 * 48) > 1.0e-5, "the first 100 ms note after the turn remains audible");
    }
}
}

int main() {
    HeldNotesKeepMoving(); BlocksAndResetDoNotChangeThePlayer();
    ManualControlKeepsItsAirAndMonoStaysMono(); NoHandStillMeansNoAir();
    SwellsReserveEnoughAir();
    AShortRestPreparesTheNextNote();
    std::puts("PASS Life: held notes, deterministic blocks/reset, manual air, moving pickup, mono and silence");
}
