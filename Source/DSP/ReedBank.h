#pragma once

// The reed blocks of the instrument: which reeds each register rank sounds
// for a key, how each push and pull reed differs from its neighbours, and the
// reed and chamber a sounding cell is built from. Shared by the engine and by
// Tools/GenerateReedTuning.cpp, which tunes every reed offline.

#include "ReedModel.h"

#include <cmath>
#include <cstdint>

namespace respira {

enum Rank : int {
    kRankL, kRankM, kRankMMinus, kRankMPlus, kRankH,   // treble: 16', 8', 8' low, 8' high, 4'
    kRankBass16, kRankBass8, kRankChord8, kRankChord4,  // bass side: bass and chord reeds
    kRankCount
};

inline bool IsTreble(int rank) { return rank <= kRankH; }

// The treble reed blocks cover C2..C8; keys outside are folded in by octaves.
// An accordion's treble stops near C7. Above it every rank goes on with the
// same reed design to a piano's top, so that every key to C8 sounds at its own
// pitch (the owner's rule for every Rack instrument). Tuned and played alone,
// those reeds come within 20 dB of their 400 Pa level after 2 s from about
// 100 Pa at C#7, 140 at D7 and 265-280 at C8, where C7 needs 90 (ReedProbe's
// Radiated, each treble rank): Auto's 400 Pa speaks them all, Touch's softest
// strokes (about 180 Pa) not those from G#7 up.
constexpr int kTrebleLow = 36, kTrebleHigh = 108;
constexpr int kTrebleKeys = kTrebleHigh - kTrebleLow + 1;

inline int TrebleKey(int key) {
    while (key < kTrebleLow) key += 12;
    while (key > kTrebleHigh) key -= 12;
    return key;
}

// C#7, the 4' rank's highest note an octave up: above it the rank breaks back
// to sound with the 8', as an accordion's 4' does near the keyboard's top,
// where its reeds would be the smallest and need the most pressure to start.
constexpr int kHighestReed = 97;

inline int PitchClass(int key) { return ((key % 12) + 12) % 12; }

inline double NoteHz(double note) { return 440.0 * std::pow(2.0, (note - 69.0) / 12.0); }

// Sounding note of `rank` for a treble key (already folded) or, on the bass
// side, for a pitch class 0..11.
inline double RankNote(int rank, int key, double musetteCents) {
    const int pc = PitchClass(key);
    const auto fold = [pc](int low) { return low + PitchClass(pc - low); };
    switch (rank) {
    case kRankL: return key - 12;
    case kRankM: return key;
    case kRankMMinus:
    case kRankMPlus: {
        // Tremolo reeds are tuned to a beat rate that rises with pitch, as tuners
        // do, rather than to fixed cents: musetteCents at A4, beat ~ f^0.51, as
        // a Hohner's two-reed musette beats from C4 to C6 (0.25-0.75 at 90 %,
        // 18 cents at A4; Tools/measure_musette.py). It was f^0.7, a 1959
        // Hohner's 1.7 beats a second at F3 and 7 at F6 (0.68, as a tuner
        // reported it on accordionists.info); chosen over it as measured
        // (Docs/decisions.md).
        const double f = NoteHz(key);
        const double beat440 = 440.0 * (std::pow(2.0, musetteCents / 1200.0) - 1.0);
        const double beat = beat440 * std::pow(f / 440.0, 0.51);
        const double cents = 1200.0 * std::log2(1.0 + beat / f);
        return key + (rank == kRankMPlus ? cents : -cents) / 100.0;
    }
    case kRankH: {
        // The 4' rank breaks back an octave at the top (kHighestReed).
        const int note = key + 12;
        return note > kHighestReed ? note - 12 : note;
    }
    case kRankBass16: return fold(28);   // E1..D#2
    case kRankBass8: return fold(40);    // E2..D#3
    case kRankChord8: return fold(54);   // F#3..F4
    case kRankChord4: return fold(66);   // F#4..F5
    }
    return key;
}

// Index of a reed in the tuning table: the treble key, or the bass pitch class.
inline int TuningIndex(int rank, int key) { return IsTreble(rank) ? key - kTrebleLow : PitchClass(key); }

inline std::uint32_t Hash(std::uint32_t a, std::uint32_t b) {
    std::uint32_t h = a * 0x9E3779B1u ^ (b + 0x7F4A7C15u + (a << 6) + (a >> 2));
    h ^= h >> 16; h *= 0x85EBCA6Bu; h ^= h >> 13; h *= 0xC2B2AE35u; h ^= h >> 16;
    return h;
}
inline double Unit(std::uint32_t h) { return (h & 0xFFFFFF) / double(0x1000000) * 2.0 - 1.0; } // -1..1

// Push and pull reeds are separate tongues, made and tuned separately: their
// set, loss, slot clearance, plate thickness and distal tip profiling differ a
// little, and so does their tuning. Each chamber also has its own wall/valve
// loss and tip-jet coupling. The differences are fixed per instrument, not
// drawn per performance, and scaled by Life:
//   - Life = 0.0: perfection (zero manufacturing/voicing scatter)
//   - Life = 0.5 (default): standard well-made unit in good shape (1x)
//   - Life = 1.0 (max): over-emphasized character (2x)
// At Life = 0.5 each reed sits off its nominal pitch by a standard deviation
// of 3 cents, as two played accordions' do: La Melodiosa's single reeds
// scatter 2.7-2.9 cents about a smooth line (two takes a note, 1.2-1.6 cents
// apart), its reeds two octaves apart 4.2 cents from each other (3.0 each;
// Tools/measure_tuning.py), and a Hohner's musette pairs 4.6 cents about their
// beat law (3.3 each; Tools/measure_musette.py).
inline double ImperfectionScale(double life) {
    return 2.0 * Clamp(life, 0.0, 1.0);
}

struct ReedVariation {
    double setScale, qScale, tuneCents;
    double clearanceScale, plateScale, profileScale;
    double chamberScale, ventScale;
};

inline ReedVariation Variation(int rank, int index, int side, double life = 0.5) {
    constexpr std::uint32_t kInstrument = 1u;
    constexpr double kTuneSpread = 3.0 * 1.7320508075688772;   // Unit's standard deviation is 1 / sqrt(3)
    const double s = ImperfectionScale(life);
    const std::uint32_t h = Hash(kInstrument, static_cast<std::uint32_t>(rank * 1000 + index * 2 + side));
    const std::uint32_t hc = Hash(kInstrument, static_cast<std::uint32_t>(rank * 1000 + index * 2));
    // Front vs back reed-block voicing: L and M sit on the inner blocks
    // (slightly warmer wall damping), M-, M+ and H on the outer blocks.
    const double blockLossBias = (rank == kRankL || rank == kRankM) ? 0.08 : (IsTreble(rank) ? -0.06 : 0.0);
    const double blockTipBias = (rank == kRankL || rank == kRankM) ? -0.04 : (IsTreble(rank) ? 0.04 : 0.0);
    return {
        1.0 + s * 0.06 * Unit(h),
        1.0 + s * 0.12 * Unit(Hash(h, 1)),
        s * kTuneSpread * Unit(Hash(h, 2)),
        1.0 + s * 0.12 * Unit(Hash(h, 3)),
        1.0 + s * 0.06 * Unit(Hash(h, 4)),
        1.0 + s * 0.08 * Unit(Hash(h, 5)),
        1.0 + s * (blockLossBias + 0.10 * Unit(Hash(hc, 6))),
        1.0 + s * (blockTipBias + 0.08 * Unit(Hash(hc, 7)))
    };
}

// The bare tongue's quality factor in air: a voicing estimate, inside the one
// accordion measurement found. Impulse tests on three accordion reeds in
// their plates (236, 633 and 743 Hz) gave first-mode Qs of 200-400
// (Nussbaumer and Agarwal, ICA 2016, paper 756, section 4.1); this gives 207,
// 279 and 293 there.
inline double ReedQ(double hz) { return 250.0 * std::pow(hz / 440.0, 0.3); }

// A reed sounds below its natural frequency under its air load; the tuning
// table holds natural / wanted for each reed at kTuningPressure.
constexpr double kTuningPressure = 400.0;   // Pa, a moderate playing pressure

struct CellSpec {
    ReedSpec push, pull;
    ChamberSpec chamber;
    double hz = 0.0;        // sounding pitch
    double airWeight = 0.0; // air the cell takes relative to an A4 reed (Assist)
    double voiceGain = 1.0; // per-cell acoustic radiation coupling trim
    bool bass = false;
};

// The cell for `rank` and a treble key (folded) or bass pitch class, with the
// tuning table's ratios for its push and pull reeds, to be stepped at stepRate
// or faster (MakeChamber).
inline CellSpec MakeCell(int rank, int key, double musetteCents, const float* ratios, double stepRate, double life = 0.5) {
    CellSpec spec;
    const int index = TuningIndex(rank, key);
    const double s = ImperfectionScale(life);
    spec.hz = NoteHz(RankNote(rank, key, musetteCents));
    spec.bass = !IsTreble(rank);
    // Probe at 400 Pa (Tools/ProbeReed): 0.64 L/s at C2, 0.47 at C3, 0.22 at C4,
    // 0.125 at A4, 0.054 at C6, 0.036 at C7 -- close to 0.125 L/s * 440 / f from
    // C3 up; C2 takes 20 % less.
    spec.airWeight = 440.0 / spec.hz;
    const std::uint32_t hc = Hash(1u, static_cast<std::uint32_t>(rank * 1000 + index * 2));
    spec.voiceGain = 1.0 + s * 0.07 * Unit(Hash(hc, 8));
    const double q = ReedQ(spec.hz);
    const double baseRatio = ratios ? 0.5 * (static_cast<double>(ratios[0]) + static_cast<double>(ratios[1])) : 1.0;
    for (int side = 0; side < 2; ++side) {
        const ReedVariation v = Variation(rank, index, side, life);
        const double wanted = spec.hz * std::pow(2.0, v.tuneCents / 1200.0);
        if (side == 0) {
            const ReedSpec chamberReed = MakeReed(wanted, q * v.qScale, v.setScale,
                                                  v.clearanceScale, v.plateScale, v.profileScale, s);
            spec.chamber = MakeChamber(chamberReed, wanted, spec.bass, stepRate, v.chamberScale, v.ventScale);
        }
        const double ratio = ratios ? (baseRatio + s * (static_cast<double>(ratios[side]) - baseRatio)) : 1.0;
        const double natural = wanted * ratio;
        (side == 0 ? spec.push : spec.pull) = MakeReed(natural, q * v.qScale, v.setScale,
                                                       v.clearanceScale, v.plateScale, v.profileScale, s);
    }
    return spec;
}

} // namespace respira
