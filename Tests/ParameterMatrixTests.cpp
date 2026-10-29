// U4 parameter-extremes matrix over the engine and its shared control set,
// through the shared portfolio harness (Tests/ParameterMatrix.h is a
// byte-identical copy of the master; change it in reason-rack-extensions).
// Respira has no plug-in yet, so the device is the engine with the controls
// every front end uses (RespiraControls.h), stored normalised as a front end
// stores them.
//
//   RespiraParameterMatrixTests [--jobs N] [--rotation] [--shard K/N]
//                               [--case LABEL] [--count] [--list]
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
    // delivers controller values.
    void Process(float* left, float* right, int frames, const Event* events, int eventCount) override {
        int done = 0;
        for (int e = 0; e <= eventCount; ++e) {
            const int until = e < eventCount ? events[e].offset : frames;
            if (until > done) {
                engine->SetHand(Hand(static_cast<double>(elapsed + done) / rate));
                engine->Render(left + done, right + done, until - done);
                done = until;
            }
            if (e < eventCount) engine->Midi(events[e].bytes, events[e].size);
        }
        elapsed += frames;
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

private:
    // The hand rests once the keys are up (the harness lets go at 0.8 s), as a
    // player's does: still moving, it would turn a closed bellows and the
    // folds would flip (Mechanics.h), so no silence would be due. The reeds'
    // silence under a moving hand is Respira.Engine's ReleaseAndAllNotesOff.
    static double Hand(double seconds) {
        seconds = std::min(seconds, 0.8);
        const double phase = seconds - std::floor(seconds);   // one stroke each way per second
        return 0.5 + 0.6 * (phase < 0.5 ? phase : 1.0 - phase) - 0.15;
    }
    static double Value(const std::vector<float>& values, int control) { return FromNormalized(control, values[static_cast<std::size_t>(control)]); }

    void Apply() {
        Controls controls;
        for (int i = 0; i < kControlCount; ++i) controls[i] = FromNormalized(i, normalised[static_cast<std::size_t>(i)]);
        engine->SetControls(controls);
    }

    std::unique_ptr<Engine> engine;
    std::array<float, kControlCount> normalised {};
    double rate = 48000.0;
    long long elapsed = 0;
};

} // namespace

int main(int argc, char** argv) {
    return protocodus::matrix::Main(argc, argv, [] { return std::make_unique<RespiraDevice>(); });
}
