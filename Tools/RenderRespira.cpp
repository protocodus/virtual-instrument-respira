// Renders a MIDI file through one factory preset, with a trace of the hand
// and the bellows. Tools/fit_bellows.py plays the demo phrases through it.
//
//   RenderRespira <preset> <in.mid> <out.wav> [trace.csv] [--set <control>=<value> ...]
//
// --set overrides one of the preset's controls by id, in physical units
// (RespiraControls.h), for experiments.
// --transport <csv> supplies host transport at explicit times, for Auto:
// seconds,bpm,numerator,denominator,quarter_position,bar_start. The MIDI
// reader applies tempo to note times but does not expose meta events to the
// engine. An authored host timeline keeps Auto aligned with those same bars.
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
    std::string trace, transportPath;
    for (int i = 4; i < argc; ++i) {
        if (std::strcmp(argv[i], "--transport") == 0) {
            if (++i == argc) { std::fprintf(stderr, "--transport needs a CSV path\n"); return 2; }
            transportPath = argv[i];
            continue;
        }
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

    struct HostEvent { std::size_t frame; respira::Transport transport; };
    std::vector<HostEvent> host;
    if (!transportPath.empty()) {
        std::ifstream input(transportPath);
        if (!input) { std::fprintf(stderr, "cannot read transport %s\n", transportPath.c_str()); return 2; }
        std::string line;
        std::getline(input, line); // header
        while (std::getline(input, line)) {
            double seconds = 0.0; respira::Transport t;
            if (std::sscanf(line.c_str(), "%lf,%lf,%d,%d,%lf,%lf", &seconds, &t.bpm,
                            &t.numerator, &t.denominator, &t.position, &t.barStart) != 6
                || !std::isfinite(seconds) || seconds < 0.0 || seconds > 86400.0
                || !std::isfinite(t.bpm) || t.bpm <= 0.0 || t.bpm > 999.0
                || !std::isfinite(t.position) || !std::isfinite(t.barStart)
                || t.numerator < 1 || t.numerator > 64 || t.denominator < 1 || t.denominator > 64) {
                std::fprintf(stderr, "invalid transport row: %s\n", line.c_str()); return 2;
            }
            t.playing = true;
            const auto frame = static_cast<std::size_t>(std::llround(seconds * demos::kSampleRate));
            if (!host.empty() && frame < host.back().frame) {
                std::fprintf(stderr, "transport rows must be time ordered\n"); return 2;
            }
            host.push_back({frame,t});
        }
        if (host.empty() || host.front().frame != 0) {
            std::fprintf(stderr, "transport must start at zero\n"); return 2;
        }
    }

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
    std::size_t next = 0, nextHost = 0, done = 0;
    while (done < frames) {
        while (nextHost < host.size() && host[nextHost].frame <= done) {
            engine->SetTransport(host[nextHost].transport);
            ++nextHost;
        }
        while (next < events.size() && due(next) <= done) {
            engine->Midi(events[next].bytes, events[next].size);
            ++next;
        }
        std::size_t until = std::min({ frames, done + demos::kBlockFrames, (done / traceStep + 1) * traceStep });
        if (next < events.size()) until = std::min(until, due(next));
        if (nextHost < host.size()) until = std::min(until, host[nextHost].frame);
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
