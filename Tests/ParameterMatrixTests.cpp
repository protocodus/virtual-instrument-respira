// U4 parameter-extremes matrix over the engine and its shared control set,
// through the shared portfolio harness (Tests/ParameterMatrix.h is a
// byte-identical copy of the master; change it in reason-rack-extensions).
// Respira has no plug-in yet, so the device is the engine with the controls
// every front end uses (RespiraControls.h), stored normalised as a front end
// stores them.
//
//   RespiraParameterMatrixTests [--jobs N] [--rotation] [--shard K/N]
//                               [--case LABEL] [--count] [--list]
//   RespiraParameterMatrixTests --reach [--jobs N] [--case PARAMETER]
//
// The limits below follow from Source/DSP/RespiraEngine.cpp; each one names
// the mechanism it comes from.

#include "DSP/RespiraEngine.h"
#include "DSP/RespiraPresets.h"
#include "ParameterMatrix.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>
#include <utility>
#include <vector>

namespace {

using namespace respira;
using protocodus::matrix::Case;
using protocodus::matrix::Event;
using protocodus::matrix::Limits;
using protocodus::matrix::Parameter;

class RespiraDevice final : public protocodus::matrix::Device {
public:
    RespiraDevice() : engine(std::make_unique<Engine>()) {
        for (int i = 0; i < kControlCount; ++i) normalised[static_cast<std::size_t>(i)] = static_cast<float>(ToNormalized(i, kControlInfo[static_cast<std::size_t>(i)].initial));
    }

    std::vector<Parameter> Parameters() const override {
        std::vector<Parameter> parameters;
        for (int i = 0; i < kControlCount; ++i) {
            const ControlInfo& c = kControlInfo[static_cast<std::size_t>(i)];
            parameters.push_back({ c.id, c.steps, static_cast<float>(ToNormalized(i, c.initial)) });
        }
        return parameters;
    }
    int PresetCount() const override { return static_cast<int>(kPresets.size()); }
    void Prepare(double sampleRate, int) override {
        rate = sampleRate;
        engine->Prepare(sampleRate);
        Apply();
    }
    void LoadPreset(int index) override {
        for (int i = 0; i < kControlCount; ++i)
            normalised[static_cast<std::size_t>(i)] = static_cast<float>(ToNormalized(i, kPresets[static_cast<std::size_t>(index)].values[static_cast<std::size_t>(i)]));
        Apply();
    }
    float GetParameter(std::size_t index) const override { return normalised[index]; }
    void SetParameter(std::size_t index, float value) override {
        normalised[index] = value;
        Apply();
    }

    // The phrase has no bellows of its own, so the device is also the hand: a
    // steady back-and-forth of the wheel, fast enough to sound a chord at the
    // smallest Bellows without Assist, placed at each block start as a host
    // delivers controller values. It moves while a key is down and rests,
    // where it is, while none is, as a player's does: still moving, it would
    // turn a closed bellows and the folds would flip (Mechanics.h), so no
    // silence would be due. The reeds' silence under a moving hand is
    // Respira.Engine's ReleaseAndAllNotesOff.
    void Process(float* left, float* right, int frames, const Event* events, int eventCount) override {
        int done = 0;
        for (int e = 0; e <= eventCount; ++e) {
            const int until = e < eventCount ? events[e].offset : frames;
            if (until > done) {
                engine->SetHand(Hand(static_cast<double>(moving) / rate));
                engine->Render(left + done, right + done, until - done);
                if (keysDown > 0) moving += until - done;
                done = until;
            }
            if (e < eventCount) {
                Track(events[e]);
                engine->Midi(events[e].bytes, events[e].size);
            }
        }
    }

    std::vector<std::uint8_t> SaveState() override {
        std::vector<std::uint8_t> state(sizeof normalised);
        std::memcpy(state.data(), normalised.data(), sizeof normalised);
        return state;
    }
    void RestoreState(const std::vector<std::uint8_t>& state) override {
        if (state.size() != sizeof normalised) return;
        std::memcpy(normalised.data(), state.data(), sizeof normalised);
        Apply();
    }

    Limits LimitsFor(const Case& c) const override {
        Limits limits;
        // The air button vents the bellows, so held notes go quiet by design
        // while it is on in either row; and the air rushing through it hisses
        // while the hand keeps moving, keys or none, so no silence is due.
        const bool air = Value(c.values, kAir) >= 0.5 || Value(c.base, kAir) >= 0.5;
        limits.soundsWhileHeld = !air;
        if (air) limits.releaseSeconds = -1.0;
        return limits;
    }

    protocodus::matrix::ReachRule ReachFor(std::size_t index, float) const override {
        using protocodus::matrix::Reach;
        const std::vector<std::pair<std::size_t, float>> manual { { kAuto, static_cast<float>(ToNormalized(kAuto, 0.0)) } };
        switch (static_cast<int>(index)) {
        // Engine::StartCell: a sounding cell keeps the reed it was built with.
        case kMusette: return { Reach::NewNotes, "the tremolo reeds' tuning each new note is built with" };
        // Auto's arm moves the air the reeds ask for whatever the bellows'
        // size (StepPlayer), and Assist only lets it go faster; with the
        // wheel as the hand both set the air each stroke moves.
        case kBellows:
        case kAssist: return { Reach::Held, "", manual };
        // The phrase plays from G3 up on channel 1, all treble.
        case kBass: return { Reach::Exempt, "the bass and chord side's level; the phrase plays the treble only" };
        case kKeyboard: return { Reach::Exempt, "what the keys below C3 play; the phrase plays from G3 up" };
        // What a finer internal rate changes sits above the default Grille's
        // 1 kHz. The reeds' turbulence is drawn once per internal step, so a
        // new rate also moves the noise sequence on (Engine::SetControls).
        case kOversampling:
            return { Reach::Held, "", { { kGrille, 1.0f } }, "the reeds' turbulence runs on in the sequence the old rate drew" };
        case kMechanics: return { Reach::Exempt, "the knocks of the keys and buttons, far under the reeds while they sound" };
        // Touch::Effort: a chord at full velocity keeps no sustained share of
        // it once its accent has passed, so only new strikes carry it.
        case kTouch: return { Reach::NewNotes, "the accent and effort each new strike's velocity asks of the arm" };
        default: return {};
        }
    }

private:
    // The hand's position after `seconds` of moving.
    static double Hand(double seconds) {
        const double phase = seconds - std::floor(seconds);   // one stroke each way per second
        return 0.5 + 0.6 * (phase < 0.5 ? phase : 1.0 - phase) - 0.15;
    }
    static double Value(const std::vector<float>& values, int control) { return FromNormalized(control, values[static_cast<std::size_t>(control)]); }

    void Apply() {
        Controls controls;
        for (int i = 0; i < kControlCount; ++i) controls[i] = FromNormalized(i, normalised[static_cast<std::size_t>(i)]);
        engine->SetControls(controls);
    }

    void Track(const Event& event) {
        const int status = event.bytes[0] & 0xF0;
        const std::size_t key = event.bytes[1] & 0x7F;
        const bool on = status == 0x90 && event.bytes[2] > 0;
        if ((on || status == 0x80 || status == 0x90) && down[key] != on) {
            down[key] = on;
            keysDown += on ? 1 : -1;
        }
    }

    std::unique_ptr<Engine> engine;
    std::array<float, kControlCount> normalised {};
    double rate = 48000.0;
    std::array<bool, 128> down {};
    int keysDown = 0;
    long long moving = 0; // frames the hand has moved
};

} // namespace

int main(int argc, char** argv) {
    return protocodus::matrix::Main(argc, argv, [] { return std::make_unique<RespiraDevice>(); });
}
