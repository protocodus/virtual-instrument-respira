#pragma once

// 2^n to 1 decimation, n = 1..kMaxStages: cascaded half-band stages, each a
// two-path polyphase allpass. The same structure and coefficients as
// Maremba's Oversampler.h (protocodus/virtual-instrument-maremba), which the
// Respira prototype was voiced with.

#include "DspMath.h"

#include <initializer_list>

namespace respira {

class Decimator {
public:
    static constexpr int kMaxStages = 4;

    void Reset() { *this = Decimator {}; }

    // Decimates the 2^stages samples in `in`, overwriting them. Stage 0 is the
    // last (to the output rate), so a change of `stages` keeps the state of the
    // stages both settings share; the ones added start empty.
    float Process(float* in, int stages) {
        for (int s = stages - 1, n = 1 << stages; s >= 0; --s, n /= 2)
            for (int k = 0; k < n / 2; ++k) in[k] = stage[s].Decimate(in[2 * k], in[2 * k + 1]);
        return in[0];
    }

    // Zeroes states below about -400 dB so a silent input never leaves them
    // cycling on subnormal values.
    void FlushTiny() {
        for (auto& s : stage) s.FlushTiny();
    }

private:
    struct Allpass {
        float a = 0.0f, x1 = 0.0f, y1 = 0.0f;
        float Process(float x) {
            const float y = a * x + x1 - a * y1;
            x1 = x;
            y1 = y;
            return y;
        }
    };

    struct Halfband {
        Allpass path0 { 0.165728f }, path1 { 0.590836f };
        float z1 = 0.0f;
        // Two consecutive samples at 2 Fs in, one sample at Fs out.
        float Decimate(float s0, float s1) {
            const float y = (path0.Process(s0) + path1.Process(z1)) * 0.5f;
            z1 = s1;
            return y;
        }
        void FlushTiny() {
            for (float* v : { &path0.x1, &path0.y1, &path1.x1, &path1.y1, &z1 })
                if (Abs(*v) < 1.0e-20f) *v = 0.0f;
        }
    };

    Halfband stage[kMaxStages];
};

} // namespace respira
