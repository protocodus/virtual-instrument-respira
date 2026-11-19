#pragma once

// The reed blocks of the instrument: which reeds each register rank sounds
// for a key, how each push and pull reed differs from its neighbours, and the
// reed and chamber a sounding cell is built from. Shared by the engine and by
// Tools/GenerateReedTuning.cpp, which tunes every reed offline.

#include "ReedModel.h"

#include <array>
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
// 80 Pa at C#7, 145 at G#7, 175 at A#7, 200 at B7 and 220 at C8, where C7
// needs 70 (ReedProbe's Radiated at the tuning rate; the 8' ranks and the 4',
// which sounds with them there; the 16' rank's reeds an octave down need
// 40-70): Auto's 400 Pa speaks them all, Touch's softest strokes (about
// 180 Pa) not B7 and C8.
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

// The instrument's build, the Type control: which kind of accordion the
// reeds, their plates, their chambers, the reed sets the treble carries and
// the treble's body are made as. The keys, bellows, the bass side and the
// microphones stay the same. Default is the instrument as it was before
// builds existed and stays bit for bit so: every factor is 1, every rank is
// there, nothing sits in a tone chamber and nothing encloses the treble.
//
// Tremolo is set by the Musette knob, in cents at A4, for Default; another
// build tunes its tremolo reeds at its own share of that, the share its kind
// of instrument is tuned at against the 22 cents the default is fitted to
// (ReedBank's RankNote). Tuners' published figures, all approximate (Liberty
// Bellows' tuning list; accordionists.info's tuning threads): swing or
// concert, a jazz box, 2-4 cents; modern Irish 5-7; German 14-15; Italian
// about 15; French 18-22 and older French musette wetter still. Balkan,
// Italian and Finnish are read off one comparison the owner shared
// (2026-11-18): a digital accordion's demo of its French, Balkan, German,
// Italian, Finnish and Irish sets, about 11-17 s of busy playing each. Their
// tremolo is the median beat of its sustained notes, from the side peaks
// round the strongest partial (0.6 s windows) and from the partial's envelope
// (0.8 s), taken to A4 by RankNote's f^0.51 law: Balkan 9-18 cents (about
// 15), Italian 10-19 (about 16; one held E4 beat 4.2 Hz, 19), Finnish 18-21
// (about 19); German read 11-16 on the same ruler. Above 2 kHz the Balkan
// and Finnish sets sit about 8 dB under the French, German and Irish ones,
// the Italian about 3 dB under.
//
// The cover: how much of the reeds' upper harmonics the treble cover lets
// out, as a factor on the Grille knob's corner (RespiraControls.h, a voiced
// one-pole, not a model of a grille). Read off the same comparison, each set
// against its German one, the kind Default's reeds were fitted to, after
// taking out what the sets' different pitches do in this model (the same
// melody rendered at each set's median note): from 2 kHz up the French set
// is 6-9 dB brighter, the Irish 4-7, the Italian 0-5 (2-4, 4-8 and 8-16 kHz
// against 250-1000 Hz). Corners 2.2, 1.7 and 1.4 times the knob's give 4-5.5,
// 2-5 and 2.5-3 dB on one phrase. They are a fit to one sampled instrument's
// sets through 64 kbit/s audio, good to perhaps 3 dB, not a measured grille
// (README, Known gaps); no maker's source says why those boxes are
// brighter. The trim is the level that takes back, so that a type changes
// the instrument and not its loudness (TypesSitTogether).
//
// Besides the cover, only the ranks, the tremolo, the cassotto and the
// bandoneon's end board move the steady sound by more than about 2 dB in any
// octave: in this
// model the reed, plate, block and hole factors, pushed 40-60 % either way
// one at a time, move Paris Musette's octave bands by 0.1-2.8 dB, mostly in
// how the reeds speak. They are kept where the kind of instrument is known
// to be built so.
//
// The rest are estimates of each kind's construction, from makers' and
// players' descriptions, not measurements of an instrument (README, Known
// gaps). A reed's size for its pitch is not among them: MakeReed fixes it,
// and a tongue 1.3 times as long at the same pitch takes 5 dB off 2-4 kHz
// and 8-16 kHz (Paris Musette's phrase), but no maker or study gives the
// tongues' sizes by kind of instrument, so every build keeps Default's:
//   - Bandoneon: two reeds an octave apart on every key (16' and 8', the L
//     and M ranks) and no others, so every register sounds that pair; the
//     reeds riveted to long zinc plates, thicker and much heavier than an
//     accordion's aluminium, which players and restorers credit with the
//     deep sound (bando-bando.de, "ELA Zink"): a plate that moves less takes
//     less of the tongue's energy, so the tongues' loss is 1/1.3 of the
//     default's and the slot 1.25 times as deep, the tongues thicker towards
//     the tip (1.2). No reed blocks: the plates sit over short cells in the
//     reed board (half the chamber depth) whose valves open wide (1.5 times
//     the hole), and the sound leaves through the fretwork holes of the
//     wooden end board: a box like the accordion's bass end (RespiraEngine.cpp,
//     the bass box), 40 mm deep and a tenth open through 8 mm of wood, which
//     rings about 1 kHz and rolls off above it.
//   - Irish: a B/C button box's three voices as Irish makers build them,
//     LMM: the 16', the 8' at pitch and one 8' tuned sharp (M+), no flat 8'
//     and no 4' (McNeela's guide to Irish diatonic accordions), on small
//     aluminium plates (0.8 of the slot) with tongues set low
//     (0.85) and thin at the tip (0.9) for quick ornaments, in shallow blocks
//     (0.7) behind small pallets (0.8). Its treble grille is no resonator, as
//     the accordion's is not (RespiraEngine.cpp, the bass box's note).
//   - German: a Hohner's tremolo; the default reeds were themselves fitted to
//     a Hohner and an Italian instrument (RespiraControls.h, Grille), so
//     nothing else changes.
//   - French: the musette's three middle reeds and no others, tuned as
//     French musette is, wider on the sharp side: the flat reed a third of the
//     sharp one's detune (tuners' -5/0/+15 and -6/0/+15 cents,
//     accordionists.info, and Liberty Bellows' 435/440/445 at the wide end),
//     on the lighter plates (0.9 of the slot) and thinner tips (0.9) of a
//     bright, open-voiced box.
//   - Jazz: dry, hand-made reeds: their tongues lose less (Q 1.15 times the
//     default's) and fit their slots closer (0.8 of the clearance), with the
//     16' and 8' ranks in a cassotto, the tone chamber whose bend attenuates
//     the upper harmonics and reinforces the lows (Engine's cassotto,
//     RespiraEngine.cpp).
//   - Balkan: a four-voice LMMH box with a double cassotto (Bugari's Balkan
//     DM), hand-made reeds (as Jazz's) in deeper blocks (1.15), the 16' and
//     both 8's in the cassotto, the 4' outside it.
//   - Italian: Castelfidardo hand-made reeds (as Jazz's) in deeper blocks
//     (1.15), open, no cassotto, four voices LMMH with the tremolo on one side
//     only: the second 8' tuned sharp, none flat (four Italian boxes measured
//     0/+20 three times and -5/+15 once, accordionists.info).
//   - Finnish: the five-row C-system box Finnish players use, an Italian
//     cassotto instrument (Bugari Finlandia Cassotto): the wet tremolo, the
//     16' and the three 8's in the cassotto.
//   Cassotto boxes are usually described with only the 16' and the 8' at
//   pitch inside (Wikipedia, accordion reed ranks; a Bugari owner on
//   accordionists.info). With the tremolo 8's outside, Balkan and Finnish sit
//   only 1.4 and 1.0 dB under Default at 2-4 kHz (Paris Musette's phrase,
//   level-matched), where the owner's comparison has them some 8 dB under the
//   French, German and Irish sets; so they keep their tremolo reeds inside,
//   the comparison's darkness standing until a real Balkan or Finnish box is
//   measured (README, Known gaps).
// Types are appended, never reordered: saved sessions recall them by number.
enum Type : int { kTypeDefault, kTypeBandoneon, kTypeIrish, kTypeGerman, kTypeFrench, kTypeJazz,
                  kTypeBalkan, kTypeItalian, kTypeFinnish, kTypeCount };

// A box the treble's sound leaves through, as Engine::Compartment::Set takes
// it: depth m, the cover's open fraction, the holes' effective length m, the
// cloth behind them in rayl, the energy absorbed on a round trip and where
// the depth modes take over, Hz. A depth of 0 is no box: the reeds radiate
// straight to the grille.
struct Body {
    double depth, open, neck, rayl, absorbed, modes;
};

struct Build {
    const char* name;
    double tremolo;    // the sharp tremolo reed's (M+) detune over the Musette knob's
    double flat;       // the flat tremolo reed's (M-) detune over the sharp one's
    double plate;      // reed plate thickness (slot depth) over the default's
    double q;          // the tongue's quality factor over ReedQ's
    double set;        // the tongue's set over the default's
    double clearance;  // the gap round the tongue over the default's
    double profile;    // the tongue's tip thickness and piston area over the default's
    double block;      // the reed block's chamber depth over the default's
    double hole;       // the pallet hole's area over the default's
    double cover;      // the treble cover's corner over the Grille knob's
    double trim;       // treble level, dB, that sets the build beside Default (TypesSitTogether)
    std::array<bool, 5> ranks;      // treble ranks kRankL .. kRankH the instrument carries
    std::array<bool, 5> cassotto;   // treble ranks that sound into the tone chamber
    Body body;
};

constexpr std::array<bool, 5> kAllRanks { true, true, true, true, true };
constexpr std::array<bool, 5> kNoRanks { false, false, false, false, false };
constexpr std::array<bool, 5> kMiddleRanks { false, true, true, true, false };   // M, M-, M+
constexpr std::array<bool, 5> kLowAndSharp { true, true, false, true, false };   // L, M, M+
constexpr std::array<bool, 5> kFourVoice { true, true, false, true, true };     // L, M, M+, H
constexpr std::array<bool, 5> kLowAndStraight { true, true, false, false, false }; // L, M
constexpr Body kOpenTreble { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };

inline constexpr std::array<Build, kTypeCount> kBuilds { {
    { "Default", 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 0.0, kAllRanks, kNoRanks, kOpenTreble },
    { "Bandoneon", 0.05, 1.0, 1.25, 1.3, 1.0, 1.0, 1.2, 0.5, 1.5, 1.0, 0.0, { true, true, false, false, false }, kNoRanks,
      { 0.04, 0.10, 0.0081, 0.0, 0.04, 2000.0 } },
    { "Irish", 7.0 / 22.0, 1.0, 0.8, 1.0, 0.85, 1.0, 0.9, 0.7, 0.8, 1.7, -0.6, kLowAndSharp, kNoRanks, kOpenTreble },
    { "German", 14.0 / 22.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 0.0, kAllRanks, kNoRanks, kOpenTreble },
    { "French", 28.0 / 22.0, 1.0 / 3.0, 0.9, 1.0, 1.0, 1.0, 0.9, 1.0, 1.0, 2.2, -1.7, kMiddleRanks, kNoRanks, kOpenTreble },
    { "Jazz", 3.0 / 22.0, 1.0, 1.0, 1.15, 1.0, 0.8, 1.0, 1.0, 1.0, 1.0, 0.0, kAllRanks, kLowAndStraight, kOpenTreble },
    { "Balkan", 15.0 / 22.0, 1.0, 1.0, 1.15, 1.0, 0.8, 1.0, 1.15, 1.0, 1.0, 0.0, kFourVoice, { true, true, false, true, false }, kOpenTreble },
    { "Italian", 16.0 / 22.0, 1.0, 1.0, 1.15, 1.0, 0.8, 1.0, 1.15, 1.0, 1.4, -1.4, kFourVoice, kNoRanks, kOpenTreble },
    { "Finnish", 19.0 / 22.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 0.0, kAllRanks, { true, true, true, true, false }, kOpenTreble },
} };

inline const Build& BuildOf(int type) { return kBuilds[static_cast<std::size_t>(type < 0 ? 0 : type >= kTypeCount ? kTypeCount - 1 : type)]; }

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

// A reed's pitch error in cents. The national builds' 16' and 4' reeds are
// tuned dry against their key's 8' reed, as a tuner sets octave reeds, so
// they share its error: with errors of their own (La Melodiosa's, above) the
// octaves beat deeply at 0.3-1.3 Hz, which the owner heard
// as phasing in Bandoneon, Jazz and Balkan and chose against by ear
// (Docs/decisions.md, 2026-11-19). Default keeps its reeds' own errors, bit
// for bit as before builds existed.
inline double TuneCents(int rank, int index, int side, double life, int type) {
    const bool dryOctave = type != kTypeDefault && (rank == kRankL || rank == kRankH);
    return Variation(dryOctave ? kRankM : rank, index, side, life).tuneCents;
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
    bool cassotto = false;   // radiates into the treble's tone chamber (Build)
    ChamberSpec chamber;
    double hz = 0.0;        // sounding pitch
    double airWeight = 0.0; // air the cell takes relative to an A4 reed (Assist)
    double voiceGain = 1.0; // per-cell acoustic radiation coupling trim
    bool bass = false;
};

// The cell for `rank` and a treble key (folded) or bass pitch class, with the
// tuning table's ratios for its push and pull reeds, to be stepped at stepRate
// or faster (MakeChamber).
inline CellSpec MakeCell(int rank, int key, double musetteCents, const float* ratios, double stepRate, double life = 0.5,
                         int type = kTypeDefault) {
    CellSpec spec;
    const Build& build = BuildOf(type);
    const int index = TuningIndex(rank, key);
    const double s = ImperfectionScale(life);
    spec.hz = NoteHz(RankNote(rank, key, musetteCents * build.tremolo * (rank == kRankMMinus ? build.flat : 1.0)));
    spec.bass = !IsTreble(rank);
    spec.cassotto = !spec.bass && build.cassotto[static_cast<std::size_t>(rank)];
    // Probe at 400 Pa (Tools/ProbeReed): 0.64 L/s at C2, 0.47 at C3, 0.22 at C4,
    // 0.125 at A4, 0.054 at C6, 0.035 at C7 -- close to 0.125 L/s * 440 / f from
    // C3 up; C2 takes 20 % less.
    spec.airWeight = 440.0 / spec.hz;
    const std::uint32_t hc = Hash(1u, static_cast<std::uint32_t>(rank * 1000 + index * 2));
    spec.voiceGain = 1.0 + s * 0.07 * Unit(Hash(hc, 8));
    const double q = ReedQ(spec.hz) * build.q;
    const double baseRatio = ratios ? 0.5 * (static_cast<double>(ratios[0]) + static_cast<double>(ratios[1])) : 1.0;
    for (int side = 0; side < 2; ++side) {
        const ReedVariation v = Variation(rank, index, side, life);
        const double wanted = spec.hz * std::pow(2.0, TuneCents(rank, index, side, life, type) / 1200.0);
        if (side == 0) {
            const ReedSpec chamberReed = MakeReed(wanted, q * v.qScale, v.setScale * build.set, v.clearanceScale * build.clearance,
                                                  v.plateScale * build.plate, v.profileScale * build.profile, s);
            spec.chamber = MakeChamber(chamberReed, wanted, spec.bass, stepRate, v.chamberScale, v.ventScale,
                                       spec.bass ? 1.0 : build.block, spec.bass ? 1.0 : build.hole);
        }
        const double ratio = ratios ? (baseRatio + s * (static_cast<double>(ratios[side]) - baseRatio)) : 1.0;
        const double natural = wanted * ratio;
        (side == 0 ? spec.push : spec.pull) = MakeReed(natural, q * v.qScale, v.setScale * build.set, v.clearanceScale * build.clearance,
                                                       v.plateScale * build.plate, v.profileScale * build.profile, s);
    }
    return spec;
}

} // namespace respira
