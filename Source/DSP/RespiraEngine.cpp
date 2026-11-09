#include "RespiraEngine.h"

#include "ReedTuning.h"

#include <cmath>
#include <initializer_list>

namespace respira {

namespace {

// Pa per m^3 of air the hand moves but the reeds do not take. The air in a
// ~6 L bellows alone is 2.4e7; the folds and the player's arm give a few
// times more, which keeps the pressure from collapsing whenever a chord opens.
// As the bellows open from their closed frame and fold dead space, the
// enclosed air volume grows, so the stiffness falls around kBellowsStiffness
// at mid-extension (BellowsStiffness, Engine::Extension).
constexpr double kBellowsStiffness = 4.0e6;
constexpr double kBellowsVolumeSlope = 0.30;
// A ~6 L bellows' air alone has bulk stiffness 2.4e7 Pa/m^3; the remaining
// compliance comes from the cardboard/leather folds bowing and the player's
// arm yielding. When the hand moves against closed or throttled pallets,
// kFoldDistensionShare of the hand's stroke distends or compresses the fold
// walls themselves, rubbing their cloth and leather hinges even before air
// leaves through the reeds.
constexpr double kFoldDistensionShare = 0.35;
constexpr double kPalletCoupling = 0.25;   // shared outer end-correction share between sibling ranks under one pallet pad
constexpr double kPalletChiff = 1.5;       // extra orifice-jet turbulence across the unseating pallet rim at mid-lift
constexpr double kPalletVentChiff = 0.35;  // pallet-hole orifice-jet fluctuation share while the pad lifts or seats
constexpr double kEddyForceShare = 0.65;   // coherent gap-eddy pressure fluctuation on the speaking reed tongue

// At `extension` of the bellows' travel, 0 closed .. 1 fully open.
double BellowsStiffness(double extension) {
    return kBellowsStiffness / (1.0 + kBellowsVolumeSlope * (extension - 0.5));
}
constexpr double kBellowsLeak = 2.0e-8;     // m^3/s per Pa: 0.02 L/s at 1 kPa
// The air button lifts a pad off a 25 mm hole in the bass end. Air leaves
// through it as through the reed gaps (Bernoulli with Cd = 0.65, plus the
// laminar term): 4.0 L/s at 100 Pa, so an ordinary stroke stays below the
// reeds' speaking pressure.
constexpr double kAirValveArea = 4.9e-4;    // m^2
// The jet through the valve is turbulent. Eddies about the pad's lift, ~3 mm,
// are roughly independent around the hole's 79 mm edge, each fluctuating by
// ~10 %: the valve flow fluctuates by 0.1 / sqrt(26) of itself, its spectrum
// falling above the eddies' rate U / (2 pi l). It radiates as the reeds do,
// as the rate of change of that flow: the hiss of a working air button.
constexpr double kValveEddy = 3.0e-3;       // m
constexpr double kValveBreath = 0.0196;
constexpr double kHandCatchUp = 25.0;       // 1/s: how fast the hand closes on the wheel position
// Auto's hand holds 400 Pa with the wheel at rest, the pressure the demo
// phrases' ordinary strokes are fitted to (Tools/write_preset_midi.py), and
// the wheel scales whatever pressure the velocity asks, down to
// kAutoSoftShare of it at the top, so it softens and swells a held note by
// the same share at every velocity: the wheel is Auto's volume (owner,
// 2026-11-04). 0.1.12 to 0.1.14 swelled notes struck under 64 instead, by
// only 3-8 dB, and 0.1.15 brought every velocity to one 60 Pa at the top.
// Voiced, not measured.
constexpr double kAutoPressure = 400.0;     // Pa
constexpr double kAutoSoftShare = 0.15;     // of the velocity's pressure with the wheel at the top
// An otherwise undefined controller sets who moves the bellows on the engine's
// own MIDI: below 64 the wheel, from 64 Auto's hand. The demo phrases send it
// at their start, since they play the bellows on the wheel.
constexpr int kHandModeCc = 85;
// With Auto on, these treble-channel keys latch which way its hand moves the
// bellows, the player's own choice of turn (owner, 2026-11-03). They sit an
// octave under the bass buttons, where the split keyboard only repeats them;
// with Auto off they play as keys.
constexpr int kPullKey = 12, kPushKey = 13;   // C0, C#0 (C4 = 60, as Respira names notes)
// A full-size accordion's bellows hold about 6 L between closed and open, an
// estimate: with its 0.41 x 0.19 m section, 77 mm of travel at the bass end.
constexpr double kBellowsVolume = 6.0e-3;   // m^3
constexpr double kAutoFastest = 4.0e-3;     // m^3/s: the most air the hand moves, opening or closing
// Auto's player is a person, whose arm never presses quite evenly. In a
// steady contraction the force wanders by a few percent of itself, most of
// that power below 2-3 Hz (force steadiness, e.g. Enoka and Farina, J
// Physiol 599, 2021: a coefficient of variation of 1-5 %, larger for a moving
// limb than a held one), and no two strokes are pressed alike. The pressure
// the player wants wanders by kHandWander of itself through two one-poles at
// kHandWanderHz, and each stroke takes its own level, kHandStroke of it at one
// standard deviation. Voiced within those ranges, not measured on players.
constexpr double kHandWander = 0.08;
constexpr double kHandWanderHz = 1.5;
constexpr double kHandStroke = 0.06;
// Auto's player closes on the pressure they want in 0.1 s, a hand's reaction,
// whatever their arm's Momentum: they lead the arm (Hand::Tick), so its lag
// is not in their loop.
constexpr double kReaction = 0.1;
// The radiated sound is the pallet flow's rate of change as from a small
// source 1 m away; 0.67 Pa there (90.5 dB SPL) is digital full scale, so the
// notes of the demo phrases read -9 on Reason's VU meter (the louder channel's
// RMS over 300 ms, +12 dB) at their median (Tests/EngineTests.cpp,
// TypicalLevel). It was 1 Pa until the Grille's default fell from 9 kHz to
// 1 kHz, which took 2.5 dB off them. A bass, a chord and a triad at the arm's full pressure are
// some 23 dB louder; the output stage below keeps them out of the red. The
// factory presets set their Volume so each sits within 1 dB of Init on a
// moderate stroke (PresetsSitTogether).
constexpr double kPascalToFullScale = 1.5;
// Each register's trim on the treble side, so switching one does not jump the
// level: on Init's controls and the reference stroke (Tests/EngineTests.cpp,
// PresetsSitTogether), a C3, a C4-E4-G4 triad, a G4, a C5 and a C6 average the
// Musette register's RMS. More reeds are louder on a real accordion too (the
// Master register 1.9 dB, one reed 3.5-3.9 dB); the owner asked for registers
// that change the colour, not the level (RegistersSitTogether).
constexpr std::array<double, 8> kRegisterTrimDb { { 2.34, 2.98, 1.08, 0.0, 0.48, 0.78, 1.16, -1.90 } };
// The output stage, which the owner asked for: no red on Reason's meters
// however hard the bellows are pushed (OutputStage.h, shared by the newer
// instruments). Its compressor follows the louder channel's power over 20 ms
// and above -15 dBFS (-3 VU) brings it down 20:1 over a 4 dB knee, so
// playing below about -5 VU passes untouched: chosen by ear over 10:1 from
// -9 VU, which flattened a swell (Docs/decisions.md). It takes hold at once
// and lets go over 300 ms, slower than a musette's beating, so the beats keep
// their depth. The plucked instruments' guard reads the meter's 300 ms over a knee
// centred at -15 dBFS and 16 dB wide: the bellows reach the arm's full
// pressure within tens of milliseconds and 23 dB above ordinary playing,
// where a chord struck at velocity 127 is 7-9 dB, so they need a faster,
// firmer guard. Its 2 ms look-ahead limiter holds what gets past at -1.5 dBFS.
constexpr protocodus::CompressorSettings kBellowsGuard { -18.0, 6.0, 5.0, 0.02, 0.0, 0.3 };
// The bass pallets open into the bass box, the space under the bass end cover
// round the bass mechanism, which vents through the cover's holes: a
// perforated-panel Helmholtz resonator (Compartment::Set). Typical values: 40
// mm deep, 10 % open, holes 6 mm across in 3 mm wood (8.1 mm effective, with
// both end corrections), cloth of 30 rayl: 960 Hz at Q 1.3, rising 3 dB before
// the roll-off. The plausible spread (30-50 mm, 5-20 %, 5-10 mm) is 0.55-2.0
// kHz. Lumped, the box must be shallow against the wavelength: at resonance k
// depth = sqrt(open depth / l') = 0.70. As a plane-wave layer, the flow out
// over the flow in is 1 / (cos kd - k (l' / open) sin kd + j (1 + cloth / rho
// c) sin kd); the lumped box is within 2 dB of it up to 2 kHz. Above that the
// layer rings at its depth modes, from c / 2 depth (4.3 kHz), where the lumped
// box alone fell 12 dB an octave: 9, 17 and 23 dB under even a box that
// absorbs every reflection at 2-4, 4-8 and 8-16 kHz. Over the box's irregular
// depth (30-50 mm) the modes average out to a leak: the holes pass |t|^2 =
// (2 rho c / w m)^2 of what reaches them and the rest goes round again, less
// the fraction a of its energy the box absorbs on each round trip, so the leak
// is |t|^2 / (a + |t|^2), flat up to w = 2 c open / (l' sqrt a) and 6 dB an
// octave down above. Joined to the lumped box through two high-passes at 2 kHz, the sum's
// octave bands from 1 to 16 kHz come within 2.6 dB (RMS 1.6) of the exact
// layer's averaged over 30-50 mm, for a from 2 to 10 %. Two sample libraries'
// bass buttons (La Melodiosa by Petri Pohjanmies, CC BY; a Hohner by Marek
// Kintosh, Pianobook) fit a = 4-6 % best: at 4 % Respira's octave bands from
// 2 to 16 kHz against 0.5-1 kHz sit within 2.3 dB of both, but for the
// Hohner's 8-16 kHz (7 dB), and 0.2 dB over them on average, where the lumped
// box read 11-44 dB under them (Tools/compare_accordion_tone.py,
// Docs/decisions.md). Bare wood absorbs 1-3 % at
// 2-4 kHz (larch, J. Wood Sci. 2017, doi:10.1007/s10086-017-1662-z); the felt
// and leather of the mechanism would add to it. The treble
// compartment under the grille is no resonator: 35 mm deep under a grille 30 %
// open with 3.6 mm slots, its lumped resonance would fall at k depth = 1.7,
// and as a layer, averaged over its plausible depths (25-45 mm), it passes the
// reeds within 3 dB up to 16 kHz.
constexpr double kBassBoxDepth = 0.04, kBassBoxOpen = 0.10, kBassHoleLength = 0.0081, kClothRayl = 30.0;
constexpr double kBassBoxAbsorbed = 0.04, kBassBoxModes = 2000.0;
constexpr double kOpenSeconds = 0.012;     // register slide / air button; key strokes: Touch.h
constexpr double kCloseSeconds = 0.008;
constexpr double kQuietSeconds = 0.02;      // a closed chamber is let go after this
// Key travel at full channel pressure: the pallet opening (raised cosine of
// the travel) at which the model's C4 falls the 5.1 Hz per kPa of bellows
// pressure Okada measured with the pallet lifted 1 mm (Adachi et al.,
// "Aerodynamic mechanism of accordion pitch bending", Forum Acusticum 2026),
// 0.32 of the open area. Okada's own model fits that pallet with 150/35 the
// open one's loss coefficient, 0.48 of its area; a fifth of a 5 mm lift, 0.2,
// silenced half-pressed low keys at mezzo forte, where Elejalde-Garcia et al.
// (2021) measured them bent 11 dB under open.
constexpr double kHalfPressed = 0.38278;    // acos(1 - 2 * 0.32) / pi
constexpr double kTurbulence = 2.0e-3;      // gap turbulence force per Pa of bellows pressure and m^2
constexpr double kNoiseRate = 192000.0;     // internal rate the turbulence levels are set at
// The gap jet's flow fluctuation (ReedModel.h, breath) has the spectrum of its
// eddies, the size of the gap near the tongue's tip as it opens and closes:
// flat up to their rate U / (2 pi l), with U the jet's speed sqrt(2 p / rho),
// and falling above it, as the air valve's does; at 500 Pa near 6 kHz for a
// C4 tongue 1 mm open and 18 kHz for an A5 one 0.3 mm open. White noise per
// internal step would spread most of the estimated fluctuation above 24 kHz,
// where no one hears it.

double DbToGain(double db) { return std::pow(10.0, db / 20.0); }

// The bellows' folds' levels against their estimate (Mechanics.h, Pleat and
// Rustle), voiced by ear. Literal gains, not DbToGain: the Rack toolchain
// refuses any global it has to initialise at run time.
constexpr double kFoldVoicing = 0.012589254117941675;   // -38 dB
constexpr double kRustleVoicing = 0.1778279410038923;   // -15 dB
// The room (Room.h), a hall with a hundred people. Its level keeps what Room
// means on the knob: at 70 % the Paris Musette demo's channels correlate as
// they did in each room before it (0.49 by the phase transform, 170 ms
// frames, the median peak at zero lag), where the owner's five solo
// recordings read 0.20-0.51, median 0.38. The hall's longer tail needs 0.71
// times the twenty-person room's level for that (the same 0.29 zero-lag
// correlation, plain, as that room gave at 3.25).
constexpr double kRoomLevel = 2.32;
constexpr double kValveVoicing = 0.031622776601683794;   // -30 dB: the push valves' slaps (kFlapDensity), against their estimate

// Rank of the bass side a group sounds: channel 1 bass buttons, channel 2 chords.
enum Group { kTreble, kBassButtons, kChordButtons };

// Which reeds a key of a channel plays: its own treble reeds, or, on the split
// keyboard's low keys, the bass or chord buttons'.
Group GroupOf(int channel, int key, bool split) {
    if (channel == 1) return kBassButtons;
    if (channel == 2) return kChordButtons;
    if (split && key < 36) return kBassButtons;
    if (split && key < 48) return kChordButtons;
    return kTreble;
}

// Where a part lands along its side's key bed (KeyBed), 0..1: a treble key
// by its place on the keyboard, C3 to C8 across the panel; a bass or chord
// button by its column, the Stradella board's circle of fifths. The register
// switches sit over the keyboard's top end, the air button at the bass
// board's end and the strap across its middle; all estimates.
constexpr double kSwitchPlace = 0.93, kAirButtonPlace = 0.9, kStrapPlace = 0.5;
double KeyPlace(int source, bool button) {
    const int key = source % 128;
    if (button) return 0.1 + 0.8 * ((key * 7) % 12) / 11.0;
    return 0.08 + 0.84 * Clamp((key - 48) / 60.0, 0.0, 1.0);
}

// Whether a register sounds `rank` for a (folded) treble key. Broken back
// above C#7, the 4' would double the 8' in unison, a cent or so apart, and
// the two would beat slowly down to near silence; where the 8' is open the
// 4' stays shut there instead.
bool Sounds(const Register& reg, int rank, int key) {
    if (!reg.ranks[static_cast<std::size_t>(rank)]) return false;
    return rank != kRankH || key + 12 <= kHighestReed || !reg.ranks[kRankM];
}

// The microphones: a coincident pair of cardioids (XY) kMicDistance in front
// of the instrument, in the player's view, the treble on the right. The treble
// keyboard stands upright, so its pallet holes and the grille over them are
// one place across the pair, not spread by pitch; the bass end cover is the
// other end of the instrument. A coincident pair hears them by level alone,
// consistent with the owner's five solo recordings, though not proof of their
// microphone setup: their channels align at zero lag and differ by 0.1-2.2 dB
// in every band from 60 Hz to 12 kHz, where
// panning by pitch put the bass side 8-10 dB to one side (Docs/decisions.md).
// The positions are a full-size instrument's halves, estimated.
constexpr double kMicDistance = 1.0;                        // m
constexpr double kTrebleSideX = 0.20, kBassSideX = -0.25;   // m right of the bellows' middle
// Width turns the pair's axes apart: 0 one microphone, 0.6 (the default) the
// usual XY pair's 90 degrees, 1 150 degrees.
constexpr double kMicHalfAngle = 75.0 * kPi / 180.0;

// The pair's gains for a source x m to the right, at the source's power.
void MicPair(double x, double width, float& left, float& right) {
    const double azimuth = std::atan2(x, kMicDistance), axis = width * kMicHalfAngle;
    const double l = 1.0 + std::cos(azimuth + axis), r = 1.0 + std::cos(azimuth - axis);
    const double norm = 1.0 / std::sqrt(l * l + r * r);
    left = static_cast<float>(l * norm);
    right = static_cast<float>(r * norm);
}

} // namespace

// Per unit of cover area the holes' air mass is rho l' / open and the
// compartment's compliance depth / (rho c^2), so f = c / (2 pi) sqrt(open /
// (depth l')). The cover is large against the wavelength there, so radiation
// loads it with rho c, and the cloth behind it adds its flow resistance:
// Q = sqrt(mass / compliance) / (rho c + cloth).
void Engine::Compartment::Set(double depth, double open, double neck, double rayl, double absorbed, double modesHz, double rate) {
    const double mass = kRho * neck / open, compliance = depth / kRhoC2;
    const double hz = 1.0 / (2.0 * kPi * Sqrt(mass * compliance)), q = Sqrt(mass / compliance) / (Sqrt(kRho * kRhoC2) + rayl);
    double k = std::tan(kPi * Min(hz, 0.45 * rate) / rate), norm = 1.0 / (1.0 + k / q + k * k);
    box.b0 = box.b2 = k * k * norm;
    box.b1 = 2.0 * box.b0;
    box.a1 = 2.0 * (k * k - 1.0) * norm;
    box.a2 = (1.0 - k / q + k * k) * norm;
    const double leakHz = Sqrt(kRhoC2 / kRho) * open / (kPi * neck * Sqrt(absorbed));
    k = std::tan(kPi * Min(leakHz, 0.45 * rate) / rate);
    leak.b0 = leak.b1 = k / (1.0 + k);
    leak.a1 = (k - 1.0) / (k + 1.0);
    k = std::tan(kPi * Min(modesHz, 0.45 * rate) / rate), norm = 1.0 / ((1.0 + k) * (1.0 + k));
    modes.b0 = modes.b2 = norm;
    modes.b1 = -2.0 * norm;
    modes.a1 = 2.0 * (k * k - 1.0) * norm;
    modes.a2 = (1.0 - k) * (1.0 - k) * norm;
}

void Engine::Compartment::FlushTiny() {
    for (Section* s : { &box, &leak, &modes })
        if (Abs(s->z1) >= 1.0e-30 || Abs(s->z2) >= 1.0e-30) return;
    Clear();
}

void Engine::Hand::Tick(double dt, double momentum, const double* drive, double driveVelocity) {
    if (!placed && received) {
        wheel = lastTarget = target;
        if (!drive) position = target;
        placed = true;
    }
    if (!placed && !drive) return;
    if (target != lastTarget) {
        const double interval = Clamp(sinceStep, dt, 0.3);
        lastStep = target - lastTarget;
        speed = lastStep / interval;
        lastTarget = target;
        sinceStep = 0.0;
    }
    sinceStep += dt;
    if (Abs(speed) * sinceStep > Abs(lastStep)) speed = lastStep / sinceStep;
    const double next = Clamp(wheel + dt * (speed + kHandCatchUp * (target - wheel)), 0.0, 1.0);
    const double wheelVelocity = (next - wheel) / dt;
    wheel = next;
    // Auto's player leads their own arm by the trail it would keep behind a
    // steady stroke (2 / w for this critically damped arm), so it follows the
    // stroke they mean and its mass shows only where the stroke changes, at a
    // turn. The wheel's arm keeps its trail: there the arm is the player's.
    const double w = momentum > 0.0 ? Min(1.0e3 / momentum, 0.5 / dt) : 0.0;
    const double goal = drive ? *drive + (w > 0.0 ? 2.0 / w * driveVelocity : 0.0) : wheel;
    if (momentum <= 0.0) {
        position = goal;
        velocity = drive ? driveVelocity : wheelVelocity;
        return;
    }
    // Semi-implicit Euler, kept well inside its stable range (w dt <= 0.5)
    // for a Momentum shorter than two samples.
    velocity += dt * w * (w * (goal - position) - 2.0 * velocity);
    position = drive ? position + dt * velocity : Clamp(position + dt * velocity, 0.0, 1.0);
}

Engine::Engine() { Prepare(48000.0); }

void Engine::Prepare(double sampleRate) {
    rate = sampleRate > 1000.0 && sampleRate < 1.0e6 ? sampleRate : 48000.0;
    bassBox.Set(kBassBoxDepth, kBassBoxOpen, kBassHoleLength, kClothRayl, kBassBoxAbsorbed, kBassBoxModes, rate);
    rustle.Set(rate);
    smoothing = 1.0 - std::exp(-1.0 / (0.02 * rate));
    bendSmoothing = 1.0 - std::exp(-1.0 / (0.005 * rate));
    wanderStep = 1.0 - std::exp(-6.283185307179586 * kHandWanderHz / rate);
    wanderScale = kHandWander / std::sqrt(CascadeVariance(wanderStep));
    life.Prepare(rate);
    touch.Prepare(rate);
    output.Prepare(rate, kBellowsGuard);
    room.Set(Room::kDecaySeconds, Room::kTrebleShare, rate);
    SetControls(controls);
    Reset();
}

void Engine::Reset() {
    for (auto& c : cells) c = Cell {};
    hand = Hand {};
    player = Player {};
    life.Reset();
    touch.Reset();
    heldCells = 0;
    pressure = airFlow = airFlowSmoothed = demand = 0.0;
    valve = valveNoiseA = valveNoiseB = valveSound = 0.0;
    bend2 = bend2Target = 1.0;
    airPedal = false;
    trebleHeld.fill(0);
    channelDepth = { 1.0, 1.0, 1.0 };
    msb = lsb = 0;
    fineWheel = false;
    order = 0;
    noise = 1u;
    sounding = 0;
    trebleSide.Reset();
    bassSide.Reset();
    bassBox.Clear();
    grille = 0.0;
    output.Reset();
    room.Clear();
    for (auto& mic : trebleMicMotion) mic.Clear();
    for (auto& mic : bassMicMotion) mic.Clear();
    keys.fill(Key {});
    movingCount = 0;
    knocks.fill(Knock {});
    slaps.fill(Knock {});
    knocksUsed = slapsUsed = 0;
    strokeLuck = 1u;
    airHeld = false;
    airStroke = 1.0;
    // A quarter of the panels in each wall, two long and two short; each
    // panel's mass and play differ a little from its neighbours' (+-20 %,
    // fixed per instrument), so a turn's panels land one after another.
    for (int i = 0; i < kPleats; ++i) {
        Pleat& p = folds.pleats[static_cast<std::size_t>(i)];
        p = Pleat {};
        const std::uint32_t h = Hash(3u, static_cast<std::uint32_t>(i));
        p.area = kPleatDepth * ((i / (kPleats / 4)) % 2 == 0 ? kPleatLongWall : kPleatShortWall);
        p.mass = kPleatSurfaceMass * p.area * (1.0 + 0.2 * Unit(h));
        p.play = kPleatPlay * (1.0 + 0.2 * Unit(Hash(h, 1)));
    }
    folds.Rest();
    rustle = Rustle {};
    rustle.Set(rate);
    rustleForce = 0.0;
    strapPushed = false;
    // Afresh, as every landing's scatter moved their modes and drew on their
    // luck (KeyBed::Scatter).
    keyBedTreble.Set(rate, false);
    keyBedBass.Set(rate, true);
    UpdateMicrophones();
    TakeTargets();
    diag = Diagnostics {};
    untouched = true;
}

void Engine::TakeTargets() {
    gain = gainTarget;
    bassGain = bassGainTarget;
    trebleGain = trebleGainTarget;
    grilleCoeff = grilleCoeffTarget;
    treblePanL = treblePanLTarget;
    treblePanR = treblePanRTarget;
    bassPanL = bassPanLTarget;
    bassPanR = bassPanRTarget;
    foldsPanL = foldsPanLTarget;
    foldsPanR = foldsPanRTarget;
    roomGain = roomGainTarget;
    mechanicsGain = mechanicsGainTarget;
    for (auto& mic : trebleMicMotion) mic.TakeTarget();
    for (auto& mic : bassMicMotion) mic.TakeTarget();
}

void Engine::SetControls(const Controls& next) {
    const double prevLife = controls[kLife];
    for (int i = 0; i < kControlCount; ++i) {
        const ControlInfo& info = kControlInfo[static_cast<std::size_t>(i)];
        const double v = next[i];
        controls[i] = v == v ? Clamp(v, info.min, info.max) : info.initial;
        if (info.steps > 0) controls[i] = std::floor(controls[i] + 0.5);
    }
    grilleCoeffBase = 1.0 - std::exp(-2.0 * kPi * Min(controls[kGrille], 0.45 * rate) / rate);
    grilleCoeffTarget = grilleCoeffBase;
    const int index = static_cast<int>(controls[kOversampling]);
    const double nextRate = rate * (kBaseOversample << index);
    if (nextRate != internalRate)
        for (auto& c : cells) {   // a lifting or closing pallet keeps its travel time
            c.openRate *= internalRate / nextRate;
            c.vent = MakeVent(c.spec.chamber, c.lift, nextRate);
        }
    oversample = kBaseOversample << index;
    stages = 2 + index;
    internalRate = nextRate;
    if (controls[kLife] != prevLife)
        for (auto& c : cells) {
            if (!c.active) continue;
            const int r = c.rank, k = c.reed - r * 128;
            const int idx = TuningIndex(r, k);
            c.spec = MakeCell(r, k, controls[kMusette], kReedTuning[r][idx], rate * kBaseOversample, controls[kLife]);
            if (masterTune != 0.0) {
                const double scale = std::pow(2.0, masterTune / 1200.0);
                Retune(c.spec.push, scale);
                Retune(c.spec.pull, scale);
            }
            c.vent = MakeVent(c.spec.chamber, c.lift, internalRate);
        }
    const int reg = static_cast<int>(controls[kRegister]);
    // Until it first renders, the engine has made no sound: controls set then
    // (a patch loaded into a new instance) take hold at once, with no knock
    // and no glide, so a new instance plays as one after Reset() does.
    if (reg != registerIndex) {
        // The thumb's press moves the slides and lands the switch alike.
        const double stroke = untouched ? 1.0 : Stroke();
        SwitchRegister(registerIndex, reg, stroke);
        if (!untouched) {
            StartKnock(kRegisterSwitch, stroke * kRegisterSwitch.travel / kOpenSeconds, 1.0, false, false, kSwitchPlace);
            // The switch it pushes off springs back up onto its stop, in the
            // time a pallet's spring takes.
            StartKnock(kRegisterSwitch, kRegisterSwitch.travel / kCloseSeconds, 1.0, false, true, kSwitchPlace);
        }
    }
    registerIndex = reg;
    const bool nextAuto = controls[kAuto] >= 0.5;
    if (autoHand && !nextAuto) {   // the wheel takes the hand where it is, without a stroke
        if (hand.placed) hand.position = hand.wheel;
        else hand.velocity = 0.0;
    }
    if (nextAuto != autoHand) {
        touch.ClearGesture();
        player.running = player.ready = false;
        player.drive = hand.position;
    }
    autoHand = nextAuto;
    // The gap turbulence's force on the tongues is white per internal step; it
    // scales with the square root of the step rate so its spectrum, the noise
    // the reeds hear, stays the same at every rate and Oversampling. The flow's
    // fluctuation, the noise they make, is shaped to its eddies.
    noiseScale = Sqrt(internalRate / kNoiseRate);
    turbulence = kTurbulence * noiseScale;
    gainTarget = kRho / (4.0 * kPi) * kPascalToFullScale * DbToGain(controls[kVolume]);
    bassGainTarget = DbToGain(controls[kBass]);
    trebleGainTarget = DbToGain(kRegisterTrimDb[static_cast<std::size_t>(reg)]);
    mechanicsGainTarget = kForceToFlow * DbToGain(controls[kMechanics]);
    roomGainTarget = kRoomLevel * controls[kRoom];
    room.SetWidth(controls[kWidth], untouched || (roomGain == 0.0 && roomGainTarget == 0.0));
    MicPair(kTrebleSideX, controls[kWidth], trebleMicL, trebleMicR);
    MicPair(kBassSideX, controls[kWidth], bassMicL, bassMicR);
    micCos = std::cos(controls[kWidth] * kMicHalfAngle);
    micSin = std::sin(controls[kWidth] * kMicHalfAngle);
    const auto reference = [&](double x, std::array<MicrophoneMotion, 2>& pair) {
        const double r = Sqrt(x * x + kMicDistance * kMicDistance);
        pair[0].SetReference(r, (kMicDistance * micCos - x * micSin) / r, rate);
        pair[1].SetReference(r, (kMicDistance * micCos + x * micSin) / r, rate);
    };
    reference(kTrebleSideX, trebleMicMotion);
    reference(kBassSideX, bassMicMotion);
    UpdateMicrophones();
    if (untouched) TakeTargets();
}

void Engine::UpdateMicrophones() {
    // Life=0 is the original pickup, including its exact normalization.
    treblePanLTarget = trebleMicL; treblePanRTarget = trebleMicR;
    bassPanLTarget = bassMicL; bassPanRTarget = bassMicR;
    foldsPanLTarget = 0.70710678f; foldsPanRTarget = 0.70710678f;
    grilleCoeffTarget = grilleCoeffBase;
    for (auto& mic : trebleMicMotion) mic.RestoreReference();
    for (auto& mic : bassMicMotion) mic.RestoreReference();
    if (life.amount == 0.0) return;
    // Movement matters with a fixed microphone: Shure's own accordion test,
    // https://www.shure.com/en-US/insights/eight-ways-to-mic-an-accordion .
    // Ideal cardioid reception and 1/r spreading are geometry, not an EQ.
    // Full Life permits at most +/-5 cm sideways and +/-8 cm toward/away
    // from the pair. These are selectable performer motions, NOT fitted
    // directivity measurements. No Doppler is claimed for the instrument.
    // MicrophoneMotion adds the relative spherical-wave proximity response of
    // these ideal cardioids, while the treble grille's baffled-piston aperture
    // shades its off-axis high-frequency cutoff as the player sways and the
    // bellows fan open (Beranek, Acoustics, 1954, p. 101: 2 J1(k a sin theta)
    // / (k a sin theta) ~= 1 - (k a sin theta)^2 / 8).
    const double side = 0.05 * life.amount * Clamp((life.side + life.effort) / 3.0, -1.0, 1.0);
    const double depth = 0.08 * life.amount * Clamp((life.depth + life.effort) / 3.0, -1.0, 1.0);
    // The bass end also moves with the bellows, over their 77 mm of travel
    // (kBellowsVolume). Treble stays at the chest.
    const double extension = life.amount * (kBellowsVolume / kBellowsSection) * (Extension() - 0.5);
    const double ca = micCos, sa = micSin;
    const auto panAt = [&](double x0, double shift, float& left, float& right) {
        const double r0 = Sqrt(x0 * x0 + kMicDistance * kMicDistance);
        const double x = x0 + side + shift, z = kMicDistance + depth;
        const double r = Sqrt(x * x + z * z);
        const double oldL = 1.0 + (kMicDistance * ca - x0 * sa) / r0;
        const double oldR = 1.0 + (kMicDistance * ca + x0 * sa) / r0;
        left *= static_cast<float>((1.0 + (z * ca - x * sa) / r) / oldL * r0 / r);
        right *= static_cast<float>((1.0 + (z * ca + x * sa) / r) / oldR * r0 / r);
    };
    const auto move = [&](double x0, double shift, float& left, float& right,
                          std::array<MicrophoneMotion, 2>& pair) {
        panAt(x0, shift, left, right);
        const double x = x0 + side + shift, z = kMicDistance + depth;
        const double r = Sqrt(x * x + z * z);
        pair[0].Move(r, (z * ca - x * sa) / r);
        pair[1].Move(r, (z * ca + x * sa) / r);
    };
    move(kTrebleSideX, 0.0, treblePanLTarget, treblePanRTarget, trebleMicMotion);
    move(kBassSideX, -extension, bassPanLTarget, bassPanRTarget, bassMicMotion);
    // The bellows folds sit midway between the chest-anchored treble frame and
    // the moving left-hand bass casing (-0.5 * extension).
    panAt(0.0, -0.5 * extension, foldsPanLTarget, foldsPanRTarget);
    // Upper-body sway and the bellows' fan opening rotate the treble grille's
    // normal relative to the microphone stand, shading its effective cutoff.
    const double r0 = Sqrt(kTrebleSideX * kTrebleSideX + kMicDistance * kMicDistance);
    const double sin0 = kTrebleSideX / r0;
    const double x = kTrebleSideX + side, z = kMicDistance + depth, r = Sqrt(x * x + z * z);
    const double yaw = 0.8 * side / kMicDistance - 0.25 * extension / kPleatLongWall;
    const double sinTheta = Clamp((x - z * yaw) / r, -0.95, 0.95);
    const double aperture = Clamp((1.0 - 0.85 * sinTheta * sinTheta) / (1.0 - 0.85 * sin0 * sin0), 0.65, 1.35);
    grilleCoeffTarget = Clamp(grilleCoeffBase * aperture, 0.0, 1.0);
}

void Engine::SetMasterTune(double cents) { masterTune = cents == cents ? Clamp(cents, -100.0, 100.0) : 0.0; }

void Engine::SetPitchBend(double amount) {
    const double semitones = kBendSemitones * (amount == amount ? Clamp(amount, -1.0, 1.0) : 0.0);
    bend2Target = std::pow(2.0, semitones / 6.0);   // (2^(st/12))^2
}

void Engine::SetHand(double position) {
    hand.target = position == position ? Clamp(position, 0.0, 1.0) : hand.target;
    hand.received = true;
}

double Engine::PlayerEffort() const {
    double effort = 1.0 + player.level + wanderScale * player.wanderB;
    if (life.amount != 0.0)
        effort = Clamp(effort + 0.22 * life.amount * life.effort, 0.65, 1.45);
    if (touch.amount != 0.0)
        effort = Clamp(effort * (1.0 + touch.Effort()), 0.35, 1.45);
    return effort;
}

// Auto's bellows are endless, so nothing that depends on how far open the
// bellows are moves while it plays: they stay at mid-travel. The wheel's run
// from 0 to 1 is the whole travel.
double Engine::Extension() const { return autoHand ? 0.5 : Clamp(hand.position, 0.0, 1.0); }

// Runs before the hand's tick, from the keys the last sample held.
void Engine::StepPlayer(double dt) {
    // Auto's arm moves the bellows' own air: the player asks it for the air
    // the reeds need, so Assist only lets the hand go faster for more open
    // reeds. Multiplying the arm's motion by the open reeds' demand, as the
    // wheel's hand does, made a turn in a rest arrive 2-6 times over its
    // pressure when keys went down while the arm still swung at full speed.
    const double litres = controls[kBellows] * 1.0e-3;   // per wheel range
    const double perWheel = litres;
    const double fastest = kAutoFastest * (1.0 + controls[kAssist] * (Max(demand, 1.0) - 1.0)) / litres;
    const double momentum = controls[kMomentum];
    const double size = kAutoPressure * (1.0 - (1.0 - kAutoSoftShare) * hand.wheel);
    const bool pulling = player.pull;
    // With every key up the hand rests and the closed pallets keep the
    // bellows at their pressure, unless a keyswitch has turned them: the hand
    // then swings the pressure round with the pallets shut, as a player turns
    // in a rest, so the next note speaks the new way at once. A first note has
    // no pressure to turn.
    if (heldCells == 0 && (!player.ready || (pulling == player.pulled && !player.swinging))) {
        player.running = false;
        player.velocity = 0.0;
        return;
    }
    const double want = pulling ? -size : size;
    if (!player.running) {
        player.running = true;
        player.drive = hand.position;
        player.pulled = pulling;
        // A stroke that finds the bellows short of half the pressure it wants
        // (the first note, or a turn's other side) is swung up at the hand's
        // full speed, as a turn is: closing on it in a reaction time made a
        // first note speak 20-25 dB under itself for its first 100 ms.
        player.swinging = pressure * want < 0.5 * want * want;
        player.ready = true;
        NewStroke();
    }
    player.luck = player.luck * 1664525u + 1013904223u;
    player.wanderA += wanderStep * (1.7320508075688772 * Unit(player.luck) - player.wanderA);
    player.wanderB += wanderStep * (player.wanderA - player.wanderB);
    // The player feels the air the reeds take and gives it, and expects it at
    // the rate the arm can follow, which also keeps the reeds' own ripple out
    // of what they expect.
    const double air = airFlow + kBellowsLeak * pressure;
    const double arm = momentum > 0.0 ? Min(1.0e3 / momentum, 0.5 / dt) : 0.0;   // as Hand::Tick
    player.hold = arm > 0.0 ? player.hold + Min(1.0, dt * arm) * (air - player.hold) : air;
    // A keyswitch turns the bellows: the player expects the reeds to take the
    // air they took, the other way, and moves the hand as fast as it goes
    // until the pressure the arm will bring is the new one; then they play on.
    if (pulling != player.pulled) {
        player.pulled = pulling;
        player.hold = -player.hold;
        player.swinging = true;
        NewStroke();
    }
    // The pressure once the led arm has caught up with the hand. The player
    // feels their own arm and closes on this, so its lag stays out of their
    // loop.
    const double coming = pressure - kBellowsStiffness * perWheel * (player.drive - hand.position);
    // They close on the pressure they want in a reaction time
    // (above): the air that time asks of the bellows' stiffness. The hand's
    // air is undone through Assist and Bellows into wheel speed; while
    // swinging through a turn it goes one way at full speed, and once on the
    // stroke it can yield as the wheel eases so a held note softens at once.
    const double closing = player.swinging ? dt : kReaction;
    // Life continues on held notes; Touch sets both initial accent and
    // sustained velocity speed, and the wheel scales the lot.
    const double targetSize = kAutoPressure * PlayerEffort() * (1.0 - (1.0 - kAutoSoftShare) * hand.wheel);
    const double meant = pulling ? -targetSize : targetSize;
    const double flow = air + (meant - coming) / (kBellowsStiffness * closing);
    const double wanted = -flow / perWheel;
    const double low = player.swinging && pulling ? 0.0 : -fastest;
    const double high = player.swinging && !pulling ? 0.0 : fastest;
    player.velocity = Clamp(wanted, low, high);
    if (player.velocity == wanted) player.swinging = false;
    player.drive += dt * player.velocity;   // no end to run into
}

// A stroke's own level, uniform with kHandStroke's standard deviation.
void Engine::NewStroke() {
    player.luck = player.luck * 1664525u + 1013904223u;
    player.level = 1.7320508075688772 * kHandStroke * Unit(player.luck);
}

void Engine::SetChannelPressure(int channel, double amount) {
    if (channel < 0 || channel > 2) return;
    const double depth = 1.0 - (amount == amount ? Clamp(amount, 0.0, 1.0) : 0.0) * (1.0 - kHalfPressed);
    channelDepth[static_cast<std::size_t>(channel)] = depth;
    for (auto& c : cells)
        if (c.active && c.held && c.source / 128 == channel) c.depth = depth;
    for (int source = channel * 128; source < channel * 128 + 128; ++source)
        if (keys[static_cast<std::size_t>(source)].target > 0.0) {
            keys[static_cast<std::size_t>(source)].target = depth;
            MoveKey(source);
        }
}

void Engine::MoveKey(int source) {
    Key& k = keys[static_cast<std::size_t>(source)];
    if (k.listed || k.travel == k.target) return;
    k.listed = true;
    movingKeys[static_cast<std::size_t>(movingCount++)] = static_cast<std::uint16_t>(source);
}

// A finger stroke runs a little faster or slower each time (kStrokeScatter):
// this one's velocity against the one asked for, uniform with that SD,
// scaled by Life as the reeds' imperfections are.
double Engine::Stroke() {
    strokeLuck = strokeLuck * 1664525u + 1013904223u;
    return 1.0 + kStrokeScatter * ImperfectionScale(controls[kLife]) * 1.7320508075688772 * Unit(strokeLuck >> 8);
}

// A part landing on its stop. A finger's stroke (or the spring's return it
// rides) lands at its own speed (Stroke); a key's descent already carries its
// stroke's (NoteOn), and the strap, taken up at the bellows' own speed, has
// none. It lands at `place` along its side's key bed, which it rings
// (KeyBed). When every knock is still sounding, a new one in that crowd is
// dropped.
void Engine::StartKnock(const Stop& stop, double speed, double massScale, bool bass, bool stroke, double place) {
    if (stroke) speed *= Stroke();
    for (int i = 0; i < static_cast<int>(knocks.size()); ++i) {
        Knock& k = knocks[static_cast<std::size_t>(i)];
        if (k.active) continue;
        k.Start(stop, speed, massScale);
        k.bass = bass;
        KeyBed::Couple(place, k.coupling);
        (bass ? keyBedBass : keyBedTreble).Scatter(kKeyBedScatter * ImperfectionScale(controls[kLife]));
        knocksUsed = static_cast<int>(Max(knocksUsed, i + 1));
        return;
    }
}

// A push valve landing on its plate; when every slap is still sounding, a
// new one is dropped.
void Engine::StartSlap(double mass, const FlapLanding& landing, bool bass) {
    for (int i = 0; i < static_cast<int>(slaps.size()); ++i) {
        Knock& k = slaps[static_cast<std::size_t>(i)];
        if (k.active) continue;
        k.Start(Stop { mass, kFlapLift, &kRestCloth }, landing.speed, 1.0, static_cast<int>(landing.delay * rate + 0.5));
        k.bass = bass;
        slapsUsed = static_cast<int>(Max(slapsUsed, i + 1));
        return;
    }
}

// How a cell's push or pull valve lands: driven shut by a turn reversing at
// turnRate Pa/s, or else by its own curl. Its lift and curl are
// its own (kFlapSpread), fixed per instrument.
FlapLanding Engine::ValveLanding(const Cell& c, bool pull, bool turn, double turnRate) const {
    const std::uint32_t h = Hash(11u, static_cast<std::uint32_t>(c.reed * 2 + (pull ? 1 : 0)));
    const double lift = kFlapLift * (1.0 + kFlapSpread * Unit(h));
    return turn ? FlapTurn(turnRate, lift) : FlapCurl(lift, kFlapOpenPa * (1.0 + kFlapSpread * Unit(Hash(h, 1))));
}

// The keys move as their pallets do (Render): down at the stroke's speed,
// up in the time the spring takes, accelerated by suction on pull and
// cushioned by chamber pressure on push (seatScale). A key knocks on its felt
// when it reaches the bottom and its pallet on its seat when it is back up; a
// key that turns before either does not. Each key's felt and leverage differ a
// little from its neighbours' (mass +-20 %, fixed per instrument, as the reeds
// differ).
void Engine::StepKeys(double seatScale) {
    const double up = seatScale / (kCloseSeconds * rate);
    for (int i = 0; i < movingCount;) {
        const int source = movingKeys[static_cast<std::size_t>(i)];
        Key& k = keys[static_cast<std::size_t>(source)];
        const Stop* stop = nullptr;
        double speed = 0.0;
        if (k.travel < k.target) {
            k.travel = Min(k.target, k.travel + k.rate);
            if (k.travel >= 1.0) {
                stop = k.button ? &kButton : &kTrebleKey;
                speed = stop->travel * k.rate * rate;
            }
        } else {
            k.travel = Max(k.target, k.travel - up * k.up);
            if (k.travel <= 0.0) {
                stop = k.button ? &kBassPallet : &kTreblePallet;
                speed = stop->travel * up * k.up * rate;
            }
        }
        // Both landings already carry the stroke's speed (NoteOn, NoteOff).
        if (stop) StartKnock(*stop, speed, 1.0 + 0.2 * Unit(Hash(2u, static_cast<std::uint32_t>(source))), k.button, false, KeyPlace(source, k.button));
        if (k.travel == k.target) {
            k.listed = false;
            movingKeys[static_cast<std::size_t>(i)] = movingKeys[static_cast<std::size_t>(--movingCount)];
        } else {
            ++i;
        }
    }
}

void Engine::StartCell(int rank, int key, int source, double openSeconds) {
    const int reed = rank * 128 + key;
    Cell* cell = nullptr;
    for (auto& c : cells)   // a releasing chamber of the same reed opens again
        if (c.active && c.reed == reed) cell = &c;
    if (!cell)
        for (auto& c : cells)
            if (!c.active) { cell = &c; break; }
    if (!cell) {   // steal the oldest released cell, else the oldest
        cell = &cells[0];
        for (auto& c : cells)
            if ((c.held == cell->held && c.order < cell->order) || (!c.held && cell->held)) cell = &c;
        cell->active = false;
    }
    if (!cell->active) {
        const int index = TuningIndex(rank, key);
        // Musette, master tune and Life changes reach the reeds at their next note.
        cell->spec = MakeCell(rank, key, controls[kMusette], kReedTuning[rank][index], rate * kBaseOversample, controls[kLife]);
        if (masterTune != 0.0) {
            const double scale = std::pow(2.0, masterTune / 1200.0);
            Retune(cell->spec.push, scale);
            Retune(cell->spec.pull, scale);
        }
        cell->state = CellState {};
        cell->breathA = cell->breathB = 0.0;
        cell->state.push.x = cell->spec.push.rest;
        cell->state.pull.x = cell->spec.pull.rest;
        // A closed chamber sits at bellows pressure: air leaks in or out through
        // the clearance of whichever reed the bellows drives.
        for (double& p : cell->state.p) p = pressure;
        cell->open = cell->lift = 0.0;
        cell->radiated = 0.0;
        cell->vent = Vent {};
        cell->valveOpen = cell->pullValveOpen = false;
        cell->valveMass = kFlapDensity * FlapArea(cell->spec.push.length, cell->spec.push.width);
        cell->pullValveMass = kPullValveShare * kFlapDensity * FlapArea(cell->spec.pull.length, cell->spec.pull.width);
        cell->reed = reed;
        cell->rank = rank;
    }
    cell->active = cell->held = true;
    cell->source = source;
    cell->order = ++order;
    cell->closedFor = 0.0;
    cell->depth = channelDepth[static_cast<std::size_t>(source / 128)];
    cell->settled = false;
    // A sharper key stroke lifts the pallet faster; secondary ranks routed
    // through register-slide apertures and side channels unseat with slight
    // mechanical skew scaled by Life (0 at Life=0, +-16 % at default Life=0.5,
    // +-32 % at Life=1), while the primary rank directly under the pallet
    // opens on the key's exact travel.
    const bool primaryRank = rank == kRankM || rank == kRankBass8 || rank == kRankChord8;
    const double rankSkew = primaryRank
        ? 1.0
        : 1.0 + 0.16 * ImperfectionScale(controls[kLife]) * Unit(Hash(7u, static_cast<std::uint32_t>(reed)));
    cell->openRate = rankSkew / (openSeconds * internalRate);
    cell->closeStroke = 1.0;
}

void Engine::NoteOn(int channel, int key, int velocity) {
    const Group group = GroupOf(channel, key, controls[kKeyboard] < 0.5);
    const int source = channel * 128 + key;
    Key& k = keys[static_cast<std::size_t>(source)];
    k.target = channelDepth[static_cast<std::size_t>(channel)];
    // The stroke the finger actually makes (Stroke) moves the key and every
    // pallet it lifts, within the mapping's bounded travel. The measured
    // scatter is of MIDI velocity, so it goes through the same mapping: a few
    // tenths of a millisecond either way, the most on the hardest strokes.
    const double opening = KeyOpeningSeconds(Clamp(velocity * Stroke(), 1.0, 127.0));
    k.rate = 1.0 / (opening * rate);
    k.up = 1.0;
    k.velocity = static_cast<std::uint8_t>(velocity);
    k.button = group != kTreble;
    MoveKey(source);
    if (autoHand) touch.Press(velocity);
    if (group == kBassButtons) {
        StartCell(kRankBass16, PitchClass(key), source, opening);
        StartCell(kRankBass8, PitchClass(key), source, opening);
    } else if (group == kChordButtons) {
        StartCell(kRankChord8, PitchClass(key), source, opening);
        StartCell(kRankChord4, PitchClass(key), source, opening);
    } else {
        const Register& reg = kRegisters[static_cast<std::size_t>(controls[kRegister])];
        for (int rank = kRankL; rank <= kRankH; ++rank)
            if (Sounds(reg, rank, TrebleKey(key))) StartCell(rank, TrebleKey(key), source, opening);
        trebleHeld[static_cast<std::size_t>(key)] = static_cast<std::uint8_t>(velocity);
    }
}

// The register switches move slides over the treble reed blocks, so a switch
// pressed while keys are held reaches them: the ranks it opens start to speak
// and the ones it shuts fall silent. A slide opens about as fast as the
// softest key stroke lifts a pallet, and closes as a pallet does, both at the
// speed of this press (stroke).
void Engine::SwitchRegister(int from, int to, double stroke) {
    const Register& before = kRegisters[static_cast<std::size_t>(from)];
    const Register& after = kRegisters[static_cast<std::size_t>(to)];
    for (int key = 0; key < 128; ++key) {
        if (trebleHeld[static_cast<std::size_t>(key)] == 0) continue;
        for (int rank = kRankL; rank <= kRankH; ++rank) {
            const bool was = Sounds(before, rank, TrebleKey(key)), is = Sounds(after, rank, TrebleKey(key));
            if (is && !was) StartCell(rank, TrebleKey(key), key, KeyOpeningSeconds(0) / stroke);
            else if (was && !is)
                for (auto& c : cells)
                    if (c.active && c.held && c.source == key && c.rank == rank) {
                        c.held = false;
                        c.closeStroke = stroke;
                    }
        }
    }
}

void Engine::NoteOff(int channel, int key) {
    const int source = channel * 128 + key;
    // A bass or chord reed is one per pitch class, so buttons an octave apart
    // open the same chamber, which belongs to the latest of them. It keeps
    // speaking, now for another button still down, until none is.
    int other = -1;
    const bool split = controls[kKeyboard] < 0.5;
    const Group group = GroupOf(channel, key, split);
    if (group != kTreble)
        for (int s = 0; s < 3 * 128 && other < 0; ++s)
            if (s != source && keys[static_cast<std::size_t>(s)].target > 0.0 && GroupOf(s / 128, s % 128, split) == group
                && PitchClass(s % 128) == PitchClass(key))
                other = s;
    // The finger lets the key up a little faster or slower each time: it
    // rides the spring's return part of the way (kStrokeScatter's note).
    const double lift = Stroke();
    for (auto& c : cells)
        if (c.active && c.held && c.source == source) {
            if (other < 0) {
                c.held = false;
                c.closeStroke = lift;
            } else {
                c.source = other;
                c.depth = channelDepth[static_cast<std::size_t>(other / 128)];
            }
        }
    if (channel == 0) trebleHeld[static_cast<std::size_t>(key)] = 0;
    Key& released = keys[static_cast<std::size_t>(source)];
    released.target = 0.0;
    released.velocity = 0;
    released.up = lift;
    MoveKey(source);
    if (autoHand) {
        int heldVelocity = 0;
        for (const auto& k : keys)
            if (k.target > 0.0 && k.velocity > heldVelocity)
                heldVelocity = k.velocity;
        if (heldVelocity > 0) touch.Hold(heldVelocity);
    }
}

void Engine::AllNotesOff() {
    touch.ClearGesture();
    for (auto& c : cells) c.held = false;
    trebleHeld.fill(0);
    for (int source = 0; source < 3 * 128; ++source) {
        keys[static_cast<std::size_t>(source)].target = 0.0;
        keys[static_cast<std::size_t>(source)].velocity = 0;
        MoveKey(source);
    }
}

void Engine::Midi(const std::uint8_t* bytes, int size) {
    if (!bytes || size < 2) return;
    const int type = bytes[0] & 0xF0, channel = bytes[0] & 0x0F;
    const int d1 = bytes[1] & 0x7F, d2 = size > 2 ? bytes[2] & 0x7F : 0;
    const int ch = channel <= 2 ? channel : 0;
    const bool keyswitch = autoHand && ch == 0 && (d1 == kPullKey || d1 == kPushKey);
    if (type == 0x90 && d2 > 0) {
        if (keyswitch) player.pull = d1 == kPullKey;
        else NoteOn(ch, d1, d2);
    } else if (type == 0x80 || type == 0x90) {
        // A key pressed before Auto came on is still let go.
        if (!keyswitch || keys[static_cast<std::size_t>(d1)].target > 0.0) NoteOff(ch, d1);
    }
    else if (type == 0xB0) {
        if (d1 == 1) { msb = d2; lsb = 0; }
        else if (d1 == 33) { lsb = d2; fineWheel = true; }
        else if (d1 == 64) airPedal = d2 >= 64;
        else if (d1 == 120 || d1 == 123) AllNotesOff();
        else if (d1 == kHandModeCc) {   // who moves the bellows, as the Auto control
            Controls next = controls;
            next[kAuto] = d2 >= 64 ? 1.0 : 0.0;
            SetControls(next);
        }
        if (d1 == 1 || d1 == 33) SetHand(fineWheel ? (msb * 128 + lsb) / 16383.0 : msb / 127.0);
    } else if (type == 0xD0) {
        SetChannelPressure(ch, d1 / 127.0);
    } else if (type == 0xE0 && size > 2) {
        SetPitchBend((d2 * 128 + d1 - 8192) / 8192.0);
    }
}

void Engine::Render(float* left, float* right, int frames) {
    const double dt = 1.0 / rate, dtIn = 1.0 / internalRate, invDtIn = internalRate;
    const float glide = static_cast<float>(smoothing);
    const double closeRate = 1.0 / (kCloseSeconds * internalRate);
    const double litres = controls[kBellows] * 1.0e-3, assist = controls[kAssist];
    const bool airValve = airPedal || controls[kAir] >= 0.5;
    // The valve opens and closes in the time a pallet takes, at the speed of
    // this press or release of the button.
    if (airValve != airHeld) {
        airHeld = airValve;
        airStroke = Stroke();
    }
    const double valveOpen = airStroke * dt / kOpenSeconds, valveClose = airStroke * dt / kCloseSeconds;
    if (frames > 0) untouched = false;
    // Notes and register changes arrive between Render calls. Build this
    // ordered list once per callback; only a closing cell can leave it while
    // rendering. Preserve cell order for turbulence draws and summed airflow.
    Cell* live[kMaxCells];
    int groupOf[kMaxCells];
    int liveCount = 0, groupCount = 0;
    for (auto& c : cells) {
        if (!c.active) continue;
        int g = groupCount;
        for (int m = 0; m < liveCount; ++m)
            if (live[m]->source == c.source) { g = groupOf[m]; break; }
        if (g == groupCount) ++groupCount;
        groupOf[liveCount] = g;
        live[liveCount++] = &c;
    }
    for (int i = 0; i < frames; ++i) {
        touch.Step(controls[kTouch], autoHand);
        if (life.Step(controls[kLife], heldCells > 0 || Abs(hand.velocity) > 1.0e-5)) UpdateMicrophones();
        if (autoHand) StepPlayer(dt);
        hand.Tick(dt, controls[kMomentum], autoHand ? &player.drive : nullptr, player.velocity);
        const double bellowsStiffness = BellowsStiffness(Extension());
        // Assist makes the hand's stroke move as much more air as the open
        // reeds take beyond one A4 reed, so one wheel speed gives one pressure
        // whatever is played; at 0 the hand moves the bellows' own air only.
        // In Manual mode, orifice flow scales as sqrt(P), which would make P
        // collapse quadratically at slow wheel speeds (P ~ v^2) and stall the
        // reeds below ~0.06/s; shaping sub-reference wheel speeds by v^0.55
        // linearises steady pressure against wheel speed, while brisk wrist
        // take-up across zero pressure lets gentle manual starts and reversals
        // catch the reeds promptly.
        double effectiveHandVelocity = hand.velocity;
        if (!autoHand && heldCells > 0 && hand.velocity != 0.0) {
            constexpr double kManualRefSpeed = 0.095;
            const double speed = Abs(hand.velocity);
            if (speed < kManualRefSpeed) {
                const double u = speed / kManualRefSpeed;
                const double active = Clamp((speed - 0.001) / 0.004, 0.0, 1.0);
                double shaped = speed + active * (kManualRefSpeed * std::pow(u, 0.48) - speed);
                const double signedP = hand.velocity < 0.0 ? pressure : -pressure;
                if (signedP < 220.0)
                    shaped *= 1.0 + 2.4 * active * (1.0 - u) * Clamp((220.0 - signedP) / 220.0, 0.0, 2.0);
                effectiveHandVelocity = hand.velocity < 0.0 ? -shaped : shaped;
            } else if (speed < 0.115) {
                const double signedP = hand.velocity < 0.0 ? pressure : -pressure;
                if (signedP >= -20.0 && signedP < 180.0) {
                    const double w = (0.115 - speed) / (0.115 - kManualRefSpeed);
                    effectiveHandVelocity *= 1.0 + 1.5 * w * Clamp((180.0 - signedP) / 180.0, 0.0, 1.2);
                }
            }
        }
        double handFlow = -litres * effectiveHandVelocity * (autoHand ? 1.0 : 1.0 + assist * (Max(demand, 1.0) - 1.0));
        if (!autoHand && heldCells > 0 && hand.velocity != 0.0) {
            constexpr double kManualRefSpeed = 0.095;
            const double speed = Abs(hand.velocity);
            const double signedP = hand.velocity < 0.0 ? pressure : -pressure;
            if (speed > 0.001 && speed < kManualRefSpeed && signedP > 150.0 && Abs(handFlow) < Abs(airFlow)) {
                const double u = speed / kManualRefSpeed;
                const double active = Clamp((speed - 0.001) / 0.004, 0.0, 1.0);
                handFlow += 3.0 * active * (1.0 - u) * (handFlow - airFlow);
            }
        }
        const double valveWas = valve;
        valve = airValve ? Min(1.0, valve + valveOpen) : Max(0.0, valve - valveClose);
        if (valve >= 1.0 && valveWas < 1.0) StartKnock(kButton, airStroke * kButton.travel / kOpenSeconds, 1.0, true, false, kAirButtonPlace);
        if (valve <= 0.0 && valveWas > 0.0) StartKnock(kBassPallet, airStroke * kBassPallet.travel / kCloseSeconds, 1.0, true, false, kAirButtonPlace);
        // Suction on pull accelerates a closing pallet onto its seat; positive
        // chamber pressure on push cushions its landing. Zero pressure stays
        // exact at kCloseSeconds.
        const double seatScale = Clamp(1.0 - 0.20 * pressure / 1000.0, 0.75, 1.30);
        const double stepCloseRate = closeRate * seatScale;
        StepKeys(seatScale);
        double vented = 0.0, hiss = 0.0;
        if (valve > 0.0) {
            const double area = kAirValveArea * valve, p = Abs(pressure);
            const double a = 0.5 * kRho / (kCd * kCd * area * area), b = kLaminar / area;
            // Never more than empties the bellows in one step, which only
            // matters at very low host rates.
            const double q = Min(2.0 * p / (b + Sqrt(b * b + 4.0 * a * p)), p / (dt * bellowsStiffness));
            vented = pressure >= 0.0 ? q : -q;
            // Two one-pole stages at the eddies' rate shape white noise, scaled
            // back to unit variance.
            const double w = q / (kCd * area) / kValveEddy * dt, c = w / (1.0 + w);
            noise = noise * 1664525u + 1013904223u;
            valveNoiseA += c * (1.7320508075688772 * Unit(noise) - valveNoiseA);
            valveNoiseB += c * (valveNoiseA - valveNoiseB);
            const double fluctuation = kValveBreath * q * valveNoiseB / Sqrt(Max(CascadeVariance(c), 1.0e-30));
            hiss = (fluctuation - valveSound) * rate;
            valveSound = fluctuation;
        } else if (valveSound != 0.0) {
            hiss = -valveSound * rate;
            valveSound = valveNoiseA = valveNoiseB = 0.0;
        }
        const double leak = kBellowsLeak * pressure + vented;
        const double pressureWas = pressure;
        pressure = Clamp(pressure + dt * bellowsStiffness * (handFlow - airFlow - leak), -kMaxPressure, kMaxPressure);
        // On a push-to-pull reversal the left wrist shifts from pressing the
        // bass board to drawing the leather bass strap, taking up its slack
        // at the bellows end's speed: a few mm/s to a few cm/s, so soft
        // (Mechanics.h). It was taken from the hand's speed in wheel ranges
        // a second as though it were m/s, 6 times too hard at the default
        // Bellows and independent of it.
        if (pressure > kPleatHold) strapPushed = true;
        else if (strapPushed && pressure < -kPleatHold) {
            const double slack = BellowsEndSpeed(litres, hand.velocity);
            if (slack > kFoldsStick) {
                strapPushed = false;
                StartKnock(kBassPallet, slack, 1.0, true, false, kStrapPlace);
            }
        }
        bend2 += bendSmoothing * (bend2Target - bend2);

        // Below about 1 m/s the gap flow is laminar (ReedModel.h, kLaminar),
        // which also keeps the eddies' rate finite as the bellows pressure
        // passes zero.
        const double jetStep = Max(Sqrt(2.0 * Abs(pressure) / kRho), 1.0) * dtIn;
        const bool pushing = pressure >= 0.0;
        float t[kMaxOversample] = {}, b[kMaxOversample] = {};
        double drawn = 0.0, open = 0.0;
        const int active = liveCount;
        int held = 0;
        // One cell's steps are a serial chain of divisions and square roots,
        // so the cells take each internal step together, where the processor
        // can overlap them. Each cell's turbulence draws and its share of the
        // bellows' air are taken in the order of one cell's steps after
        // another, so the sound is the same to the bit.
        double ec[kMaxCells], breath[kMaxCells], eddyForce[kMaxCells], travel[kMaxCells];
        double draws[kMaxCells * kMaxOversample], taken[kMaxCells * kMaxOversample];
        for (int j = 0; j < active; ++j) {
            Cell& c = *live[j];
            if (c.held) ++held;
            const CellSpec& spec = c.spec;
            travel[j] = c.held ? c.depth : 0.0;
            // The breath's two one-pole stages at the eddies' rate in the
            // flowing reed's gap, scaled back to the white draw's variance; once
            // a host sample, some 20 times a cycle of the highest reed.
            const ReedSpec& flowing = pushing ? spec.push : spec.pull;
            const double tip = GapArea(flowing, (pushing ? c.state.push : c.state.pull).x) / flowing.edge;
            const double eddy = jetStep / (flowing.eddyShare * tip);
            ec[j] = eddy / (1.0 + eddy);
            const double invEddyRms = 1.0 / Sqrt(Max(CascadeVariance(ec[j]), 1.0e-30));
            breath[j] = flowing.breath * Sqrt(tip) * invEddyRms;
            eddyForce[j] = kEddyForceShare * invEddyRms;
            // Turbulence in the gap: a small random force that also seeds the
            // reeds, and the fluctuation of the jet's flow.
            for (int k = 0; k < oversample; ++k) {
                noise = noise * 1664525u + 1013904223u;
                draws[j * oversample + k] = Unit(noise);
            }
        }
        // With no reed sounding the internal steps have nothing to do.
        for (int k = 0; active > 0 && k < oversample; ++k) {
            // Ranks opened by one key share a single lifted pallet pad and
            // couple through the mutual acoustic radiation inertance at the
            // pallet mouth. Sum each key group's previous-step volume
            // acceleration (~2 mm acoustic transit at 192 kHz) so sibling
            // ranks hear each other symmetrically regardless of rank order.
            // Every group index is below groupCount, so only those are zeroed.
            double palletRad[kMaxCells];
            for (int g = 0; g < groupCount; ++g) palletRad[g] = 0.0;
            for (int j = 0; j < active; ++j) palletRad[groupOf[j]] += live[j]->radiated;
            // Ranks opened by one key usually share exactly the same travel.
            // Reuse only that identical raised-cosine result; each chamber
            // still computes its own vent law and air/reed state. A newly
            // selected rank or unequal travel automatically misses the cache.
            // Local to this internal sample: no changed timing or approximation.
            double lastTravel = -1.0, lastLift = 0.0;
            for (int j = 0; j < active; ++j) {
                Cell& c = *live[j];
                const CellSpec& spec = c.spec;
                const bool palletMoving = c.open != travel[j];
                if (palletMoving) {
                    c.open = c.open < travel[j] ? Min(travel[j], c.open + c.openRate) : Max(travel[j], c.open - stepCloseRate * c.closeStroke);
                    // The pallet lifts off and settles smoothly: the hole area
                    // follows a raised cosine of the key travel, so the jet
                    // starts and stops without a step in the radiated flow.
                    if (c.open == lastTravel) c.lift = lastLift;
                    else {
                        c.lift = c.open >= 1.0 ? 1.0 : c.open <= 0.0 ? 0.0 : 0.5 - 0.5 * std::cos(kPi * c.open);
                        lastTravel = c.open;
                        lastLift = c.lift;
                    }
                    c.vent = MakeVent(spec.chamber, c.lift, invDtIn);
                }
                const double draw = draws[j * oversample + k];
                c.breathA += ec[j] * (draw - c.breathA);
                c.breathB += ec[j] * (c.breathA - c.breathB);
                // While the pallet pad is lifting or seating, the narrow
                // crescent rim orifice (peaking at c.lift = 0.5) adds starting
                // jet shear ("chiff") at both the pallet rim and the reed slot
                // before the reed's periodic oscillation locks the flow.
                const double palletPulse = palletMoving ? 4.0 * c.lift * (1.0 - c.lift) : 0.0;
                const double palletChiff = 1.0 + kPalletChiff * palletPulse;
                const double reedArea = (pushing ? spec.push : spec.pull).area;
                const double force = turbulence * (draw + palletChiff * eddyForce[j] * c.breathB) * reedArea * Abs(pressure);
                const double ventNoise = kPalletVentChiff * palletPulse * breath[j] * c.breathB;
                const double siblingRad = palletRad[groupOf[j]] - c.radiated;
                const double palletPressure = Clamp(kPalletCoupling * spec.chamber.endInertance * siblingRad,
                                                    -0.25 * kMaxPressure, 0.25 * kMaxPressure);
                double& share = taken[j * oversample + k];
                share = 0.0;
                const double out = StepCell(spec.push, spec.pull, spec.chamber, c.vent, c.state, pressure, dtIn,
                                            invDtIn, pushing ? force : 0.0, pushing ? 0.0 : force, share, bend2,
                                            breath[j] * palletChiff * c.breathB, palletPressure, ventNoise);
                c.radiated = out;
                const float o = static_cast<float>(out * spec.voiceGain);
                (spec.bass ? b : t)[k] += o;
            }
        }
        int nextLiveCount = 0;
        for (int j = 0; j < active; ++j) {
            Cell& c = *live[j];
            const CellSpec& spec = c.spec;
            for (int k = 0; k < oversample; ++k) drawn += taken[j * oversample + k];
            // The push valve lifts with the push air, and slaps shut when the
            // bellows turn or, as the pallet shuts, the chamber fills; the pull
            // valve on the bellows side does the converse on pull strokes.
            const double drop = pressure * c.lift;
            if (!c.valveOpen) c.valveOpen = drop > kFlapHold;
            else if (drop < -kFlapHold || (c.lift < 1.0 && drop < kFlapHold)) {
                c.valveOpen = false;
                StartSlap(c.valveMass, ValveLanding(c, false, drop < -kFlapHold, (pressureWas - pressure) / dt), spec.bass);
            }
            if (!c.pullValveOpen) c.pullValveOpen = drop < -kFlapHold;
            else if (drop > kFlapHold || (c.lift < 1.0 && drop > -kFlapHold)) {
                c.pullValveOpen = false;
                StartSlap(c.pullValveMass, ValveLanding(c, true, drop > kFlapHold, (pressure - pressureWas) / dt), spec.bass);
            }
            // Assist counts a held key in full once its pallet is down, however
            // far channel pressure holds it: a half-open pallet passes 0.3-0.9
            // of the open air, not its share of the opening, and counting the
            // opening took the hand's air away, so half-pressed low notes died.
            c.settled = c.settled || (c.held && c.open == travel[j]);
            double share = 1.0;
            if (!c.held || !c.settled) {
                const double full = c.depth >= 1.0 ? 1.0 : 0.5 - 0.5 * std::cos(kPi * c.depth);
                share = Min(1.0, c.lift / full);
            }
            open += share * spec.airWeight;
            if (!c.held && c.open <= 0.0) {
                c.closedFor += dt;
                if (c.closedFor > kQuietSeconds) c.active = false;
            }
            // Removal takes effect on the next sample, just as the former
            // per-sample scan did. Compaction never changes the live order.
            if (c.active) {
                groupOf[nextLiveCount] = groupOf[j];
                live[nextLiveCount++] = &c;
            }
        }
        liveCount = nextLiveCount;
        sounding = active;
        heldCells = held;
        demand = open;
        airFlow = drawn / oversample;
        airFlowSmoothed += 0.002 * (airFlow - airFlowSmoothed);

        mechanicsGain += smoothing * (mechanicsGainTarget - mechanicsGain);
        grilleCoeff += smoothing * (grilleCoeffTarget - grilleCoeff);
        double valveTreble = 0.0, valveBass = 0.0;
        int used = 0;
        for (int n = 0; n < slapsUsed; ++n) {
            Knock& k = slaps[static_cast<std::size_t>(n)];
            if (!k.active) continue;
            k.Step(dt);
            (k.bass ? valveBass : valveTreble) += k.force / kFlapDensity;
            if (k.active) used = n + 1;
        }
        slapsUsed = used;
        const double valves = mechanicsGain * (kValveVoicing / kForceToFlow);
        grille += grilleCoeff * (trebleSide.Process(t, stages) + valves * valveTreble - grille);
        gain += smoothing * (gainTarget - gain);
        bassGain += smoothing * (bassGainTarget - bassGain);
        trebleGain += smoothing * (trebleGainTarget - trebleGain);
        // Width glides like the other levels, so automating it never steps.
        treblePanL += glide * (treblePanLTarget - treblePanL);
        treblePanR += glide * (treblePanRTarget - treblePanR);
        bassPanL += glide * (bassPanLTarget - bassPanL);
        bassPanR += glide * (bassPanRTarget - bassPanR);
        foldsPanL += glide * (foldsPanLTarget - foldsPanL);
        foldsPanR += glide * (foldsPanRTarget - foldsPanR);
        double knockTreble[kKeyBedModes] {}, knockBass[kKeyBedModes] {};
        bool trebleLands = false, bassLands = false;
        used = 0;
        for (int n = 0; n < knocksUsed; ++n) {
            Knock& k = knocks[static_cast<std::size_t>(n)];
            if (!k.active) continue;
            const double change = k.Step(dt) * rate;
            double* drive = k.bass ? knockBass : knockTreble;
            for (int m = 0; m < kKeyBedModes; ++m) drive[m] += k.coupling[m] * change;
            (k.bass ? bassLands : trebleLands) = true;
            if (k.active) used = n + 1;
        }
        knocksUsed = used;
        // The folds sit between the halves and radiate straight to the air, not
        // through the body, as panels of their own size (kPleatToFlow).
        const double flips = folds.Step(pressure, dt);
        // The folds rub both as air leaves the bellows and as the hand's stroke
        // distends or compresses the fold walls (kFoldDistensionShare), with
        // higher hinge friction when pleats crowd near closed or pull taut near
        // full extension.
        const double extension = Extension();
        const double foldStrain = 1.0 + 0.35 * (2.0 * extension - 1.0) * (2.0 * extension - 1.0);
        const double foldFlow = (1.0 - kFoldDistensionShare) * (airFlow + leak) + kFoldDistensionShare * handFlow;
        const double rubbing = rustle.Step(foldStrain * foldFlow / kBellowsSection);
        const double foldsOut = (kFoldVoicing * flips + kRustleVoicing * (rubbing - rustleForce)) * rate
                                * (kPleatToFlow / kForceToFlow);
        rustleForce = rubbing;
        // The air valve and the bass mechanism sit in the bass box too: their
        // sound leaves through the bass end cover's holes with the bass reeds'.
        const double bass = bassBox.Process(bassSide.Process(b, stages) + hiss + valves * valveBass
                                            + mechanicsGain * keyBedBass.Process(knockBass, bassLands)) * bassGain;
        const double treble = grille * trebleGain + mechanicsGain * keyBedTreble.Process(knockTreble, trebleLands);
        double l = gain * (treble * treblePanL + mechanicsGain * foldsOut * foldsPanL + bass * bassPanL);
        double r = gain * (treble * treblePanR + mechanicsGain * foldsOut * foldsPanR + bass * bassPanR);
        if (roomGain != 0.0 || roomGainTarget != 0.0) {
            roomGain += glide * (roomGainTarget - roomGain);
            if (Abs(roomGain - roomGainTarget) < 1.0e-6) roomGain = roomGainTarget;
            double roomL, roomR;
            room.Process(l, r, roomL, roomR);
            l += roomGain * roomL;
            r += roomGain * roomR;
            // Room turned off: start the next room from silence, not from
            // the tail it held.
            if (roomGain == 0.0) room.Clear();
        }
        // Room is the surrounding field. Only the direct source paths take
        // the relative near-field microphone colour caused by Life movement.
        l += gain * (treblePanL * trebleMicMotion[0].Correction(treble, smoothing)
                   + bassPanL * bassMicMotion[0].Correction(bass, smoothing));
        r += gain * (treblePanR * trebleMicMotion[1].Correction(treble, smoothing)
                   + bassPanR * bassMicMotion[1].Correction(bass, smoothing));
        std::array<float, 2> frame { { static_cast<float>(l), static_cast<float>(r) } };
        output.Process(frame);
        left[i] = frame[0];
        right[i] = frame[1];
    }
    // The mechanics' bodies, the bass box and the microphones' integrals sit
    // silent through most of a phrase (no knock, no bass note, no movement),
    // their states decaying into subnormals, which the processor computes
    // tens of times slower. Each is cleared only once all its states are
    // below 1e-30 (-600 dB), so one still sounding is never touched.
    bassBox.FlushTiny();
    for (auto& mic : trebleMicMotion) mic.FlushTiny();
    for (auto& mic : bassMicMotion) mic.FlushTiny();
    for (KeyBed* bed : { &keyBedTreble, &keyBedBass }) bed->FlushTiny();
    // The air meter's smoothing falls for ever once the reeds stop.
    if (Abs(airFlowSmoothed) < 1.0e-30) airFlowSmoothed = 0.0;
    if (sounding == 0) {
        trebleSide.FlushTiny();
        bassSide.FlushTiny();
        room.FlushTiny();
        if (Abs(grille) < 1.0e-30) grille = 0.0;
    }
    diag.hand = hand.position;
    diag.handVelocity = hand.velocity;
    diag.pressure = pressure;
    diag.airFlow = airFlowSmoothed * 1.0e3;
    diag.cells = sounding;
}

} // namespace respira
