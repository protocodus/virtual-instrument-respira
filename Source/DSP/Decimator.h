#pragma once

// 4x to 1x decimation: two cascaded half-band stages, each a two-path
// polyphase allpass. The same structure and coefficients as Maremba's
// Oversampler.h (protocodus/virtual-instrument-maremba), which the Respira
// prototype was voiced with.

#include "DspMath.h"

#include <initializer_list>

namespace respira {

class Decimator4x {
public:
    void Reset() { *this = Decimator4x {}; }

    float Process(const float* in) {
        const float a = stage[0].Decimate(in[0], in[1]);
        const float b = stage[0].Decimate(in[2], in[3]);
        return stage[1].Decimate(a, b);
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

    Halfband stage[2];
};

} // namespace respira
