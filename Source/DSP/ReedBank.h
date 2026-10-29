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

// The treble reed blocks cover C2..C7; keys outside are folded in by octaves.
constexpr int kTrebleLow = 36, kTrebleHigh = 96;
constexpr int kTrebleKeys = kTrebleHigh - kTrebleLow + 1;

inline int TrebleKey(int key) {
    while (key < kTrebleLow) key += 12;
    while (key > kTrebleHigh) key -= 12;
    return key;
}

// C#7, the 4' rank's highest reed: above it the model's reeds need more than
// a moderate pressure to speak (D7-F7 from about 400 Pa, C8 only at 1.5 kPa).
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
        // do, rather than to fixed cents: musetteCents at A4, beat ~ f^0.7. A
        // Hohner's two-reed musette beats as f^0.51 (0.25-0.75 at 90 %), 18
        // cents at A4 (Tools/measure_musette.py).
        const double f = NoteHz(key);
        const double beat440 = 440.0 * (std::pow(2.0, musetteCents / 1200.0) - 1.0);
        const double beat = beat440 * std::pow(f / 440.0, 0.7);
        const double cents = 1200.0 * std::log2(1.0 + beat / f);
        return key + (rank == kRankMPlus ? cents : -cents) / 100.0;
    }
    case kRankH: {
        // The 4' rank breaks back an octave at the top, where its reeds would be
        // too small to speak: no reed sounds above C#7.
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
// set and loss differ a little, and each is tuned to within a cent. The
// differences are fixed per instrument, not drawn per performance.
struct ReedVariation {
    double setScale, qScale, tuneCents;
};

inline ReedVariation Variation(int rank, int index, int side) {
    constexpr std::uint32_t kInstrument = 1u;
    const std::uint32_t h = Hash(kInstrument, static_cast<std::uint32_t>(rank * 1000 + index * 2 + side));
    return { 1.0 + 0.10 * Unit(h), 1.0 + 0.20 * Unit(Hash(h, 1)), 0.8 * Unit(Hash(h, 2)) };
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
    bool bass = false;
};

// The cell for `rank` and a treble key (folded) or bass pitch class, with the
// tuning table's ratios for its push and pull reeds, to be stepped at stepRate
// or faster (MakeChamber).
inline CellSpec MakeCell(int rank, int key, double musetteCents, const float* ratios, double stepRate) {
    CellSpec spec;
    const int index = TuningIndex(rank, key);
    spec.hz = NoteHz(RankNote(rank, key, musetteCents));
    spec.bass = !IsTreble(rank);
    // Probe at 400 Pa (Tools/ProbeReed): 0.65 L/s at C2, 0.44 at C3, 0.21 at C4,
    // 0.11 at A4, 0.05 at C6, 0.03 at C7 -- close to 0.12 L/s * 440 / f from C3
    // up; C2 takes 20 % less.
    spec.airWeight = 440.0 / spec.hz;
    const double q = ReedQ(spec.hz);
    for (int side = 0; side < 2; ++side) {
        const ReedVariation v = Variation(rank, index, side);
        const double wanted = spec.hz * std::pow(2.0, v.tuneCents / 1200.0);
        if (side == 0) spec.chamber = MakeChamber(MakeReed(wanted, q * v.qScale, v.setScale), wanted, spec.bass, stepRate);
        const double natural = wanted * (ratios ? ratios[side] : 1.0);
        (side == 0 ? spec.push : spec.pull) = MakeReed(natural, q * v.qScale, v.setScale);
    }
    return spec;
}

} // namespace respira
