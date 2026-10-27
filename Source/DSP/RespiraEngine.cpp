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
// m^3/s per Pa with the air button held: 4 L/s at 100 Pa, about what a 30 mm
// valve passes there, so an ordinary stroke stays below the reeds' speaking
// pressure.
constexpr double kAirValve = 4.0e-5;
constexpr double kHandCatchUp = 25.0;       // 1/s: how fast the hand closes on the wheel position
// The radiated sound is the pallet flow's rate of change as from a small
// source 1 m away; 1 Pa there (94 dB SPL) is digital full scale. A single
// reed played moderately peaks near -7 dBFS and the full register near +8,
// so the factory presets set their Volume to bring each to the same range.
constexpr double kPascalToFullScale = 1.0;
constexpr double kBassLowpassHz = 2500.0;   // the bass reeds sound through the bass box's grille
constexpr double kOpenSeconds = 0.012, kOpenVelocity = 0.009;  // pallet lift: 12 ms soft .. 3 ms hard
constexpr double kCloseSeconds = 0.008;
constexpr double kQuietSeconds = 0.02;      // a closed chamber is let go after this
constexpr double kTurbulence = 2.0e-3;      // gap turbulence force per Pa of bellows pressure and m^2

double DbToGain(double db) { return std::pow(10.0, db / 20.0); }

// Rank of the bass side a group sounds: channel 2 bass buttons, channel 3 chords.
enum Group { kTreble, kBassButtons, kChordButtons };

} // namespace

void Engine::Hand::Tick(double dt) {
    if (!placed) {
        if (!received) return;
        position = lastTarget = target;
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
    const double next = Clamp(position + dt * (speed + kHandCatchUp * (target - position)), 0.0, 1.0);
    velocity = (next - position) / dt;
    position = next;
}

Engine::Engine() { Prepare(48000.0); }

void Engine::Prepare(double sampleRate) {
    rate = sampleRate > 1000.0 && sampleRate < 1.0e6 ? sampleRate : 48000.0;
    internalRate = rate * kOversample;
    bassCoeff = 1.0 - std::exp(-2.0 * kPi * kBassLowpassHz / rate);
    smoothing = 1.0 - std::exp(-1.0 / (0.02 * rate));
    SetControls(controls);
    Reset();
}

void Engine::Reset() {
    for (auto& c : cells) c = Cell {};
    hand = Hand {};
    pressure = airFlow = airFlowSmoothed = demand = 0.0;
    airPedal = false;
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
    gainTarget = kRho / (4.0 * kPi) * kPascalToFullScale * DbToGain(controls[kVolume]);
    bassGainTarget = DbToGain(controls[kBass]);
    const double bassPan = -0.9 * controls[kWidth];
    bassPanL = static_cast<float>(std::cos((bassPan + 1.0) * kPi / 4.0));
    bassPanR = static_cast<float>(std::sin((bassPan + 1.0) * kPi / 4.0));
    for (auto& c : cells)
        if (c.active) UpdatePan(c);
}

void Engine::SetMasterTune(double cents) { masterTune = cents == cents ? Clamp(cents, -100.0, 100.0) : 0.0; }

void Engine::SetHand(double position) {
    hand.target = position == position ? Clamp(position, 0.0, 1.0) : hand.target;
    hand.received = true;
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
        cell->open = 0.0;
        cell->reed = reed;
        cell->rank = rank;
        UpdatePan(*cell);
    }
    cell->active = cell->held = true;
    cell->source = source;
    cell->order = ++order;
    cell->closedFor = 0.0;
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
    }
}

void Engine::NoteOff(int channel, int key) {
    const int source = channel * 128 + key;
    for (auto& c : cells)
        if (c.active && c.held && c.source == source) c.held = false;
}

void Engine::AllNotesOff() {
    for (auto& c : cells) c.held = false;
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
    }
}

void Engine::Render(float* left, float* right, int frames) {
    const double dt = 1.0 / rate, dtIn = 1.0 / internalRate, invDtIn = internalRate;
    const double closeRate = 1.0 / (kCloseSeconds * internalRate);
    const double litres = controls[kBellows] * 1.0e-3, assist = controls[kAssist];
    const bool airValve = airPedal || controls[kAir] >= 0.5;
    for (int i = 0; i < frames; ++i) {
        hand.Tick(dt);
        // Assist makes the hand's stroke move as much more air as the open
        // reeds take beyond one A4 reed, so one wheel speed gives one pressure
        // whatever is played; at 0 the hand moves the bellows' own air only.
        const double handFlow = -litres * hand.velocity * (1.0 + assist * (Max(demand, 1.0) - 1.0));
        const double leak = (kBellowsLeak + (airValve ? kAirValve : 0.0)) * pressure;
        pressure = Clamp(pressure + dt * kBellowsStiffness * (handFlow - airFlow - leak), -kMaxPressure, kMaxPressure);

        float tl[kOversample] = {}, tr[kOversample] = {}, b[kOversample] = {};
        double drawn = 0.0, open = 0.0;
        int active = 0;
        for (auto& c : cells) {
            if (!c.active) continue;
            ++active;
            const CellSpec& spec = c.spec;
            double lift = 0.0;
            for (int k = 0; k < kOversample; ++k) {
                c.open = c.held ? Min(1.0, c.open + c.openRate) : Max(0.0, c.open - closeRate);
                // The pallet lifts off and settles smoothly: the hole area follows
                // a raised cosine of the key travel, so the jet starts and stops
                // without a step in the radiated flow.
                lift = c.open >= 1.0 ? 1.0 : c.open <= 0.0 ? 0.0 : 0.5 - 0.5 * std::cos(kPi * c.open);
                // Turbulence in the gap: a small random force that also seeds the reeds.
                noise = noise * 1664525u + 1013904223u;
                const double pushForce = kTurbulence * Unit(noise) * spec.push.area * Abs(pressure);
                const double out = StepCell(spec.push, spec.pull, spec.chamber, c.state, pressure, lift, dtIn,
                                            invDtIn, pushForce, -pushForce, drawn);
                const float o = static_cast<float>(out);
                if (spec.bass) b[k] += o;
                else { tl[k] += o * c.gainL; tr[k] += o * c.gainR; }
            }
            open += lift * spec.airWeight;
            if (!c.held && c.open <= 0.0) {
                c.closedFor += dt;
                if (c.closedFor > kQuietSeconds) c.active = false;
            }
        }
        sounding = active;
        demand = open;
        airFlow = drawn / kOversample;
        airFlowSmoothed += 0.002 * (airFlow - airFlowSmoothed);

        grilleL += grilleCoeff * (trebleLeft.Process(tl) - grilleL);
        grilleR += grilleCoeff * (trebleRight.Process(tr) - grilleR);
        bassLow += bassCoeff * (bassSide.Process(b) - bassLow);
        gain += smoothing * (gainTarget - gain);
        bassGain += smoothing * (bassGainTarget - bassGain);
        const double bass = bassLow * bassGain;
        left[i] = static_cast<float>(gain * (grilleL + bass * bassPanL));
        right[i] = static_cast<float>(gain * (grilleR + bass * bassPanR));
    }
    if (sounding == 0) {
        trebleLeft.FlushTiny();
        trebleRight.FlushTiny();
        bassSide.FlushTiny();
        for (double* v : { &grilleL, &grilleR, &bassLow })
            if (Abs(*v) < 1.0e-20) *v = 0.0;
    }
    diag.hand = hand.position;
    diag.handVelocity = hand.velocity;
    diag.pressure = pressure;
    diag.airFlow = airFlowSmoothed * 1.0e3;
    diag.cells = sounding;
}

} // namespace respira
