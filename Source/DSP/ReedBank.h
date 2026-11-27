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
// the treble's body, bellows and bass side are made as, at which size and on
// which keys (Frame). Default is the instrument as it was before builds
// existed: every factor is 1, every rank is there, nothing sits in a tone
// chamber and nothing encloses or re-voices the treble.
//
// Three builds stand beside it since 0.8.0, the owner's choice (2026-11-20)
// of the eight there were: Irish, German and Balkan. Bandoneon, French, Jazz,
// Italian and Finnish were taken out; they had been in no released build
// (Docs/decisions.md has what each was).
//
// Tremolo is set by the Musette knob, in cents at A4, for Default; another
// build tunes its tremolo reeds at its own share of that, the share its kind
// of instrument is tuned at against the 22 cents the default is fitted to
// (ReedBank's RankNote), along its school's beat-rate slope along the
// keyboard (beatExp: f^beatExp). Each national school has its own tuning, and
// a tuner's table gives them at A4 (Liberty Bellows' tuning list, after Jack
// Brantley): dry, the 8' reeds in unison, for Balkan and classical
// instruments; 6-7 cents ("demi-swing") for Irish; 15 ("standard") for German
// and Italian; 22 ("very fast") for the old French and Italian tuning the
// knob starts at. A share of 0 is a dry build, which the knob does not move.
//
// In 0.10.0 each school also carries its own physical reed geometry
// (lengthScale, widthScale), inner-vs-outer block contrast (outerBlock,
// outerHole, outerGainDb), bellows capacity/stiffness/sealing, bass-box and
// bass-rank voicing (BassSpec), and passive treble cabinet/grille acoustic
// coloration (Voicing):
//   - Irish: a compact 2-row B/C solid-hardwood button box (Castagnari /
//     Saltarelle / Paolo Soprani tradition), LMM, 6 cents demi-swing with a
//     flatter beat exponent (f^0.38) across its shorter range, shorter narrow
//     reeds on thinner plates in shallow blocks with crisp wood-body presence
//     (900 Hz, leaner below 260 Hz), light 8-bass accompaniment (half-level
//     16' bass, open high-resonance bass box), and ~2.7 L stiffer bellows
//     (1.8x) so bisonoric push/draw reversals snap like a real 2-row box.
//   - German: a Trossingen 4-voice Hohner (Atlantic / Verdi tradition), LMMH,
//     15 cents two-reed tremolo with steeper Hohner-like beat growth (f^0.65),
//     machine-grade tolerances (higher clearance 1.35x, lower Q 0.85x, higher
//     set 1.08x), rigid stamped-aluminum grille presence (+3.5 dB near 2.8 kHz,
//     -2 dB scoop at 700 Hz), punchy dry-forward bass box (1250 Hz).
//   - Balkan: a 4-voice Serbian B-system dugmetara (Guerrini President /
//     Dallape Supermaestro / Scandalli Super VI), true 2+2 double cassotto
//     (L and M inside the deep wooden tone chamber; M+ and H outside!), dry
//     unison with hand-matched a mano reeds (narrow clearance 0.75x, high Q
//     1.18x, matched set/plate between M and M+ to eliminate slow unison-drift
//     phasing), broad long reeds (1.08x length, 1.12x width), heavy deep
//     5-voice bass side (650 Hz bass box, +2.6 dB 16' contrabass), and warm
//     chest body (+1.5 dB below 380 Hz).
//
// From 0.8.0 on, types are appended, never reordered: saved sessions recall
// them by number. The numbers of 0.2.0-0.7.4, which no released build
// carried, were Default 0, Bandoneon 1, Irish 2, German 3, French 4, Jazz 5,
// Balkan 6, Italian 7 and Finnish 8.
enum Type : int { kTypeDefault, kTypeIrish, kTypeGerman, kTypeBalkan, kTypeCount };

// A box the treble's sound leaves through, as Engine::Compartment::Set takes
// it: depth m, the cover's open fraction, the holes' effective length m, the
// cloth behind them in rayl, the energy absorbed on a round trip and where
// the depth modes take over, Hz. A depth of 0 is no box: the reeds radiate
// straight to the grille. No build has one since 0.8.0; the bandoneon's
// wooden end board was the one that did.
struct Body {
    double depth, open, neck, rayl, absorbed, modes;
};

// The instrument's own size and keyboard, estimates from typical
// instruments of each kind (makers' catalogue sizes, rounded), not
// measurements of one: the bellows' section (length and width, m), their
// depth closed (m), how far the player's lap and the bass half lie from the
// treble against Default's (reach), and what the right hand presses.
// NearField.h rings the box these make and Mechanics.h lands the keys.
//   - Default: a full-size 41-key piano accordion, 0.41 x 0.19 m.
//   - Irish: a two-row B/C box, 0.29 x 0.16 m, on buttons, bisonoric as
//     every such box is: Auto's hand turns the bellows for each note
//     (Engine::NoteOn).
//   - German: a 41-key Hohner of the Morino's size, 0.44 x 0.20 m.
//   - Balkan: a six-row button accordion (dugmetara), 0.43 x 0.21 m.
enum Action : int { kPianoKeys, kButtons };
struct Frame {
    double length, width, closed, reach;
    Action action;
    bool bisonoric = false;   // a button gives one note on the press and another on the draw
    double bellowsVolume = 6.0e-3;
    double keybedLength = 0.42;
};

// The way a B/C box's bellows go for each note, C to B. Each row is a
// diatonic one: pressed, its buttons give the major triad of its key, and
// drawn, the rest of its scale (Wikipedia, Diatonic button accordion). So
// the C row gives C, E and G on the press and D, F, A and B on the draw, and
// the B row, a semitone lower, B, D# and F# and C#, E, G# and A#: E and B
// come both ways, every other note one way only.
enum Way : int { kDraw = -1, kEitherWay = 0, kPress = 1 };
constexpr std::array<Way, 12> kBcBoxWays { kPress, kDraw, kDraw, kPress, kEitherWay, kDraw,
                                           kPress, kPress, kDraw, kDraw, kDraw, kEitherWay };
constexpr Frame kFullSize { 0.41, 0.19, 0.15, 1.0, kPianoKeys, false, 6.0e-3, 0.42 };

struct BassSpec {
    double depth, open, neck, rayl, absorbed, modes;
    double bass16Gain, bass8Gain, chordGain;
    double reedClearance;
};

struct Voicing {
    bool active;
    double lowHz, lowDb;
    double midHz, midQ, midDb;
    double highHz, highQ, highDb;
};

struct Build {
    const char* name;
    double tremolo;          // the sharp tremolo reed's (M+) detune over the Musette knob's
    double flat;             // the flat tremolo reed's (M-) detune over the sharp one's
    double beatExp;          // tremolo beat growth exponent along the keyboard (f^beatExp)
    double lengthScale;      // treble tongue length over the default's
    double widthScale;       // treble tongue width over the default's
    double plate;            // reed plate thickness (slot depth) over the default's
    double q;                // the tongue's quality factor over ReedQ's
    double set;              // the tongue's set over the default's
    double clearance;        // the gap round the tongue over the default's
    double profile;          // the tongue's tip thickness and piston area over the default's
    double block;            // the reed block's chamber depth over the default's
    double hole;             // the pallet hole's area over the default's
    double outerBlock;       // outer tremolo block chamber depth multiplier
    double outerHole;        // outer tremolo block pallet hole multiplier
    double outerGainDb;      // outer tremolo rank acoustic radiation offset, dB
    double cover;            // the treble cover's corner over the Grille knob's
    double trim;             // treble level, dB, that sets the build beside Default (TypesSitTogether)
    double bellowsStiffness; // bellows stiffness multiplier (1 / bellows air capacity)
    double bellowsLeak;      // bellows air leak multiplier (case airtightness)
    std::array<bool, 5> ranks;      // treble ranks kRankL .. kRankH the instrument carries
    std::array<bool, 5> cassotto;   // treble ranks that sound into the tone chamber
    Body body;
    Frame frame;
    BassSpec bass;
    Voicing voicing;
};

constexpr std::array<bool, 5> kAllRanks { true, true, true, true, true };
constexpr std::array<bool, 5> kNoRanks { false, false, false, false, false };
constexpr std::array<bool, 5> kLowAndSharp { true, true, false, true, false };   // L, M, M+
constexpr std::array<bool, 5> kFourVoice { true, true, false, true, true };     // L, M, M+, H
constexpr Body kOpenTreble { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
constexpr BassSpec kDefaultBass { 0.04, 0.10, 0.0081, 30.0, 0.04, 2000.0, 1.0, 1.0, 1.0, 1.0 };
constexpr Voicing kFlatVoicing { false, 300.0, 0.0, 1000.0, 1.0, 0.0, 3000.0, 1.0, 0.0 };

inline constexpr std::array<Build, kTypeCount> kBuilds { {
    { "Default", 1.0, 1.0, 0.51, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 0.0, 1.0, 0.0, 1.0, 1.0,
      kAllRanks, kNoRanks, kOpenTreble, kFullSize, kDefaultBass, kFlatVoicing },
    { "Irish", 6.0 / 22.0, 1.0, 0.38, 0.90, 0.92, 0.86, 0.95, 1.00, 1.00, 0.88, 0.72, 0.82, 0.86, 0.92, -1.5, 1.45, -2.65, 1.8, 0.9,
      kLowAndSharp, kNoRanks, kOpenTreble, { 0.29, 0.16, 0.13, 0.72, kButtons, true, 2.7e-3, 0.29 },
      { 0.024, 0.14, 0.0065, 25.0, 0.05, 2800.0, 0.45, 0.90, 0.85, 1.0 },
      { true, 260.0, -3.0, 900.0, 1.1, 3.0, 2800.0, 1.0, 0.0 } },
    { "German", 15.0 / 22.0, 1.0, 0.65, 0.94, 0.88, 0.88, 0.85, 1.08, 1.35, 0.95, 0.92, 0.95, 0.86, 0.92, -1.5, 1.65, -1.25, 1.1, 0.85,
      kFourVoice, kNoRanks, kOpenTreble, { 0.44, 0.20, 0.15, 1.06, kPianoKeys, false, 5.5e-3, 0.44 },
      { 0.032, 0.12, 0.0072, 22.0, 0.04, 2400.0, 0.95, 1.12, 1.05, 1.15 },
      { true, 260.0, 0.5, 750.0, 0.9, -3.5, 3200.0, 0.85, 4.5 } },
    { "Balkan", 0.0, 1.0, 0.51, 1.08, 1.12, 1.12, 1.18, 0.94, 0.75, 1.05, 1.20, 1.08, 0.90, 0.95, -2.5, 0.82, 1.40, 0.92, 0.55,
      kFourVoice, { true, true, false, false, false }, kOpenTreble, { 0.43, 0.21, 0.16, 1.05, kButtons, false, 6.8e-3, 0.43 },
      { 0.056, 0.08, 0.0100, 35.0, 0.06, 1600.0, 1.35, 1.10, 0.95, 0.75 },
      { true, 380.0, 1.5, 750.0, 1.2, 0.8, 3000.0, 1.0, 0.0 } },
} };

inline const Build& BuildOf(int type) { return kBuilds[static_cast<std::size_t>(type < 0 ? 0 : type >= kTypeCount ? kTypeCount - 1 : type)]; }

// Sounding note of `rank` for a treble key (already folded) or, on the bass
// side, for a pitch class 0..11.
inline double RankNote(int rank, int key, double musetteCents, double beatExp = 0.51) {
    const int pc = PitchClass(key);
    const auto fold = [pc](int low) { return low + PitchClass(pc - low); };
    switch (rank) {
    case kRankL: return key - 12;
    case kRankM: return key;
    case kRankMMinus:
    case kRankMPlus: {
        // Tremolo reeds are tuned to a beat rate that rises with pitch, as tuners
        // do, rather than to fixed cents: musetteCents at A4, beat ~ f^beatExp,
        // defaulting to f^0.51 as a Hohner's two-reed musette beats from C4 to C6
        // (0.25-0.75 at 90 %, 18 cents at A4; Tools/measure_musette.py).
        const double f = NoteHz(key);
        const double beat440 = 440.0 * (std::pow(2.0, musetteCents / 1200.0) - 1.0);
        const double beat = beat440 * std::pow(f / 440.0, beatExp);
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
// as phasing in Balkan and in two builds since taken out, and chose against by ear
// (Docs/decisions.md, 2026-11-19). Their tremolo reeds are tuned against it
// too, to a beat: one has its key's 8' reed's error and misses its own
// detune by a share of that detune, so a dry or a swing tuning stays what
// its tuner set. With an error of its own a dry pair would beat at anything
// up to 5 Hz from C4 to C6, phasing on one key and a tremolo on the next,
// and a 7 cent pair sit anywhere from dry to 20.
// The share is a played Hohner's, whose musette pairs lie 4.6 cents about
// their beat law at 18 (Tools/measure_musette.py); that it grows with the
// detune is assumed, one instrument at one tuning being all there is.
// Default keeps its reeds' own errors, bit for bit as before builds existed.
constexpr double kBeatScatter = 4.6 / 18.0;

inline double TuneCents(int rank, int index, int side, double life, int type, double detuneCents = 0.0) {
    const double own = Variation(rank, index, side, life).tuneCents;   // 3 cents at Life 0.5
    if (type == kTypeDefault || !IsTreble(rank) || rank == kRankM) return own;
    return Variation(kRankM, index, side, life).tuneCents + own / 3.0 * kBeatScatter * detuneCents;
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

// How far a tremolo reed's level sits from its key's reed at pitch, either
// way, in dB at Life 0.5. Two reeds of one level beat to silence and back on
// every key alike, as an LFO does; a real pair is never matched. Fitted: the
// model's two-reed notes beat 0.91 deep at the median with matched reeds, and
// a recorded musette C5 (Wikimedia Commons, "Accordion registers", public
// domain, two takes) beats 0.75 and 0.76 deep on its first four partials
// (band-pass 6 % either side, Hilbert envelope, 40 Hz low-pass, (p98 - p2) /
// (p98 + p2)). One recorded key: the median is fitted, the spread is assumed.
constexpr double kTremoloSpreadDb = 4.5;

inline ReedVariation BuildVariation(int rank, int index, int side, double life, int type) {
    ReedVariation v = Variation(rank, index, side, life);
    const Build& build = BuildOf(type);
    if (type != kTypeDefault && build.tremolo == 0.0 && rank == kRankMPlus) {
        // Dry unison 8' pairs on a hand-made instrument are matched in set,
        // plate, clearance, Q and profile so both reeds bend identically with
        // bellows pressure and do not drift into slow phaser sweeps on held notes.
        const ReedVariation ref = Variation(kRankM, index, side, life);
        v.setScale = ref.setScale;
        v.plateScale = ref.plateScale;
        v.clearanceScale = 0.5 * (v.clearanceScale + ref.clearanceScale);
        v.qScale = 0.5 * (v.qScale + ref.qScale);
        v.profileScale = 0.5 * (v.profileScale + ref.profileScale);
        v.chamberScale = 0.5 * (v.chamberScale + ref.chamberScale);
    }
    return v;
}

inline ReedSpec BuildReedSpec(double f0, double nominalHz, int rank, const ReedVariation& v, double imperfection, int type) {
    const Build& build = BuildOf(type);
    const bool treble = IsTreble(rank);
    const double q = ReedQ(nominalHz) * (treble ? build.q : 1.0) * v.qScale;
    const double setScale = v.setScale * (treble ? build.set : 1.0);
    const double clearanceScale = v.clearanceScale * (treble ? build.clearance : build.bass.reedClearance);
    const double plateScale = v.plateScale * (treble ? build.plate : 1.0);
    const double profileScale = v.profileScale * (treble ? build.profile : 1.0);
    const double lengthScale = treble ? build.lengthScale : 1.0;
    const double widthScale = treble ? build.widthScale : 1.0;
    return MakeReed(f0, q, setScale, clearanceScale, plateScale, profileScale, imperfection, lengthScale, widthScale);
}

// The cell for `rank` and a treble key (folded) or bass pitch class, with the
// tuning table's ratios for its push and pull reeds, to be stepped at stepRate
// or faster (MakeChamber).
inline CellSpec MakeCell(int rank, int key, double musetteCents, const float* ratios, double stepRate, double life = 0.5,
                         int type = kTypeDefault) {
    CellSpec spec;
    const Build& build = BuildOf(type);
    const int index = TuningIndex(rank, key);
    const double s = ImperfectionScale(life);
    const double note = RankNote(rank, key, musetteCents * build.tremolo * (rank == kRankMMinus ? build.flat : 1.0),
                                 build.beatExp);
    spec.hz = NoteHz(note);
    const bool tremolo = rank == kRankMMinus || rank == kRankMPlus;
    const double detune = tremolo ? 100.0 * std::fabs(note - key) : 0.0;   // cents
    spec.bass = !IsTreble(rank);
    spec.cassotto = !spec.bass && build.cassotto[static_cast<std::size_t>(rank)];
    // Probe at 400 Pa (Tools/ProbeReed): 0.64 L/s at C2, 0.47 at C3, 0.22 at C4,
    // 0.125 at A4, 0.054 at C6, 0.035 at C7 -- close to 0.125 L/s * 440 / f from
    // C3 up; C2 takes 20 % less.
    spec.airWeight = 440.0 / spec.hz;
    const std::uint32_t hc = Hash(1u, static_cast<std::uint32_t>(rank * 1000 + index * 2));
    spec.voiceGain = 1.0 + s * 0.07 * Unit(Hash(hc, 8));
    if (spec.bass) {
        const double rankGain = rank == kRankBass16 ? build.bass.bass16Gain
                              : rank == kRankBass8 ? build.bass.bass8Gain
                              : build.bass.chordGain;
        spec.voiceGain *= rankGain;
    }
    if (tremolo) {
        if (type == kTypeDefault)
            spec.voiceGain *= std::pow(10.0, s * kTremoloSpreadDb * Unit(Hash(hc, 9)) / 20.0);
        else if (build.tremolo == 0.0)
            spec.voiceGain *= std::pow(10.0, build.outerGainDb / 20.0);
        else
            spec.voiceGain *= std::pow(10.0, (build.outerGainDb + s * 3.0 * Unit(Hash(hc, 9))) / 20.0);
    }
    const bool outerTremolo = type != kTypeDefault && tremolo;
    const double blockScale = spec.bass ? 1.0 : build.block * (outerTremolo ? build.outerBlock : 1.0);
    const double holeScale = spec.bass ? 1.0 : build.hole * (outerTremolo ? build.outerHole : 1.0);
    const double baseRatio = ratios ? 0.5 * (static_cast<double>(ratios[0]) + static_cast<double>(ratios[1])) : 1.0;
    for (int side = 0; side < 2; ++side) {
        const ReedVariation v = BuildVariation(rank, index, side, life, type);
        const double wanted = spec.hz * std::pow(2.0, TuneCents(rank, index, side, life, type, detune) / 1200.0);
        if (side == 0) {
            const ReedSpec chamberReed = BuildReedSpec(wanted, spec.hz, rank, v, s, type);
            spec.chamber = MakeChamber(chamberReed, wanted, spec.bass, stepRate, v.chamberScale, v.ventScale,
                                       blockScale, holeScale);
        }
        const double ratio = ratios ? (baseRatio + s * (static_cast<double>(ratios[side]) - baseRatio)) : 1.0;
        const double natural = wanted * ratio;
        (side == 0 ? spec.push : spec.pull) = BuildReedSpec(natural, spec.hz, rank, v, s, type);
    }
    return spec;
}

} // namespace respira
