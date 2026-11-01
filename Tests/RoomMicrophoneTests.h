#pragma once

#include "DSP/RespiraEngine.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <memory>

namespace respira_room_tests {

using Check = void (*)(bool, const char*, const char*, double);

struct Energy {
    double left = 0.0, right = 0.0, cross = 0.0;
    void Add(double l, double r) { left += l * l; right += r * r; cross += l * r; }
    double Correlation() const { return cross / std::sqrt(left * right); }
    double Power() const { return 0.5 * (left + right); }
};

inline void Run(Check expect) {
    using namespace respira;
    // These are emitted pressure ratios at isolated reflection arrivals,
    // recovered by undoing the known 2 kHz receiver low-pass. The independent
    // geometry oracle uses the stage/ceiling image heights and direct path.
    for (double width : { 0.0, 0.6, 1.0 }) {
        constexpr double rate = 48000.0;
        auto room = std::make_unique<Room>();
        room->Set(Room::kDecaySeconds, Room::kTrebleShare, rate);
        room->SetWidth(width, true);
        room->Clear();
        const double axis = width * 75.0 * 3.14159265358979323846 / 180.0;
        const double direct = 1.0 + std::cos(axis) / std::sqrt(1.04);
        const double pole = std::exp(-2.0 * 3.14159265358979323846 * 2000.0 / rate);
        const int stage = static_cast<int>(0.00407 * rate), ceiling = static_cast<int>(0.01419 * rate);
        double previous = 0.0, worst = 0.0;
        for (int i = 0; i <= ceiling; ++i) {
            double l, r;
            room->Process(i == 0 ? 1.0 : 0.0, i == 0 ? 1.0 : 0.0, l, r);
            if (i == stage || i == ceiling) {
                const double height = i == stage ? 2.2 : 5.8;
                const double level = i == stage ? 0.42 * 0.9 : 0.173 * 0.8;
                const double expected = level / std::sqrt(2.0) * (1.0 + std::cos(axis) / std::hypot(1.0, height)) / direct;
                const double recovered = (l - pole * previous) / (1.0 - pole);
                worst = std::max(worst, std::fabs(recovered - expected));
            }
            previous = l;
        }
        expect(worst < 1.0e-12, "RoomMicrophones", "stage and ceiling retain their vertical incidence", worst);
    }

    // A late field, after the early taps have ended. Its finite delay network
    // approximates independent directions: allow 0.04 absolute correlation
    // error and 5 percent power spread, not exact covariance for every note.
    for (double rate : { 44100.0, 48000.0, 96000.0 }) {
        for (int stimulus : { 0, 1 }) {
            std::array<Energy, 3> measured {};
            const std::array<double, 3> widths { 0.0, 0.6, 1.0 };
            bool mono = true, audible = true;
            double error = 0.0;
            for (std::size_t w = 0; w < widths.size(); ++w) {
                auto room = std::make_unique<Room>();
                room->Set(Room::kDecaySeconds, Room::kTrebleShare, rate);
                room->SetWidth(widths[w], true);
                room->Clear();
                std::uint32_t seed = 1u;
                const int stop = stimulus ? static_cast<int>(2.0 * rate) : 0;
                for (int i = 0; i < stop + static_cast<int>(0.45 * rate); ++i) {
                    seed = seed * 1664525u + 1013904223u;
                    const double input = stimulus ? (i < stop ? (seed >> 8) / 8388608.0 - 1.0 : 0.0) : (i == 0 ? 1.0 : 0.0);
                    double l, r;
                    room->Process(input, input, l, r);
                    if (w == 0) mono = mono && l == r;
                    if (i >= stop + static_cast<int>(0.08 * rate)) measured[w].Add(l, r);
                }
                audible = audible && std::isfinite(measured[w].left) && std::isfinite(measured[w].right)
                    && measured[w].left > 1.0e-12 && measured[w].right > 1.0e-12
                    && std::isfinite(measured[w].Correlation());
                const double expected = 0.75 + 0.25 * std::cos(widths[w] * 150.0 * 3.14159265358979323846 / 180.0);
                error = std::max(error, std::fabs(measured[w].Correlation() - expected));
            }
            double spread = 0.0;
            for (const Energy& e : measured) spread = std::max(spread, std::fabs(e.Power() / measured[0].Power() - 1.0));
            expect(audible && mono && error < 0.04, "RoomMicrophones", "late-field correlation follows the coincident cardioid angle", error);
            expect(audible && spread < 0.05, "RoomMicrophones", "changing microphone angle preserves diffuse-field power", spread);
        }
    }

    // Compare the first millisecond of a moving Width with an instantaneous
    // receiver change on the very same sound field, and its settled result.
    std::array<std::unique_ptr<Room>, 3> rooms;
    for (auto& room : rooms) {
        room = std::make_unique<Room>();
        room->Set(Room::kDecaySeconds, Room::kTrebleShare, 48000.0);
        room->SetWidth(0.0, true);
        room->Clear();
    }
    std::uint32_t seed = 7u;
    double first = 0.0, abrupt = 0.0, settled = 0.0, change = 0.0;
    for (int i = 0; i < 33600; ++i) {
        if (i == 24000) {
            rooms[1]->SetWidth(1.0);
            rooms[2]->SetWidth(1.0, true);
        }
        seed = seed * 1664525u + 1013904223u;
        const double input = (seed >> 8) / 8388608.0 - 1.0;
        std::array<double, 3> left {}, right {};
        for (std::size_t k = 0; k < rooms.size(); ++k) rooms[k]->Process(input, input, left[k], right[k]);
        const double moving = right[1] - right[0], hard = right[2] - right[0], residual = right[1] - right[2];
        if (i >= 24000 && i < 24048) { first += moving * moving; abrupt += hard * hard; }
        if (i >= 31200) { settled += residual * residual; change += hard * hard; }
    }
    expect(abrupt > 1.0e-8 && std::sqrt(first / abrupt) < 0.06,
           "RoomMicrophones", "Width glides over 20 ms rather than jumping", std::sqrt(first / abrupt));
    expect(change > 1.0e-8 && std::sqrt(settled / change) < 0.002,
           "RoomMicrophones", "Width reaches its new receiver response", std::sqrt(settled / change));

    // Clearing/resetting a pending movement takes the new configuration at
    // once, exactly like a freshly constructed room with those controls.
    rooms[1]->SetWidth(0.6);
    rooms[1]->Clear();
    rooms[2]->SetWidth(0.6, true);
    rooms[2]->Clear();
    bool reset = true;
    for (int i = 0; i < 4800; ++i) {
        double a, b, c, d;
        rooms[1]->Process(i == 0, i == 0, a, b);
        rooms[2]->Process(i == 0, i == 0, c, d);
        reset = reset && a == c && b == d;
    }
    expect(reset, "RoomMicrophones", "reset applies microphone targets immediately", reset ? 0.0 : 1.0);

    auto engine = std::make_unique<Engine>();
    engine->Prepare(48000.0);
    Controls controls;
    controls[kWidth] = 0.0;
    controls[kRoom] = 1.0;
    engine->SetControls(controls);
    const std::uint8_t note[3] { 0x90, 60, 100 };
    engine->Midi(note, 3);
    bool mono = true;
    double energy = 0.0;
    std::array<float, 64> left {}, right {};
    for (int block = 0; block < 375; ++block) {
        engine->Render(left.data(), right.data(), 64);
        for (std::size_t i = 0; i < left.size(); ++i) { mono = mono && left[i] == right[i]; energy += left[i] * left[i]; }
    }
    expect(mono && energy > 0.01, "RoomMicrophones", "Width zero is mono through the full wet engine", energy);
}

} // namespace respira_room_tests
