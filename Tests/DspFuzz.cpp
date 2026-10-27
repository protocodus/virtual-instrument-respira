// U4 input fuzzing of the JUCE-free engine through the shared portfolio
// harness (Tests/FuzzHarness.h is a byte-identical copy of the master; change
// it in reason-rack-extensions). Every input starts from a fresh engine, so a
// saved crash replays alone; commands and rendered frames are bounded
// whatever the input length.
#include "DSP/RespiraEngine.h"
#include "DSP/RespiraPresets.h"
#include "FuzzHarness.h"

#include <cmath>
#include <memory>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    using namespace protocodus::fuzz;
    Input input { data, size };

    auto engine = std::make_unique<respira::Engine>();
    engine->Prepare(input.Rate());   // Prepare() replaces a nonsense rate with 48 kHz
    respira::Controls controls = respira::kPresets[input.Pick(respira::kPresets.size())].ToControls();
    engine->SetControls(controls);

    constexpr std::size_t maxFrames = 512;
    Sentinels<maxFrames> block;
    for (int command = 0; command < 64 && !input.Empty(); ++command) {
        switch (input.Pick(9)) {
        case 0: {   // any channel message, including truncated and running-status-like ones
            const std::uint8_t bytes[3] = { input.Byte(), input.Byte(), input.Byte() };
            engine->Midi(bytes, static_cast<int>(input.Pick(4)));
            break;
        }
        case 1: {   // a note on any channel, with any velocity
            const std::uint8_t bytes[3] = { static_cast<std::uint8_t>(0x90 | (input.Byte() & 0x0F)),
                                            static_cast<std::uint8_t>(input.Byte() & 0x7F),
                                            static_cast<std::uint8_t>(input.Byte() & 0x7F) };
            engine->Midi(bytes, 3);
            break;
        }
        case 2:
        case 3: engine->SetHand(input.Scalar()); break;   // the bellows, twice as often as the rest
        case 4:
            // One raw write, which may be any float, to any control.
            controls[static_cast<int>(input.Pick(respira::kControlCount))] = input.Scalar() * 100.0f;
            engine->SetControls(controls);
            break;
        case 5:
            controls = respira::kPresets[input.Pick(respira::kPresets.size())].ToControls();
            engine->SetControls(controls);
            break;
        case 6: engine->SetMasterTune(input.Scalar() * 400.0f - 200.0f); break;
        case 7: engine->Reset(); break;
        default:
            if (input.Byte() & 1) engine->Prepare(input.Rate());
            else engine->AllNotesOff();
            break;
        }
        const auto frames = 1 + input.Word() % maxFrames;
        block.Arm();
        engine->Render(block.Left(), block.Right(), static_cast<int>(frames));
        block.Check(frames);
        // The output stage's peak limiter holds every sample at -1.5 dBFS
        // (Tests/EngineTests.cpp, LoudestCase); full scale here.
        for (std::size_t i = 0; i < frames; ++i)
            Require(std::fabs(block.Left()[i]) < 1.0f && std::fabs(block.Right()[i]) < 1.0f);
        const auto& d = engine->Diag();
        Require(std::isfinite(d.pressure) && std::fabs(d.pressure) <= 2500.0 && d.cells >= 0
                && d.cells <= respira::Engine::kMaxCells);
    }
    return 0;
}
