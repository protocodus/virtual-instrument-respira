// Renders one README demo per factory preset (README G3) from the committed
// phrases in Docs/audio/midi, through the shipping engine. The contract and
// the fixed rate, block size and seed live in the shared PresetDemoRenderer.h.

#include "DSP/RespiraEngine.h"
#include "DSP/RespiraPresets.h"
#include "PresetDemoRenderer.h"

#include <memory>
#include <stdexcept>
#include <string>

namespace {

class RespiraVoice final : public protocodus::demos::Voice {
public:
    RespiraVoice(const respira::Preset& preset, double sampleRate) : engine(std::make_unique<respira::Engine>()) {
        engine->Prepare(sampleRate);
        engine->SetControls(preset.ToControls());
        engine->Reset();
    }

    // Notes and controllers go to the engine as a plug-in hands them over.
    // Anything else in a phrase is an error, not a silently dropped event.
    void Midi(const std::uint8_t* bytes, int size) override {
        const int kind = bytes[0] & 0xF0;
        if (kind != 0x80 && kind != 0x90 && kind != 0xB0)
            throw std::runtime_error("unsupported MIDI message " + std::to_string(bytes[0]));
        engine->Midi(bytes, size);
    }

    void Render(float* left, float* right, int frames) override { engine->Render(left, right, frames); }

private:
    std::unique_ptr<respira::Engine> engine;
};

} // namespace

int main(int argc, char** argv) {
    protocodus::demos::Instrument instrument;
    instrument.name = "Respira";
    for (const auto& preset : respira::kPresets) instrument.presets.push_back(preset.name);
    instrument.open = [](std::size_t preset, double sampleRate, int, std::uint32_t) {
        // The turbulence generator has a fixed seed, so the engine takes none.
        return std::make_unique<RespiraVoice>(respira::kPresets[preset], sampleRate);
    };
    // About three times the slowest demo's median, 0.25x realtime (Master
    // Tutti), measured 2026-10-27 in the development container (GCC 13,
    // Release); re-set it from the first ci.yml dsp-tests run.
    instrument.cpuBudget = 0.75;
    return protocodus::demos::Main(argc, argv, instrument);
}
