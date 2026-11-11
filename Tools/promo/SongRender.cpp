// SongRender <preset> <in.mid> <out.wav> <trace.csv>
// One Respira playing a performed part; a program change sets Register
// (0..7), as a player's thumb on a register switch. Trace at 60 Hz:
// seconds, pressure (Pa), air (L/s), sounding chambers, output RMS.
#include "DSP/RespiraEngine.h"
#include "DSP/RespiraPresets.h"
#include "PresetDemoRenderer.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
namespace demos = protocodus::demos;
int main(int argc, char** argv) {
    if (argc < 5) { std::fprintf(stderr, "usage: SongRender <preset> <in.mid> <out.wav> <trace.csv>\n"); return 2; }
    respira::Controls controls = respira::kPresets[static_cast<std::size_t>(std::atoi(argv[1]))].ToControls();
    const auto events = demos::ReadMidi(argv[2]);
    auto engine = std::make_unique<respira::Engine>();
    engine->Prepare(demos::kSampleRate);
    engine->SetControls(controls);
    engine->Reset();
    const auto frames = static_cast<std::size_t>((events.back().seconds + 3.0) * demos::kSampleRate);
    demos::detail::Demo demo;
    demo.left.assign(frames, 0.0f);
    demo.right.assign(frames, 0.0f);
    std::ofstream csv(argv[4]);
    csv << "seconds,pressure,air,cells,rms\n";
    const auto due = [&](std::size_t i) { return static_cast<std::size_t>(std::llround(events[i].seconds * demos::kSampleRate)); };
    const std::size_t step = 800;   // 60 Hz at 48 kHz
    std::size_t next = 0, done = 0;
    while (done < frames) {
        while (next < events.size() && due(next) <= done) {
            const auto& e = events[next++];
            if ((e.bytes[0] & 0xF0) == 0xC0) {
                controls[respira::kRegister] = e.bytes[1] & 7;
                engine->SetControls(controls);
            } else {
                engine->Midi(e.bytes, e.size);
            }
        }
        std::size_t until = std::min({ frames, done + static_cast<std::size_t>(demos::kBlockFrames), (done / step + 1) * step });
        if (next < events.size()) until = std::min(until, due(next));
        engine->Render(&demo.left[done], &demo.right[done], static_cast<int>(until - done));
        done = until;
        if (done % step == 0) {
            const auto& d = engine->Diag();
            double sum = 0.0;
            for (std::size_t k = done - step; k < done; ++k) sum += demo.left[k] * demo.left[k] + demo.right[k] * demo.right[k];
            char line[128];
            std::snprintf(line, sizeof line, "%.4f,%.1f,%.4f,%d,%.6f\n", done / demos::kSampleRate, d.pressure, d.airFlow, d.cells, std::sqrt(sum / (2.0 * step)));
            csv << line;
        }
    }
    for (std::size_t i = 0; i < frames; ++i) demo.peak = std::max({ demo.peak, std::fabs(demo.left[i]), std::fabs(demo.right[i]) });
    demos::detail::WriteWav(argv[3], demo);
    std::printf("%s: %.1f s, peak %.2f dBFS\n", argv[3], frames / demos::kSampleRate, demos::detail::Dbfs(demo.peak));
}
