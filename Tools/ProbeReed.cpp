// Plays single reed cells at a fixed bellows pressure and prints what each
// sounds and how much air it takes, for voicing and for the air figures
// quoted beside the code (ReedBank.h's airWeight).
//
//   ProbeReed [pressure Pa] [rank]
//
// Pressure defaults to the tuning pressure (400 Pa), rank to the treble M
// (8') rank. One line per octave from C2, plus A4 (per pitch class on the
// bass side): the key, the wanted and sounding pitch of the push reed, its
// error in cents and its mean air flow.
#include "ReedProbe.h"
#include "DSP/ReedTuning.h"

#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>

using namespace respira;

int main(int argc, char** argv) {
    const double pressure = argc > 1 ? std::atof(argv[1]) : kTuningPressure;
    const int rank = argc > 2 ? std::atoi(argv[2]) : kRankM;
    if (rank < 0 || rank >= kRankCount || !(pressure > 0.0)) {
        std::fprintf(stderr, "usage: ProbeReed [pressure Pa > 0] [rank 0..%d]\n", kRankCount - 1);
        return 2;
    }
    std::printf("rank %d at %.0f Pa\n key   wanted Hz  sounding Hz  cents   air L/s\n", rank, pressure);
    const std::array<int, 7> treble { 36, 48, 60, 69, 72, 84, 96 };
    for (int i = 0; i < (IsTreble(rank) ? 7 : 12); ++i) {
        const int key = IsTreble(rank) ? treble[static_cast<std::size_t>(i)] : i, k = key;
        const CellSpec spec = MakeCell(rank, k, 22.0, kReedTuning[rank][TuningIndex(rank, k)], probe::kRate);
        const double hz = probe::SoundingHz(spec.push, spec.chamber, spec.hz, pressure);
        // Mean air through the cell over its last half second, pallet open.
        CellState s;
        s.push.x = spec.push.rest;
        s.pull.x = spec.pull.rest;
        const double rate = probe::kRate, dt = 1.0 / rate;
        const int n = static_cast<int>(rate), from = n / 2;
        double air = 0.0, total = 0.0;
        for (int i = 0; i < n; ++i) {
            air = 0.0;
            StepCell(spec.push, spec.pull, spec.chamber, MakeVent(spec.chamber, std::fmin(1.0, i / (0.004 * rate)), rate), s, pressure, dt,
                     rate, 0.0, 0.0, air);
            if (i >= from) total += air;
        }
        std::printf("%4d  %9.2f  %11.2f  %6.1f  %8.3f\n", key, spec.hz, hz, hz > 0.0 ? 1200.0 * std::log2(hz / spec.hz) : 0.0,
                    total / (n - from) * 1.0e3);
    }
    return 0;
}
