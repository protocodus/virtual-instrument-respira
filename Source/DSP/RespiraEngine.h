#pragma once

// Respira: a physically modelled accordion whose bellows is the player's hand.
//
// Every sounding note of every register rank is a reed cell (ReedModel.h): a
// push reed and a pull reed in one chamber, vented through the key's pallet.
// The cells draw their air from one bellows. The bellows is moved by the hand,
// read from the mod wheel: opening draws (pull reeds speak), closing presses
// (push reeds speak), and only the hand's speed moves air, so the wheel has to
// keep moving back and forth. Nothing here is an oscillator.
//
// MIDI (Midi)
//   channel 1  treble keyboard. With Keyboard = split, C1..B1 are bass buttons
//              and C2..B2 chord buttons (one pitch class each, so any chord can
//              be played), C3 and up the treble.
//   channel 2  bass buttons (pitch class, folded into the bass octaves)
//   channel 3  chord buttons (pitch class, folded into the chord octave)
//   CC1 + CC33 hand position, 14-bit: 0 bellows closed .. 16383 fully open
//              (CC1 alone is read as 7-bit until a CC33 arrives)
//   CC64       air button, held with Air: vents the bellows so the hand can
//              move without sound
//   CC120/123  all notes off
//   channel pressure  holds that channel's keys part way down: full pressure
//              leaves each pallet lifted 1 mm, so a strong bellows bends
//              the reeds flat and quieter, as a half-pressed key does
//   pitch bend +-2 semitones, retuning every sounding tongue
// Other channels play as channel 1. Velocity sets how fast the pallet lifts.
// With Auto on, the hand plays the bellows by itself while keys are held, in
// time with the host (SetTransport), and the wheel presses it harder.
// The register switches act on held keys too, as the slides they move do.
// The keys, buttons, pallets and switches knock on their stops, and the
// bellows' folds flip as the bellows turn and rustle as they move
// (Mechanics.h).
//
// JUCE-free, C++17, no allocation, locks or exceptions after construction.

#include "Decimator.h"
#include "Mechanics.h"
#include "OutputStage.h"
#include "ReedBank.h"
#include "RespiraControls.h"

#include <array>
#include <cstdint>

namespace respira {

struct Register {
    const char* name;
    std::array<bool, 5> ranks;   // kRankL .. kRankH
};

// The treble register switches, in Register control order.
inline constexpr std::array<Register, 8> kRegisters { {
    { "Clarinet", { false, true, false, false, false } },
    { "Bassoon", { true, false, false, false, false } },
    { "Violin", { false, true, false, true, false } },
    { "Musette", { false, true, true, true, false } },
    { "Bandoneon", { true, true, false, false, false } },
    { "Organ", { true, false, false, false, true } },
    { "Oboe", { false, true, false, false, true } },
    { "Master", { true, true, true, true, true } },
} };

// The host's tempo and song position, for Auto. A front end sets it before a
// block; while `playing`, the engine advances the position through the block.
struct Transport {
    double bpm = 120.0;
    int numerator = 4, denominator = 4;
    bool playing = false;
    double position = 0.0;   // quarter notes, at the next sample
    double barStart = 0.0;   // quarter notes, where the bar holding `position` began
};

struct Diagnostics {
    double hand = 0.0;           // 0..1
    double handVelocity = 0.0;   // wheel ranges per second
    double pressure = 0.0;       // bellows pressure, Pa (> 0 push, < 0 pull)
    double airFlow = 0.0;        // air through the reeds, L/s (smoothed)
    int cells = 0;               // sounding reed chambers
};

class Engine {
public:
    // The reeds run at kBaseOversample times the host rate; Oversampling
    // multiplies that by 1, 2 or 4.
    static constexpr int kBaseOversample = 4, kMaxOversample = kBaseOversample * 4;
    static constexpr int kMaxCells = 72;

    Engine();
    void Prepare(double sampleRate);
    void Reset();                         // silence, bellows at rest, hand unplaced
    void SetControls(const Controls& controls);
    const Controls& GetControls() const { return controls; }
    static constexpr double kBendSemitones = 2.0;

    void SetMasterTune(double cents);     // applies to the next notes
    void SetPitchBend(double amount);     // -1 .. 1, kBendSemitones each way, sounding reeds too
    void SetHand(double position);        // 0 closed .. 1 open
    void SetTransport(const Transport& transport);
    void SetChannelPressure(int channel, double amount);   // 0 keys fully down .. 1 half pressed
    void Midi(const std::uint8_t* bytes, int size);
    void AllNotesOff();
    void Render(float* left, float* right, int frames);
    bool Sounding() const { return sounding > 0; }
    // The output stage's look-ahead, samples at the host rate: a front end
    // reports it to its host as latency.
    int LatencySamples() const { return output.DelaySamples(); }
    const Diagnostics& Diag() const { return diag; }

private:
    struct Cell {
        bool active = false, held = false;
        int source = 0;          // channel * 128 + played key, for the note-off
        int reed = 0;            // rank * 128 + table key: one chamber per reed
        int rank = 0;
        std::uint64_t order = 0;
        double open = 0.0, openRate = 0.0, closedFor = 0.0;   // key travel, 0 closed .. 1 down
        double depth = 1.0;      // travel the held key rests at (channel pressure)
        bool settled = false;    // the pallet has reached the held key's travel
        double lift = 0.0;       // pallet opening for `open`
        CellSpec spec;
        Vent vent;               // the pallet hole's flow law at `lift`
        CellState state;
        float gainL = 0.0f, gainR = 0.0f, gainLTarget = 0.0f, gainRTarget = 0.0f;   // pan
        double breathA = 0.0, breathB = 0.0;   // the gap turbulence, shaped to its eddies
    };

    // A shallow compartment the pallets open into, radiating through a
    // perforated cover: a Helmholtz resonator per unit of cover area, so a
    // second-order low-pass, and beside it the leak of its depth modes, a
    // one-pole low-pass and a second-order high-pass (Engine::Compartment::Set).
    // Each is bilinear and prewarped, in transposed direct form II.
    struct Compartment {
        struct Section {
            double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0, z1 = 0.0, z2 = 0.0;
            double Process(double x) {
                const double y = b0 * x + z1;
                z1 = b1 * x - a1 * y + z2;
                z2 = b2 * x - a2 * y;
                return y;
            }
        };
        Section box, leak, modes;
        // depth m, open fraction of the cover, effective hole length m
        // (thickness plus end corrections), cloth behind it in rayl, the
        // fraction of the sound's energy the box absorbs on a round trip, and
        // where in Hz the depth modes take over from the lumped resonator
        void Set(double depth, double open, double neck, double rayl, double absorbed, double modesHz, double rate);
        void Clear() { box.z1 = box.z2 = leak.z1 = leak.z2 = modes.z1 = modes.z2 = 0.0; }
        void FlushTiny();
        double Process(double x) { return box.Process(x) + modes.Process(leak.Process(x)); }
    };

    // The hand on the bellows, from a stepwise controller stream. The wheel's
    // path is rebuilt first: between steps it keeps the speed of the last one
    // (a slow wheel sends a step only every so often), slowing as a step fails
    // to arrive, and closes on the latest position, so even a 7-bit wheel gives
    // a continuous air flow. The arm then follows that path with Momentum: a
    // critically damped mass with that time constant, so the bellows reverse
    // and stop smoothly, a little behind the wheel, and never overshoot it.
    struct Hand {
        // The first position received is where the hand starts, taken at the
        // next tick so that a 14-bit CC1 + CC33 pair places it as one value.
        bool received = false, placed = false;
        double target = 0.0, lastTarget = 0.0;
        double wheel = 0.0, speed = 0.0, lastStep = 0.0, sinceStep = 1.0;
        double position = 0.0, velocity = 0.0;   // the arm, which moves the bellows
        // The arm follows the wheel, or with `drive` the player's own path.
        void Tick(double dt, double momentum, const double* drive = nullptr, double driveVelocity = 0.0);
    };

    // Auto: a player's hand that moves the bellows while any key is held and
    // rests when none is. It pulls for one stroke and pushes for the next, a
    // stroke being one bar, or two, four or eight when a bar is shorter than
    // kAutoShortestStroke; so in 3/4 a pull and a push take 6 beats. While the
    // host plays, the strokes turn on its bar lines; stopped, they run at its
    // tempo from the first note, starting again on a pull after a rest of a
    // stroke or more. The player holds kAutoPressure, up to four times it with
    // the wheel, whatever is played and whatever Bellows and Assist are set
    // to, and turns early only if the bellows run out (Engine::StepPlayer).
    struct Player {
        bool running = false, turned = false;
        int half = 0;                 // 0 pulling, 1 pushing, on the grid
        double cycle = 0.0;           // quarter notes into the pull + push cycle
        double stroke = 16.0;         // quarter notes in a stroke
        double bar = 4.0;             // quarter notes in a bar
        double barLine = 0.0;         // the bar line the hand's next turn is timed for
        double rested = 1.0e9;        // seconds since the hand last moved
        double drive = 0.0, velocity = 0.0;   // the hand's path, in wheel ranges, which the arm follows
        bool pulled = false;          // the way the hand moved last sample
        bool swinging = false;        // turning: the hand goes as fast as it can
        double hold = 0.0;            // the air the player expects the reeds to take, m^3/s
    };

    void StepPlayer(double dt);

    // A key or button of one channel (channel * 128 + key): its travel, 0 up
    // .. 1 down, which its pallets follow, and where it is headed.
    struct Key {
        double travel = 0.0, target = 0.0, rate = 0.0;   // rate: travel per sample going down
        bool button = false, listed = false;
    };

    void NoteOn(int channel, int key, int velocity);
    void NoteOff(int channel, int key);
    void StartCell(int rank, int key, int source, int velocity);
    void SwitchRegister(int from, int to);
    void UpdatePan(Cell& cell) const;
    void MoveKey(int source);
    void StartKnock(const Stop& stop, double speed, double massScale, bool bass, int tableKey);
    void StepKeys();

    Controls controls;
    double rate = 48000.0, internalRate = 192000.0;
    int oversample = kBaseOversample, stages = 2;   // internal steps per sample, its log2
    double noiseScale = 1.0, turbulence = 0.0;
    double masterTune = 0.0;
    double bend2 = 1.0, bend2Target = 1.0, bendSmoothing = 0.0;   // tongue stiffness scale
    Hand hand;
    Transport transport;
    Player player;
    bool autoHand = false;
    int heldCells = 0;
    double pressure = 0.0, airFlow = 0.0, airFlowSmoothed = 0.0, demand = 0.0;
    bool airPedal = false;
    double valve = 0.0;   // air valve opening, 0..1
    double valveNoiseA = 0.0, valveNoiseB = 0.0, valveSound = 0.0;   // its jet's flow fluctuation
    int registerIndex = static_cast<int>(kControlInfo[kRegister].initial);
    std::array<std::uint8_t, 128> trebleHeld {};   // velocity of each held treble key, 0 when up
    std::array<double, 3> channelDepth { 1.0, 1.0, 1.0 };
    int msb = 0, lsb = 0;
    bool fineWheel = false;
    std::uint64_t order = 0;
    std::uint32_t noise = 1u;
    int sounding = 0;
    std::array<Cell, kMaxCells> cells {};
    Decimator trebleLeft, trebleRight, bassSide;
    Compartment bassBox;   // under the bass end cover
    double grilleCoeff = 0.0;
    double grilleL = 0.0, grilleR = 0.0;
    double gain = 0.0, gainTarget = 0.0, bassGain = 0.0, bassGainTarget = 0.0, smoothing = 0.0;
    double trebleGain = 1.0, trebleGainTarget = 1.0;   // the register's trim
    protocodus::OutputStage<2> output;
    float bassPanL = 0.0f, bassPanR = 0.0f, bassPanLTarget = 0.0f, bassPanRTarget = 0.0f;
    std::array<Key, 3 * 128> keys {};
    std::array<std::uint16_t, 3 * 128> movingKeys {};   // the keys whose travel is changing
    int movingCount = 0;
    std::array<Knock, 32> knocks {};
    Folds folds;
    Rustle rustle;
    double rustleForce = 0.0;
    BodyHighPass mechanicsL, mechanicsR, mechanicsBass;
    double mechanicsGain = 0.0, mechanicsGainTarget = 0.0;
    Diagnostics diag;
};

} // namespace respira
