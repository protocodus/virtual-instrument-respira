#pragma once

// What stands between the reeds and the microphones besides the open air: the
// air inside the bellows, and the player and the instrument's own halves.
//
// Why it is here. A recorded reed is not the model's reed (README, Known
// gaps): on La Melodiosa's single reeds (Petri Pohjanmies, CC BY; 24 held
// notes, D#4-C7) the 2nd to 12th harmonics scatter 3.5-8 dB about a straight
// fall (median 5.7), where the model's scattered 2-4.7 and its 2nd and 3rd
// sat at -5 dB on every key; over a held note each harmonic moves 1.9 dB
// against the fundamental (standard deviation, the median; 0.7-4.8 from the
// 10th to the 90th percentile) and the harmonics' levels correlate 0.2 with
// it, where the model's moved 0.35 dB (0.2-0.8) and correlated 0.99. The push and the pull reed of one key, two tongues
// behind one pallet, agree within 1-4 dB on most harmonics where neighbouring
// keys differ by 20: the scatter belongs to the path, not to the tongue. And
// of a tremolo pair each partial beats to its own depth (a Hohner's first
// four, Marek Kintosh's samples: 0.62, 0.28, 0.44, 0.43 on one key; La
// Melodiosa's 0.23, 0.17, 0.58, 0.52), where the model's pair, two like reeds
// through one path, beat to one depth on every partial (0.93, 0.91, 0.91,
// 0.91), which is what two detuned oscillators do (Tools/measure_partials.py).
//
// The bellows' air. A reed's pulses of air leave the bellows as they enter
// the chamber, so the same flow sounds into the bellows with the other sign.
// The bellows are a box of air 0.41 x 0.19 m and as deep as they stand open
// (RespiraEngine.cpp, kBellowsVolume), whose walls are card and cloth of about
// 1 kg/m2 (Mechanics.h): by the mass law those pass what is in the box almost
// whole at 220 Hz and 6 dB less each octave above. Its air rings along its
// length (418 Hz and its multiples), across it (903 Hz) and through its depth,
// which the hand moves: every note is played on bellows that open or close,
// so the box under a held note is never the same box twice. Three delay
// loops, one each way, exchange their sound at the walls (a Householder
// reflection) and lose a quarter of it on each pass, more of the treble; each
// rank's reeds stand at their own place in the box, on their own block, so
// each drives the three ways by its own shares. Below its first mode the box
// is the spring the reeds already blow against (the bellows pressure), so
// what leaves is taken from 350 Hz up. It leaves through the folds, between
// the two halves.
//
// The player and the instrument. The treble's sound also reaches the pair by
// way of the player's lap under the keyboard, the chest and arm behind it,
// and the bass half beyond the bellows, which moves with them. Each is one
// reflection, later and softer than the direct sound and duller off cloth,
// from its own direction; each rank's holes lie 15 mm from the next under the
// pallet, so each rank's reflections arrive a little apart.
//
// Lengths are a full-size instrument's on a seated player, estimated. Levels
// are fitted to the recorded figures above, and Life scales them as it scales
// every other departure from the ideal instrument: none at Life 0, where the
// reeds sound in free air as before, these at 0.5, twice at 1. With them the
// model's harmonics scatter 5.0 dB and move 1.12 dB against the fundamental
// (correlation 0.66), and a pair's first four partials beat 0.090 apart in
// depth (0.018 before; the Hohner's 0.125). The owner chose these by ear
// (Docs/decisions.md, 2026-11-20) over 1/1.6 of them, 0.7.0's, which read
// 4.0 dB, 0.78 dB, 0.82 and 0.064, and over none. The recordings' rooms are
// in their figures too, and no fit can tell them from the instrument.

#include "DspMath.h"

#include <array>
#include <cmath>

namespace respira {

struct NearField {
    static constexpr int kRanks = 5;          // the treble ranks, kRankL .. kRankH
    static constexpr int kSize = 1024;        // samples: 2.7 ms at 384 kHz
    static constexpr int kPaths = 3, kWays = 3;
    static constexpr double kSoundSpeed = 343.0;

    // A reflection: its path over the direct one (m) with the bellows half
    // open, how much of the bellows' travel and of the player's sway (sideways
    // and towards the pair) lengthens it, its level against the direct sound,
    // where it comes from across the pair (-1 left .. 1 right, against the
    // treble's own place), how the ranks' row lies along it, and whether its
    // length is the instrument's size (Frame, reach): the lap and the bass
    // half are as far as the instrument is large, the chest as near behind
    // any keyboard.
    struct Path { double metres, travel, side, depth, level, pan, row; bool sized; };
    static constexpr std::array<Path, kPaths> kPath { {
        { 0.30, 0.0, 0.2, 0.5, 0.30, 0.00, 0.9, true },      // the lap under the keyboard
        { 0.12, 0.0, 1.0, -0.4, 0.26, 0.15, -0.7, false },   // the chest and right arm behind it
        { 0.42, 1.0, -0.6, 0.3, 0.22, -0.45, 0.5, true },    // the bass half, beyond the bellows
    } };
    // The ranks' holes under one pallet, m from the 8' at pitch.
    static constexpr std::array<double, kRanks> kRow { { -0.030, 0.0, -0.015, 0.015, 0.030 } };
    static constexpr double kClothCorner = 3500.0;   // Hz: clothes and leather take the top off a reflection

    // The bellows' box: its length and width (kPleatLongWall, kPleatShortWall)
    // and its depth closed and open, the frames' own depth plus the travel.
    // These are the Default build's; a build of another size brings its own
    // (Frame, SetFrame).
    static constexpr double kBoxLength = 0.41, kBoxWidth = 0.19, kBoxClosed = 0.15, kBoxTravel = 0.077;
    static constexpr double kBoxKeeps = 0.75;         // of the sound, each pass
    static constexpr double kBoxCorner = 3000.0;      // Hz: the card and cloth absorb the treble first
    static constexpr double kWallCorner = 220.0;      // Hz: the mass law of 1 kg/m2
    static constexpr double kBoxFrom = 350.0;         // Hz: below its first mode the box is the bellows' spring
    // Each rank's share of the three ways (length, width, depth) by where its
    // block stands: the 16' and the 8' at pitch on the inner blocks, the
    // tremolo reeds and the 4' on the outer ones (ReedBank.h, Variation); the
    // bass side's blocks at the other end of the box.
    static constexpr std::array<std::array<double, kWays>, kRanks + 1> kShare { {
        { 0.9, 0.5, 0.7 }, { 0.6, -0.8, 0.9 }, { -0.7, 0.9, 0.5 }, { 0.8, 0.7, -0.6 }, { -0.5, -0.6, 0.8 },
        { 0.7, 0.6, -0.9 },
    } };
    // The instrument's own size, which the build brings (ReedBank.h, Build):
    // the bellows' section and their depth closed, m, and how far the
    // player's lap and the bass half lie from the treble against the
    // Default's, since a smaller instrument sits closer to both. `flat` marks
    // the ranks whose blocks lie flat in a tone chamber, or whose plates lie
    // on the reed board with no block at all, turned a quarter against the
    // standing blocks: they drive the box's ways in another order. Every
    // build's bellows hold the same air over their travel (RespiraEngine.cpp,
    // kBellowsVolume), so a narrower section travels further.
    struct Frame {
        double length = kBoxLength, width = kBoxWidth, closed = kBoxClosed, reach = 1.0;
        std::array<bool, kRanks> flat {};
    };
    // Levels at Life 0.5, over each path's own and the box's: the owner's
    // choice by ear (above). 0.7.0 had 1.0 and 0.5.
    static constexpr double kPathLevel = 1.6, kBoxLevel = 0.8;

    std::array<std::array<float, kSize>, kRanks> rank {};
    std::array<std::array<float, kSize>, kWays> way {};
    std::array<double, kWays> loss {};
    int at = 0;
    double rate = 48000.0, perMetre = 0.0;
    double travel = 0.5, side = 0.0, depth = 0.0, spread = -1.0;   // gliding
    double travelTarget = 0.5, sideTarget = 0.0, depthTarget = 0.0;
    double glide = 0.0, clothStep = 0.0, boxStep = 0.0, wallStep = 0.0, fromStep = 0.0;
    double clothL = 0.0, clothR = 0.0, clothLC = 0.0, clothRC = 0.0, wall = 0.0, from = 0.0;
    double pathScale = 1.0, boxScale = 1.0;   // other levels over these, for tests and listening sets
    // The build's frame, gliding where a length can: a new build under held
    // keys moves the box, it does not cut it.
    double length = kBoxLength, width = kBoxWidth, closed = kBoxClosed, boxTravel = kBoxTravel, reach = 1.0;
    double lengthTarget = kBoxLength, widthTarget = kBoxWidth, closedTarget = kBoxClosed, boxTravelTarget = kBoxTravel, reachTarget = 1.0;
    double boxGain = 1.0, fromHz = kBoxFrom;
    std::array<std::array<double, kWays>, kRanks + 1> share = kShare;

    // A smaller box rings higher and louder: the same reed's flow makes more
    // pressure in less air, through less wall, so what leaves goes as one
    // over the box's size, the cube root of its volume against the
    // Default's. Below its own first mode it is the bellows' spring, as the
    // Default's is below 350 Hz, 0.84 of its first mode.
    void SetFrame(const Frame& frame) {
        lengthTarget = frame.length;
        widthTarget = frame.width;
        closedTarget = frame.closed;
        boxTravelTarget = kBoxTravel * (kBoxLength * kBoxWidth) / (frame.length * frame.width);
        reachTarget = frame.reach;
        const double deep = closedTarget + 0.5 * boxTravelTarget, usual = kBoxClosed + 0.5 * kBoxTravel;
        boxGain = std::cbrt(kBoxLength * kBoxWidth * usual / (frame.length * frame.width * deep));
        fromHz = kBoxFrom * Max(kBoxLength, usual) / Max(Max(frame.length, frame.width), deep);
        fromStep = 1.0 - std::exp(-6.283185307179586 * Min(fromHz, 0.45 * rate) / rate);
        for (int r = 0; r < kRanks; ++r)
            for (int w = 0; w < kWays; ++w)
                share[static_cast<std::size_t>(r)][static_cast<std::size_t>(w)] =
                    kShare[static_cast<std::size_t>(r)][static_cast<std::size_t>(frame.flat[static_cast<std::size_t>(r)] ? (w + 1) % kWays : w)];
    }
    void TakeFrame() {
        length = lengthTarget;
        width = widthTarget;
        closed = closedTarget;
        boxTravel = boxTravelTarget;
        reach = reachTarget;
    }
    bool quiet = true;

    void Prepare(double sampleRate) {
        rate = sampleRate;
        perMetre = rate / kSoundSpeed;
        const auto step = [&](double hz) { return 1.0 - std::exp(-6.283185307179586 * Min(hz, 0.45 * rate) / rate); };
        glide = step(8.0);
        clothStep = step(kClothCorner);
        boxStep = step(kBoxCorner);
        wallStep = step(kWallCorner);
        fromStep = step(fromHz);
        Clear();
    }

    void Clear() {
        for (auto& r : rank) r.fill(0.0f);
        for (auto& w : way) w.fill(0.0f);
        loss.fill(0.0);
        clothL = clothR = clothLC = clothRC = wall = from = 0.0;
        travel = travelTarget;
        side = sideTarget;
        depth = depthTarget;
        spread = -1.0;
        TakeFrame();
        quiet = true;
    }

    // Where the bellows stand (0 closed .. 1 open) and where the player has
    // swayed to (m, to the right and towards the pair).
    void Move(double bellows, double sideways, double towards) {
        travelTarget = bellows;
        sideTarget = sideways;
        depthTarget = towards;
    }

    static double Read(const std::array<float, kSize>& line, int at, double samples) {
        samples = Clamp(samples, 1.0, static_cast<double>(kSize - 2));
        const int whole = static_cast<int>(samples);
        const double part = samples - whole;
        const double a = line[static_cast<std::size_t>((at - whole) & (kSize - 1))];
        const double b = line[static_cast<std::size_t>((at - whole - 1) & (kSize - 1))];
        return a + part * (b - a);
    }

    // One host sample. `in` holds each treble rank's sound at the pallet
    // holes and `inside` whether it sounds into a tone chamber; `bass` is the
    // bass side's. `amount` is Life's scale (0, 1 at Life 0.5, 2) and `apart`
    // how far the pair's axes stand apart against the usual 90 degrees (0 one
    // microphone, which hears every direction alike). Out come
    // the reflections to the left and the right of the treble's place, for
    // the open ranks and the tone chamber's, and what leaves the bellows' box.
    struct Out { double left = 0.0, right = 0.0, leftInside = 0.0, rightInside = 0.0, box = 0.0; };
    Out Process(const double* in, const bool* inside, double bass, double amount, double apart) {
        Out out;
        if (amount <= 0.0) {
            if (!quiet) Clear();
            return out;
        }
        if (quiet) {   // nothing in it and nothing coming: no work
            bool any = bass != 0.0;
            for (int r = 0; r < kRanks; ++r) any = any || in[r] != 0.0;
            if (!any) return out;
        }
        quiet = false;
        travel += glide * (travelTarget - travel);
        side += glide * (sideTarget - side);
        depth += glide * (depthTarget - depth);
        // Width glides as the pair's other gains do, so automating it never steps.
        spread = spread < 0.0 ? apart : spread + glide * (apart - spread);
        length += glide * (lengthTarget - length);
        width += glide * (widthTarget - width);
        closed += glide * (closedTarget - closed);
        boxTravel += glide * (boxTravelTarget - boxTravel);
        reach += glide * (reachTarget - reach);
        at = (at + 1) & (kSize - 1);
        std::array<double, kWays> drive {};
        for (int r = 0; r < kRanks; ++r) {
            rank[static_cast<std::size_t>(r)][static_cast<std::size_t>(at)] = static_cast<float>(in[r]);
            for (int w = 0; w < kWays; ++w) drive[static_cast<std::size_t>(w)] += share[static_cast<std::size_t>(r)][static_cast<std::size_t>(w)] * in[r];
        }
        for (int w = 0; w < kWays; ++w) drive[static_cast<std::size_t>(w)] += share[kRanks][static_cast<std::size_t>(w)] * bass;
        const double level = amount * kPathLevel * pathScale;
        for (const Path& p : kPath) {
            const double metres = p.metres * (p.sized ? reach : 1.0) + p.travel * boxTravel * (travel - 0.5) + p.side * side + p.depth * depth;
            double open = 0.0, chamber = 0.0;
            for (int r = 0; r < kRanks; ++r) {
                const double v = Read(rank[static_cast<std::size_t>(r)], at, (metres + p.row * kRow[static_cast<std::size_t>(r)]) * perMetre);
                (inside[r] ? chamber : open) += v;
            }
            const double l = level * p.level * (1.0 - spread * p.pan), r = level * p.level * (1.0 + spread * p.pan);
            out.left += l * open;
            out.right += r * open;
            out.leftInside += l * chamber;
            out.rightInside += r * chamber;
        }
        clothL += clothStep * (out.left - clothL);
        clothR += clothStep * (out.right - clothR);
        clothLC += clothStep * (out.leftInside - clothLC);
        clothRC += clothStep * (out.rightInside - clothRC);
        out.left = clothL;
        out.right = clothR;
        out.leftInside = clothLC;
        out.rightInside = clothRC;
        // The box: there and back along each way.
        const std::array<double, kWays> there { { 2.0 * length, 2.0 * width, 2.0 * (closed + boxTravel * travel) } };
        std::array<double, kWays> back {};
        double sum = 0.0;
        for (int w = 0; w < kWays; ++w) {
            back[static_cast<std::size_t>(w)] = Read(way[static_cast<std::size_t>(w)], at, there[static_cast<std::size_t>(w)] * perMetre);
            sum += back[static_cast<std::size_t>(w)];
        }
        for (int w = 0; w < kWays; ++w) {
            const auto i = static_cast<std::size_t>(w);
            loss[i] += boxStep * (kBoxKeeps * (back[i] - 2.0 / kWays * sum) - loss[i]);
            way[i][static_cast<std::size_t>(at)] = static_cast<float>(drive[i] + loss[i]);
        }
        wall += wallStep * (sum - wall);
        from += fromStep * (wall - from);
        out.box = -amount * kBoxLevel * boxScale * boxGain * (wall - from);
        return out;
    }

    void FlushTiny() {
        if (quiet) return;
        for (double v : { clothL, clothR, clothLC, clothRC, wall, from, loss[0], loss[1], loss[2] })
            if (Abs(v) >= 1.0e-30) return;
        for (const auto& w : way)
            for (float v : w)
                if (Abs(v) >= 1.0e-30f) return;
        Clear();
    }
};

} // namespace respira
