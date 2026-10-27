#include "RespiraEngine.h"

#include "ReedTuning.h"

#include <cmath>
#include <initializer_list>

namespace respira {

namespace {

constexpr double kMaxPressure = 2500.0;     // what the player's arm can put on the bellows, Pa
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
// however hard the bellows are pushed. A compressor follows the louder
// channel's RMS over 20 ms and above -17 dBFS (-5 VU) brings it down 10:1
// over an 8 dB knee, so playing below about -9 VU passes untouched. Its gain
// drops at once and recovers over 300 ms, slower than a musette's beating, so
// the beats keep their depth. A peak limiter with no look-ahead holds what
// gets past at -1.2 dBFS and lets go over 50 ms. On HardStrokesFit's strokes
// the limiter takes at most 1.6 dB, on under 1 % of the samples; it never acts
// on the demo phrases.
constexpr double kThresholdDb = -17.0, kKneeDb = 8.0, kRatio = 10.0;
constexpr double kDetect = 0.02, kCompRelease = 0.3;
constexpr double kCeiling = 0.87, kLimitRelease = 0.05;
constexpr int kGainPeriod = 16;   // samples between gain computations, ramped across
constexpr double kBassLowpassHz = 2500.0;   // the bass reeds sound through the bass box's grille
constexpr double kOpenSeconds = 0.012, kOpenVelocity = 0.009;  // pallet lift: 12 ms soft .. 3 ms hard
constexpr double kCloseSeconds = 0.008;
constexpr double kQuietSeconds = 0.02;      // a closed chamber is let go after this
// Key travel at full channel pressure: a pallet opening of 0.2 (raised cosine
// of the travel), where the model's C4 falls 5.9 Hz per kPa of bellows
// pressure against the 5.1 Hz/kPa Okada measured with the pallet lifted 1 mm
// (Adachi et al., "Aerodynamic mechanism of accordion pitch bending", Forum
// Acusticum 2026). Fully open the model falls 1.5 Hz/kPa, measured 2.5.
constexpr double kHalfPressed = 0.29517;    // acos(1 - 2 * 0.2) / pi
constexpr double kTurbulence = 2.0e-3;      // gap turbulence force per Pa of bellows pressure and m^2
constexpr double kNoiseRate = 192000.0;     // internal rate the turbulence levels are set at

double DbToGain(double db) { return std::pow(10.0, db / 20.0); }

// Rank of the bass side a group sounds: channel 2 bass buttons, channel 3 chords.
enum Group { kTreble, kBassButtons, kChordButtons };

} // namespace

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
    bassCoeff = 1.0 - std::exp(-2.0 * kPi * kBassLowpassHz / rate);
    smoothing = 1.0 - std::exp(-1.0 / (0.02 * rate));
    bendSmoothing = 1.0 - std::exp(-1.0 / (0.005 * rate));
    detector = 1.0 - std::exp(-1.0 / (kDetect * rate));
    compRelease = 1.0 - std::exp(-kGainPeriod / (kCompRelease * rate));
    limitRelease = 1.0 - std::exp(-1.0 / (kLimitRelease * rate));
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
    grilleL = grilleR = bassLow = 0.0;
    gain = gainTarget;
    bassGain = bassGainTarget;
    trebleGain = trebleGainTarget;
    powerL = powerR = reduction = compStep = 0.0;
    compGain = limit = 1.0;
    untilGain = 0;
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
    if (reg != registerIndex) SwitchRegister(registerIndex, reg);
    registerIndex = reg;
    // The gap turbulence is white per internal step; it scales with the square
    // root of the step rate so its spectrum, and the noise the reeds make and
    // hear, stays the same at every rate and Oversampling.
    noiseScale = Sqrt(internalRate / kNoiseRate);
    turbulence = kTurbulence * noiseScale;
    gainTarget = kRho / (4.0 * kPi) * kPascalToFullScale * DbToGain(controls[kVolume]);
    bassGainTarget = DbToGain(controls[kBass]);
    trebleGainTarget = DbToGain(kRegisterTrimDb[static_cast<std::size_t>(reg)]);
    const double bassPan = -0.9 * controls[kWidth];
    bassPanL = static_cast<float>(std::cos((bassPan + 1.0) * kPi / 4.0));
    bassPanR = static_cast<float>(std::sin((bassPan + 1.0) * kPi / 4.0));
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
}

void Engine::UpdatePan(Cell& c) const {
    if (c.spec.bass) return;
    // Player's view: the bass side on the left, the treble keys spread across
    // the right from the middle register up.
    const int key = c.reed % 128;
    const double pan = controls[kWidth] * Clamp(0.55 + (key - 72) / 72.0, -0.2, 1.0);
    c.gainL = static_cast<float>(std::cos((pan + 1.0) * kPi / 4.0));
    c.gainR = static_cast<float>(std::sin((pan + 1.0) * kPi / 4.0));
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
        cell->spec = MakeCell(rank, key, controls[kMusette], kReedTuning[rank][index]);
        if (masterTune != 0.0) {
            const double scale = std::pow(2.0, masterTune / 1200.0);
            const ReedVariation pushV = Variation(rank, index, 0), pullV = Variation(rank, index, 1);
            const double q = ReedQ(cell->spec.hz);
            const double pushHz = Sqrt(cell->spec.push.omega2) / (2.0 * kPi) * scale;
            const double pullHz = Sqrt(cell->spec.pull.omega2) / (2.0 * kPi) * scale;
            cell->spec.push = MakeReed(pushHz, q * pushV.qScale, pushV.setScale);
            cell->spec.pull = MakeReed(pullHz, q * pullV.qScale, pullV.setScale);
        }
        cell->state = CellState {};
        cell->state.push.x = cell->spec.push.rest;
        cell->state.pull.x = cell->spec.pull.rest;
        // A closed chamber sits at bellows pressure: air leaks in or out through
        // the clearance of whichever reed the bellows drives.
        cell->state.pressure = pressure;
        cell->open = cell->lift = 0.0;
        cell->vent = Vent {};
        cell->reed = reed;
        cell->rank = rank;
        UpdatePan(*cell);
    }
    cell->active = cell->held = true;
    cell->source = source;
    cell->order = ++order;
    cell->closedFor = 0.0;
    cell->depth = channelDepth[static_cast<std::size_t>(source / 128)];
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
    if (group == kBassButtons) {
        StartCell(kRankBass16, PitchClass(key), source, velocity);
        StartCell(kRankBass8, PitchClass(key), source, velocity);
    } else if (group == kChordButtons) {
        StartCell(kRankChord8, PitchClass(key), source, velocity);
        StartCell(kRankChord4, PitchClass(key), source, velocity);
    } else {
        const Register& reg = kRegisters[static_cast<std::size_t>(controls[kRegister])];
        for (int rank = kRankL; rank <= kRankH; ++rank)
            if (reg.ranks[static_cast<std::size_t>(rank)]) StartCell(rank, TrebleKey(key), source, velocity);
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
            const bool was = before.ranks[static_cast<std::size_t>(rank)], is = after.ranks[static_cast<std::size_t>(rank)];
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
}

void Engine::AllNotesOff() {
    for (auto& c : cells) c.held = false;
    trebleHeld.fill(0);
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
        valve = airValve ? Min(1.0, valve + valveOpen) : Max(0.0, valve - valveClose);
        double vented = 0.0, hiss = 0.0;
        if (valve > 0.0) {
            const double area = kAirValveArea * valve, p = Abs(pressure);
            const double a = 0.5 * kRho / (kCd * kCd * area * area), b = kLaminar / area;
            // Never more than empties the bellows in one step, which only
            // matters at very low host rates.
            const double q = Min(2.0 * p / (b + Sqrt(b * b + 4.0 * a * p)), p / (dt * kBellowsStiffness));
            vented = pressure >= 0.0 ? q : -q;
            // Two one-pole stages at the eddies' rate shape white noise, scaled
            // back to unit variance (the cascade's c^4 (1 + a^2) / (1 - a^2)^3).
            const double w = q / (kCd * area) / kValveEddy * dt, c = w / (1.0 + w), r = 1.0 - c;
            noise = noise * 1664525u + 1013904223u;
            valveNoiseA += c * (1.7320508075688772 * Unit(noise) - valveNoiseA);
            valveNoiseB += c * (valveNoiseA - valveNoiseB);
            const double variance = c * c * c * c * (1.0 + r * r) / ((1.0 - r * r) * (1.0 - r * r) * (1.0 - r * r));
            const double fluctuation = kValveBreath * q * valveNoiseB / Sqrt(Max(variance, 1.0e-30));
            hiss = (fluctuation - valveSound) * rate;
            valveSound = fluctuation;
        } else if (valveSound != 0.0) {
            hiss = -valveSound * rate;
            valveSound = valveNoiseA = valveNoiseB = 0.0;
        }
        const double leak = kBellowsLeak * pressure + vented;
        pressure = Clamp(pressure + dt * kBellowsStiffness * (handFlow - airFlow - leak), -kMaxPressure, kMaxPressure);
        bend2 += bendSmoothing * (bend2Target - bend2);

        float tl[kMaxOversample] = {}, tr[kMaxOversample] = {}, b[kMaxOversample] = {};
        double drawn = 0.0, open = 0.0;
        int active = 0;
        for (auto& c : cells) {
            if (!c.active) continue;
            ++active;
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
                const double pushForce = turbulence * draw * spec.push.area * Abs(pressure);
                const double out = StepCell(spec.push, spec.pull, spec.chamber, c.vent, c.state, pressure, dtIn,
                                            invDtIn, pushForce, -pushForce, drawn, bend2, noiseScale * draw);
                const float o = static_cast<float>(out);
                if (spec.bass) b[k] += o;
                else { tl[k] += o * c.gainL; tr[k] += o * c.gainR; }
            }
            open += c.lift * spec.airWeight;
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
        bassLow += bassCoeff * (bassSide.Process(b, stages) - bassLow);
        gain += smoothing * (gainTarget - gain);
        bassGain += smoothing * (bassGainTarget - bassGain);
        trebleGain += smoothing * (trebleGainTarget - trebleGain);
        const double bass = (bassLow + hiss) * bassGain;
        const double l = gain * (grilleL * trebleGain + bass * bassPanL);
        const double r = gain * (grilleR * trebleGain + bass * bassPanR);
        powerL += detector * (l * l - powerL);
        powerR += detector * (r * r - powerR);
        if (--untilGain <= 0) {
            untilGain = kGainPeriod;
            const double over = 10.0 * std::log10(Max(Max(powerL, powerR), 1.0e-20)) - kThresholdDb;
            const double slope = 1.0 / kRatio - 1.0;
            const double target = over <= -0.5 * kKneeDb ? 0.0
                : over >= 0.5 * kKneeDb ? slope * over
                : slope * (over + 0.5 * kKneeDb) * (over + 0.5 * kKneeDb) / (2.0 * kKneeDb);
            reduction = target < reduction ? target : reduction + compRelease * (target - reduction);
            compStep = (DbToGain(reduction) - compGain) / kGainPeriod;
        }
        compGain += compStep;
        const double outL = l * compGain, outR = r * compGain;
        const double peak = Max(Abs(outL), Abs(outR));
        limit += limitRelease * (1.0 - limit);
        if (peak * limit > kCeiling) limit = kCeiling / peak;
        left[i] = static_cast<float>(outL * limit);
        right[i] = static_cast<float>(outR * limit);
    }
    if (sounding == 0) {
        trebleLeft.FlushTiny();
        trebleRight.FlushTiny();
        bassSide.FlushTiny();
        for (double* v : { &grilleL, &grilleR, &bassLow, &powerL, &powerR, &reduction })
            if (Abs(*v) < 1.0e-20) *v = 0.0;
    }
    diag.hand = hand.position;
    diag.handVelocity = hand.velocity;
    diag.pressure = pressure;
    diag.airFlow = airFlowSmoothed * 1.0e3;
    diag.cells = sounding;
}

} // namespace respira
