#include "RespiraEngine.h"

#include "ReedTuning.h"

#include <cmath>
#include <initializer_list>

namespace respira {

namespace {

// Pa per m^3 of air the hand moves but the reeds do not take. The air in a
// ~6 L bellows alone is 2.4e7; the folds and the player's arm give a few
// times more, which keeps the pressure from collapsing whenever a chord opens.
constexpr double kBellowsStiffness = 4.0e6;
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
// The radiated sound is the pallet flow's rate of change as from a small
// source 1 m away; 1 Pa there (94 dB SPL) is digital full scale, so the notes
// of the demo phrases read -9 on Reason's VU meter (the louder channel's RMS
// over 300 ms, +12 dB) at their median (Tests/EngineTests.cpp, TypicalLevel). A bass, a chord and a triad at the arm's full pressure are
// some 23 dB louder; the output stage below keeps them out of the red. The
// factory presets set their Volume so each sits within 1 dB of Init on a
// moderate stroke (PresetsSitTogether).
constexpr double kPascalToFullScale = 1.0;
// Each register's trim on the treble side, so switching one does not jump the
// level: on Init's controls and the reference stroke (Tests/EngineTests.cpp,
// PresetsSitTogether), a C3, a C4-E4-G4 triad, a G4, a C5 and a C6 average the
// Musette register's RMS. More reeds are louder on a real accordion too (the
// Master register 1.9 dB, one reed 3.5-3.9 dB); the owner asked for registers
// that change the colour, not the level (RegistersSitTogether).
constexpr std::array<double, 8> kRegisterTrimDb { { 3.45, 3.93, 1.39, 0.0, 1.46, 1.25, 1.68, -1.91 } };
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
// c) sin kd); the lumped box is within 2 dB of it up to 2 kHz. Above about 2.5
// kHz the layer rings at its depth resonances, which the box's irregular
// depth and the absorption inside it damp by amounts not known here, and the
// lumped box overstates the loss (README, Known gaps). The treble
// compartment under the grille is no resonator: 35 mm deep under a grille 30 %
// open with 3.6 mm slots, its lumped resonance would fall at k depth = 1.7,
// and as a layer, averaged over its plausible depths (25-45 mm), it passes the
// reeds within 3 dB up to 16 kHz.
constexpr double kBassBoxDepth = 0.04, kBassBoxOpen = 0.10, kBassHoleLength = 0.0081, kClothRayl = 30.0;
constexpr double kOpenSeconds = 0.012, kOpenVelocity = 0.009;  // pallet lift: 12 ms soft .. 3 ms hard
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
// eddies, about the gap's 0.3 mm: flat up to their rate U / (2 pi l), with U
// the jet's speed sqrt(2 p / rho), and falling above it, as the air valve's
// does; about 15 kHz at 500 Pa. White noise per internal step would spread
// three quarters of the estimated fluctuation above 24 kHz, where no one
// hears it.
constexpr double kGapEddy = 0.3e-3;         // m

double DbToGain(double db) { return std::pow(10.0, db / 20.0); }

// Rank of the bass side a group sounds: channel 2 bass buttons, channel 3 chords.
enum Group { kTreble, kBassButtons, kChordButtons };

// Whether a register sounds `rank` for a (folded) treble key. Broken back
// above C#7, the 4' would double the 8' in unison, a cent or so apart, and
// the two would beat slowly down to near silence; where the 8' is open the
// 4' stays shut there instead.
bool Sounds(const Register& reg, int rank, int key) {
    if (!reg.ranks[static_cast<std::size_t>(rank)]) return false;
    return rank != kRankH || key + 12 <= kHighestReed || !reg.ranks[kRankM];
}

// Player's view: the bass side on the left, the treble keys spread across the
// right from the middle register up.
void TreblePan(int tableKey, double width, float& left, float& right) {
    const double pan = width * Clamp(0.55 + (tableKey - 72) / 72.0, -0.2, 1.0);
    left = static_cast<float>(std::cos((pan + 1.0) * kPi / 4.0));
    right = static_cast<float>(std::sin((pan + 1.0) * kPi / 4.0));
}

} // namespace

// Per unit of cover area the holes' air mass is rho l' / open and the
// compartment's compliance depth / (rho c^2), so f = c / (2 pi) sqrt(open /
// (depth l')). The cover is large against the wavelength there, so radiation
// loads it with rho c, and the cloth behind it adds its flow resistance:
// Q = sqrt(mass / compliance) / (rho c + cloth).
void Engine::Compartment::Set(double depth, double open, double neck, double rayl, double rate) {
    const double mass = kRho * neck / open, compliance = depth / kRhoC2;
    const double hz = 1.0 / (2.0 * kPi * Sqrt(mass * compliance)), q = Sqrt(mass / compliance) / (Sqrt(kRho * kRhoC2) + rayl);
    const double k = std::tan(kPi * Min(hz, 0.45 * rate) / rate), norm = 1.0 / (1.0 + k / q + k * k);
    b0 = b2 = k * k * norm;
    b1 = 2.0 * b0;
    a1 = 2.0 * (k * k - 1.0) * norm;
    a2 = (1.0 - k / q + k * k) * norm;
}

void Engine::Compartment::FlushTiny() {
    if (Abs(z1) < 1.0e-20) z1 = 0.0;
    if (Abs(z2) < 1.0e-20) z2 = 0.0;
}

void Engine::Hand::Tick(double dt, double momentum) {
    if (!placed) {
        if (!received) return;
        wheel = position = lastTarget = target;
        placed = true;
    }
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
    if (momentum <= 0.0) {
        position = wheel;
        velocity = wheelVelocity;
        return;
    }
    // Semi-implicit Euler, kept well inside its stable range (w dt <= 0.5)
    // for a Momentum shorter than two samples.
    const double w = Min(1.0e3 / momentum, 0.5 / dt);
    velocity += dt * w * (w * (wheel - position) - 2.0 * velocity);
    position = Clamp(position + dt * velocity, 0.0, 1.0);
}

Engine::Engine() { Prepare(48000.0); }

void Engine::Prepare(double sampleRate) {
    rate = sampleRate > 1000.0 && sampleRate < 1.0e6 ? sampleRate : 48000.0;
    bassBox.Set(kBassBoxDepth, kBassBoxOpen, kBassHoleLength, kClothRayl, rate);
    for (BodyHighPass* body : { &mechanicsL, &mechanicsR, &mechanicsBass }) body->Set(rate);
    smoothing = 1.0 - std::exp(-1.0 / (0.02 * rate));
    bendSmoothing = 1.0 - std::exp(-1.0 / (0.005 * rate));
    output.Prepare(rate, kBellowsGuard);
    SetControls(controls);
    Reset();
}

void Engine::Reset() {
    for (auto& c : cells) c = Cell {};
    hand = Hand {};
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
    trebleLeft.Reset();
    trebleRight.Reset();
    bassSide.Reset();
    bassBox.Clear();
    grilleL = grilleR = 0.0;
    gain = gainTarget;
    bassGain = bassGainTarget;
    trebleGain = trebleGainTarget;
    bassPanL = bassPanLTarget;
    bassPanR = bassPanRTarget;
    output.Reset();
    keys.fill(Key {});
    movingCount = 0;
    knocks.fill(Knock {});
    for (BodyHighPass* body : { &mechanicsL, &mechanicsR, &mechanicsBass }) body->Clear();
    mechanicsGain = mechanicsGainTarget;
    diag = Diagnostics {};
}

void Engine::SetControls(const Controls& next) {
    for (int i = 0; i < kControlCount; ++i) {
        const ControlInfo& info = kControlInfo[static_cast<std::size_t>(i)];
        const double v = next[i];
        controls[i] = v == v ? Clamp(v, info.min, info.max) : info.initial;
        if (info.steps > 0) controls[i] = std::floor(controls[i] + 0.5);
    }
    grilleCoeff = 1.0 - std::exp(-2.0 * kPi * Min(controls[kGrille], 0.45 * rate) / rate);
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
    const int reg = static_cast<int>(controls[kRegister]);
    if (reg != registerIndex) {
        SwitchRegister(registerIndex, reg);
        StartKnock(kRegisterSwitch, kRegisterSwitch.travel / kOpenSeconds, 1.0, false, 72);
    }
    registerIndex = reg;
    // The gap turbulence's force on the tongues is white per internal step; it
    // scales with the square root of the step rate so its spectrum, the noise
    // the reeds hear, stays the same at every rate and Oversampling. The flow's
    // fluctuation, the noise they make, is shaped to its eddies (kGapEddy).
    noiseScale = Sqrt(internalRate / kNoiseRate);
    turbulence = kTurbulence * noiseScale;
    gainTarget = kRho / (4.0 * kPi) * kPascalToFullScale * DbToGain(controls[kVolume]);
    bassGainTarget = DbToGain(controls[kBass]);
    trebleGainTarget = DbToGain(kRegisterTrimDb[static_cast<std::size_t>(reg)]);
    mechanicsGainTarget = kForceToFlow * DbToGain(controls[kMechanics]);
    const double bassPan = -0.9 * controls[kWidth];
    bassPanLTarget = static_cast<float>(std::cos((bassPan + 1.0) * kPi / 4.0));
    bassPanRTarget = static_cast<float>(std::sin((bassPan + 1.0) * kPi / 4.0));
    for (auto& c : cells)
        if (c.active) UpdatePan(c);
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

void Engine::UpdatePan(Cell& c) const {
    if (!c.spec.bass) TreblePan(c.reed % 128, controls[kWidth], c.gainLTarget, c.gainRTarget);
}

void Engine::MoveKey(int source) {
    Key& k = keys[static_cast<std::size_t>(source)];
    if (k.listed || k.travel == k.target) return;
    k.listed = true;
    movingKeys[static_cast<std::size_t>(movingCount++)] = static_cast<std::uint16_t>(source);
}

// A part landing on its stop. When every knock is still sounding, a new one
// in that crowd is dropped.
void Engine::StartKnock(const Stop& stop, double speed, double massScale, bool bass, int tableKey) {
    for (auto& k : knocks) {
        if (k.active) continue;
        k.Start(stop, speed, massScale);
        k.bass = bass;
        if (!bass) TreblePan(tableKey, controls[kWidth], k.panL, k.panR);
        return;
    }
}

// The keys move as their pallets do (Render): down at the stroke's speed,
// up in the time the spring takes. A key knocks on its felt when it reaches
// the bottom and its pallet on its seat when it is back up; a key that turns
// before either does not. Each key's felt and leverage differ a little from
// its neighbours' (mass +-20 %, fixed per instrument, as the reeds differ).
void Engine::StepKeys() {
    const double up = 1.0 / (kCloseSeconds * rate);
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
            k.travel = Max(k.target, k.travel - up);
            if (k.travel <= 0.0) {
                stop = k.button ? &kBassPallet : &kTreblePallet;
                speed = stop->travel * up * rate;
            }
        }
        if (stop) StartKnock(*stop, speed, 1.0 + 0.2 * Unit(Hash(2u, static_cast<std::uint32_t>(source))), k.button, TrebleKey(source % 128));
        if (k.travel == k.target) {
            k.listed = false;
            movingKeys[static_cast<std::size_t>(i)] = movingKeys[static_cast<std::size_t>(--movingCount)];
        } else {
            ++i;
        }
    }
}

void Engine::StartCell(int rank, int key, int source, int velocity) {
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
        // Musette and master tune changes reach the reeds at their next note.
        cell->spec = MakeCell(rank, key, controls[kMusette], kReedTuning[rank][index], rate * kBaseOversample);
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
        cell->vent = Vent {};
        cell->reed = reed;
        cell->rank = rank;
        UpdatePan(*cell);
        cell->gainL = cell->gainLTarget;
        cell->gainR = cell->gainRTarget;
    }
    cell->active = cell->held = true;
    cell->source = source;
    cell->order = ++order;
    cell->closedFor = 0.0;
    cell->depth = channelDepth[static_cast<std::size_t>(source / 128)];
    cell->settled = false;
    // A sharper key stroke lifts the pallet faster.
    cell->openRate = 1.0 / ((kOpenSeconds - kOpenVelocity * velocity / 127.0) * internalRate);
}

void Engine::NoteOn(int channel, int key, int velocity) {
    Group group = channel == 1 ? kBassButtons : channel == 2 ? kChordButtons : kTreble;
    if (group == kTreble && controls[kKeyboard] < 0.5) {
        if (key < 36) group = kBassButtons;
        else if (key < 48) group = kChordButtons;
    }
    const int source = channel * 128 + key;
    Key& k = keys[static_cast<std::size_t>(source)];
    k.target = channelDepth[static_cast<std::size_t>(channel)];
    k.rate = 1.0 / ((kOpenSeconds - kOpenVelocity * velocity / 127.0) * rate);
    k.button = group != kTreble;
    MoveKey(source);
    if (group == kBassButtons) {
        StartCell(kRankBass16, PitchClass(key), source, velocity);
        StartCell(kRankBass8, PitchClass(key), source, velocity);
    } else if (group == kChordButtons) {
        StartCell(kRankChord8, PitchClass(key), source, velocity);
        StartCell(kRankChord4, PitchClass(key), source, velocity);
    } else {
        const Register& reg = kRegisters[static_cast<std::size_t>(controls[kRegister])];
        for (int rank = kRankL; rank <= kRankH; ++rank)
            if (Sounds(reg, rank, TrebleKey(key))) StartCell(rank, TrebleKey(key), source, velocity);
        trebleHeld[static_cast<std::size_t>(key)] = static_cast<std::uint8_t>(velocity);
    }
}

// The register switches move slides over the treble reed blocks, so a switch
// pressed while keys are held reaches them: the ranks it opens start to speak
// and the ones it shuts fall silent. A slide opens about as fast as the
// softest key stroke lifts a pallet.
void Engine::SwitchRegister(int from, int to) {
    const Register& before = kRegisters[static_cast<std::size_t>(from)];
    const Register& after = kRegisters[static_cast<std::size_t>(to)];
    for (int key = 0; key < 128; ++key) {
        if (trebleHeld[static_cast<std::size_t>(key)] == 0) continue;
        for (int rank = kRankL; rank <= kRankH; ++rank) {
            const bool was = Sounds(before, rank, TrebleKey(key)), is = Sounds(after, rank, TrebleKey(key));
            if (is && !was) StartCell(rank, TrebleKey(key), key, 0);
            else if (was && !is)
                for (auto& c : cells)
                    if (c.active && c.held && c.source == key && c.rank == rank) c.held = false;
        }
    }
}

void Engine::NoteOff(int channel, int key) {
    const int source = channel * 128 + key;
    for (auto& c : cells)
        if (c.active && c.held && c.source == source) c.held = false;
    if (channel == 0) trebleHeld[static_cast<std::size_t>(key)] = 0;
    keys[static_cast<std::size_t>(source)].target = 0.0;
    MoveKey(source);
}

void Engine::AllNotesOff() {
    for (auto& c : cells) c.held = false;
    trebleHeld.fill(0);
    for (int source = 0; source < 3 * 128; ++source) {
        keys[static_cast<std::size_t>(source)].target = 0.0;
        MoveKey(source);
    }
}

void Engine::Midi(const std::uint8_t* bytes, int size) {
    if (!bytes || size < 2) return;
    const int type = bytes[0] & 0xF0, channel = bytes[0] & 0x0F;
    const int d1 = bytes[1] & 0x7F, d2 = size > 2 ? bytes[2] & 0x7F : 0;
    const int ch = channel <= 2 ? channel : 0;
    if (type == 0x90 && d2 > 0) NoteOn(ch, d1, d2);
    else if (type == 0x80 || type == 0x90) NoteOff(ch, d1);
    else if (type == 0xB0) {
        if (d1 == 1) { msb = d2; lsb = 0; }
        else if (d1 == 33) { lsb = d2; fineWheel = true; }
        else if (d1 == 64) airPedal = d2 >= 64;
        else if (d1 == 120 || d1 == 123) AllNotesOff();
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
    // The valve opens and closes in the time a pallet takes.
    const double valveOpen = dt / kOpenSeconds, valveClose = dt / kCloseSeconds;
    for (int i = 0; i < frames; ++i) {
        hand.Tick(dt, controls[kMomentum]);
        // Assist makes the hand's stroke move as much more air as the open
        // reeds take beyond one A4 reed, so one wheel speed gives one pressure
        // whatever is played; at 0 the hand moves the bellows' own air only.
        const double handFlow = -litres * hand.velocity * (1.0 + assist * (Max(demand, 1.0) - 1.0));
        const double valveWas = valve;
        valve = airValve ? Min(1.0, valve + valveOpen) : Max(0.0, valve - valveClose);
        if (valve >= 1.0 && valveWas < 1.0) StartKnock(kButton, kButton.travel / kOpenSeconds, 1.0, true, 0);
        if (valve <= 0.0 && valveWas > 0.0) StartKnock(kBassPallet, kBassPallet.travel / kCloseSeconds, 1.0, true, 0);
        StepKeys();
        double vented = 0.0, hiss = 0.0;
        if (valve > 0.0) {
            const double area = kAirValveArea * valve, p = Abs(pressure);
            const double a = 0.5 * kRho / (kCd * kCd * area * area), b = kLaminar / area;
            // Never more than empties the bellows in one step, which only
            // matters at very low host rates.
            const double q = Min(2.0 * p / (b + Sqrt(b * b + 4.0 * a * p)), p / (dt * kBellowsStiffness));
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
        pressure = Clamp(pressure + dt * kBellowsStiffness * (handFlow - airFlow - leak), -kMaxPressure, kMaxPressure);
        bend2 += bendSmoothing * (bend2Target - bend2);

        // Two one-pole stages at the eddies' rate, scaled back to the white
        // draw's variance. Below about 1 m/s the gap flow is laminar
        // (ReedModel.h, kLaminar), which also keeps the scale finite as the
        // bellows pressure passes zero.
        const double jet = Max(Sqrt(2.0 * Abs(pressure) / kRho), 1.0);
        const double eddy = jet / kGapEddy * dtIn, ec = eddy / (1.0 + eddy);
        const double breathScale = 1.0 / Sqrt(Max(CascadeVariance(ec), 1.0e-30));
        float tl[kMaxOversample] = {}, tr[kMaxOversample] = {}, b[kMaxOversample] = {};
        double drawn = 0.0, open = 0.0;
        int active = 0;
        for (auto& c : cells) {
            if (!c.active) continue;
            ++active;
            // Width glides like the other levels, so automating it never steps.
            c.gainL += glide * (c.gainLTarget - c.gainL);
            c.gainR += glide * (c.gainRTarget - c.gainR);
            const CellSpec& spec = c.spec;
            const double travel = c.held ? c.depth : 0.0;
            for (int k = 0; k < oversample; ++k) {
                if (c.open != travel) {
                    c.open = c.open < travel ? Min(travel, c.open + c.openRate) : Max(travel, c.open - closeRate);
                    // The pallet lifts off and settles smoothly: the hole area
                    // follows a raised cosine of the key travel, so the jet
                    // starts and stops without a step in the radiated flow.
                    c.lift = c.open >= 1.0 ? 1.0 : c.open <= 0.0 ? 0.0 : 0.5 - 0.5 * std::cos(kPi * c.open);
                    c.vent = MakeVent(spec.chamber, c.lift, invDtIn);
                }
                // Turbulence in the gap: a small random force that also seeds the
                // reeds, and the fluctuation of the jet's flow.
                noise = noise * 1664525u + 1013904223u;
                const double draw = Unit(noise);
                c.breathA += ec * (draw - c.breathA);
                c.breathB += ec * (c.breathA - c.breathB);
                const double pushForce = turbulence * draw * spec.push.area * Abs(pressure);
                const double out = StepCell(spec.push, spec.pull, spec.chamber, c.vent, c.state, pressure, dtIn,
                                            invDtIn, pushForce, -pushForce, drawn, bend2, breathScale * c.breathB);
                const float o = static_cast<float>(out);
                if (spec.bass) b[k] += o;
                else { tl[k] += o * c.gainL; tr[k] += o * c.gainR; }
            }
            // Assist counts a held key in full once its pallet is down, however
            // far channel pressure holds it: a half-open pallet passes 0.3-0.9
            // of the open air, not its share of the opening, and counting the
            // opening took the hand's air away, so half-pressed low notes died.
            c.settled = c.settled || (c.held && c.open == travel);
            const double full = c.depth >= 1.0 ? 1.0 : 0.5 - 0.5 * std::cos(kPi * c.depth);
            open += (c.held && c.settled ? 1.0 : Min(1.0, c.lift / full)) * spec.airWeight;
            if (!c.held && c.open <= 0.0) {
                c.closedFor += dt;
                if (c.closedFor > kQuietSeconds) c.active = false;
            }
        }
        sounding = active;
        demand = open;
        airFlow = drawn / oversample;
        airFlowSmoothed += 0.002 * (airFlow - airFlowSmoothed);

        grilleL += grilleCoeff * (trebleLeft.Process(tl, stages) - grilleL);
        grilleR += grilleCoeff * (trebleRight.Process(tr, stages) - grilleR);
        gain += smoothing * (gainTarget - gain);
        bassGain += smoothing * (bassGainTarget - bassGain);
        trebleGain += smoothing * (trebleGainTarget - trebleGain);
        bassPanL += glide * (bassPanLTarget - bassPanL);
        bassPanR += glide * (bassPanRTarget - bassPanR);
        double knockL = 0.0, knockR = 0.0, knockBass = 0.0;
        for (auto& k : knocks) {
            if (!k.active) continue;
            const double change = k.Step(dt) * rate;
            if (k.bass) knockBass += change;
            else {
                knockL += change * k.panL;
                knockR += change * k.panR;
            }
        }
        mechanicsGain += smoothing * (mechanicsGainTarget - mechanicsGain);
        // The air valve and the bass mechanism sit in the bass box too: their
        // sound leaves through the bass end cover's holes with the bass reeds'.
        const double bass = bassBox.Process(bassSide.Process(b, stages) + hiss + mechanicsGain * mechanicsBass.Process(knockBass)) * bassGain;
        const double l = gain * (grilleL * trebleGain + mechanicsGain * mechanicsL.Process(knockL) + bass * bassPanL);
        const double r = gain * (grilleR * trebleGain + mechanicsGain * mechanicsR.Process(knockR) + bass * bassPanR);
        std::array<float, 2> frame { { static_cast<float>(l), static_cast<float>(r) } };
        output.Process(frame);
        left[i] = frame[0];
        right[i] = frame[1];
    }
    if (sounding == 0) {
        trebleLeft.FlushTiny();
        trebleRight.FlushTiny();
        bassSide.FlushTiny();
        bassBox.FlushTiny();
        for (BodyHighPass* body : { &mechanicsL, &mechanicsR, &mechanicsBass }) body->FlushTiny();
        for (double* v : { &grilleL, &grilleR })
            if (Abs(*v) < 1.0e-20) *v = 0.0;
    }
    diag.hand = hand.position;
    diag.handVelocity = hand.velocity;
    diag.pressure = pressure;
    diag.airFlow = airFlowSmoothed * 1.0e3;
    diag.cells = sounding;
}

} // namespace respira
