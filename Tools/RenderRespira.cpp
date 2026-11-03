// Renders a MIDI file through one factory preset, with a trace of the hand
// and the bellows. Tools/fit_bellows.py plays the demo phrases through it.
//
//   RenderRespira <preset> <in.mid> <out.wav> [trace.csv] [--set <control>=<value> ...]
//
// --set overrides one of the preset's controls by id, in physical units
// (RespiraControls.h), for experiments.
// The WAV is 48 kHz stereo 24-bit at the demo renderer's block size and runs
// 2.5 s past the last event, uncut. The trace has, every 5 ms: hand position
// (0..1), hand speed (wheel ranges per second), bellows pressure (Pa, > 0
// pushing), air through the reeds (L/s), sounding reed chambers and the RMS
// of the output over those 5 ms.
#include "DSP/RespiraEngine.h"
#include "DSP/RespiraPresets.h"
#include "PresetDemoRenderer.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <memory>
#include <string>

namespace demos = protocodus::demos;

int main(int argc, char** argv) {
    const int preset = argc >= 4 ? std::atoi(argv[1]) : -1;
    if (argc < 4 || preset < 0 || preset >= static_cast<int>(respira::kPresets.size())) {
        std::fprintf(stderr, "usage: RenderRespira <preset 0..%zu> <in.mid> <out.wav> [trace.csv] [--set <control>=<value> ...]\n",
                     respira::kPresets.size() - 1);
        return 2;
    }
    respira::Controls controls = respira::kPresets[static_cast<std::size_t>(preset)].ToControls();
    std::string trace;
    for (int i = 4; i < argc; ++i) {
        if (std::strcmp(argv[i], "--set") != 0) {
            trace = argv[i];
            continue;
        }
        const std::string setting = i + 1 < argc ? argv[++i] : "";
        const auto equals = setting.find('=');
        int control = 0;
        while (control < respira::kControlCount && setting.compare(0, equals, respira::kControlInfo[static_cast<std::size_t>(control)].id) != 0)
            ++control;
        if (equals == std::string::npos || control == respira::kControlCount) {
            std::fprintf(stderr, "--set %s: expected <control>=<value> with a control id from RespiraControls.h\n", setting.c_str());
            return 2;
        }
        controls[control] = std::atof(setting.c_str() + equals + 1);
    }
    const auto events = demos::ReadMidi(argv[2]);
    if (events.empty()) {
        std::fprintf(stderr, "%s: no MIDI channel events\n", argv[2]);
        return 1;
    }
    auto engine = std::make_unique<respira::Engine>();
    engine->Prepare(demos::kSampleRate);
    engine->SetControls(controls);
    engine->Reset();

    const auto frames = static_cast<std::size_t>((events.back().seconds + demos::kTailSeconds) * demos::kSampleRate);
    demos::detail::Demo demo;
    demo.left.assign(frames, 0.0f);
    demo.right.assign(frames, 0.0f);
    std::ofstream csv;
    if (!trace.empty()) {
        csv.open(trace);
        csv << "seconds,hand,velocity,pressure_pa,air_lps,cells,rms\n";
    }
    const auto due = [&](std::size_t i) { return static_cast<std::size_t>(std::llround(events[i].seconds * demos::kSampleRate)); };
    const auto traceStep = static_cast<std::size_t>(0.005 * demos::kSampleRate);
    std::size_t next = 0, done = 0;
    while (done < frames) {
        while (next < events.size() && due(next) <= done) {
            engine->Midi(events[next].bytes, events[next].size);
            ++next;
        }
        std::size_t until = std::min({ frames, done + demos::kBlockFrames, (done / traceStep + 1) * traceStep });
        if (next < events.size()) until = std::min(until, due(next));
        engine->Render(&demo.left[done], &demo.right[done], static_cast<int>(until - done));
        done = until;
        if (csv.is_open() && done % traceStep == 0) {
            const auto& d = engine->Diag();
            double sum = 0.0;
            for (std::size_t k = done - traceStep; k < done; ++k)
                sum += demo.left[k] * demo.left[k] + demo.right[k] * demo.right[k];
            char line[160];
            std::snprintf(line, sizeof line, "%.3f,%.5f,%.4f,%.1f,%.4f,%d,%.6f\n", done / demos::kSampleRate, d.hand,
                          d.handVelocity, d.pressure, d.airFlow, d.cells, std::sqrt(sum / (2.0 * traceStep)));
            csv << line;
        }
    }
    for (std::size_t i = 0; i < frames; ++i)
        demo.peak = std::max({ demo.peak, std::fabs(demo.left[i]), std::fabs(demo.right[i]) });
    demos::detail::WriteWav(argv[3], demo);
    std::printf("%s: %s, %.1f s, peak %.2f dBFS\n", argv[3], respira::kPresets[static_cast<std::size_t>(preset)].name,
                static_cast<double>(frames) / demos::kSampleRate, demos::detail::Dbfs(demo.peak));
    return 0;
}
