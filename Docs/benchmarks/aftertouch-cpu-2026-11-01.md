# Held aftertouch CPU — 1 November 2026

The Assist air-demand calculation now evaluates a key's raised-cosine
opening only when that value is used. A settled held key contributes exactly
one. Previously the cosine was evaluated on every host sample under
aftertouch even when its result was discarded; the Rack's `-fno-builtin`
prevents the compiler from removing that call. No state, physics, tuning or
voicing changes are involved.

Against canonical baseline `efdae07c198aa59817c4e31c0be0df3424c9dffb`, an
Apple M1 Max running macOS 26.5.1 and clang 21.0.0 measured these native
engine results. Both variants used C++17, `-O3 -DNDEBUG -fno-builtin`, 48 kHz,
64-frame blocks, Auto, Room 0.005 and rear 1x. Each process warmed for 0.5 s
and measured 2 s. Eight ABBA/BAAB groups per case interleaved two baseline
and two candidate processes, with paired wall/thread clocks per block.

| Case | Median candidate/baseline thread time | 95% bootstrap interval of mean paired ratio |
| --- | ---: | ---: |
| Master, 28 chambers | 1.00237 | 0.98976–1.00351 |
| Same, channel pressure 0.7 | 0.97474 | 0.97425–0.97973 |
| Master chord changes, up to 56 chambers | 0.99907 | 0.98884–1.00722 |

The aftertouch case used about 2.5% less thread CPU: median repeat cost was
14.61% versus 14.25% of one core. All eight paired groups favored it. An
independent three-group pilot found ratios 0.97406–0.97489. Ordinary playing
and the higher-quality control cases did not establish a reliable gain.

Twelve dynamic stereo sequences were compared byte for byte at 22.05,
44.1, 48 and 96 kHz with all three initial quality settings. Low/high notes,
bass/chord buttons, aftertouch, active register/quality changes, master
tuning, bend, Air, Auto/Wheel, Width, Room, release and reset were exercised.
All float bytes matched the baseline, as did the timed cases' output hashes.
The engine regression suite and deterministic fuzz replay passed with the
same C++17 Release flags.

These are offline engine measurements under the machine load at the time.
Thread time still depends on core selection, frequency and contention. The
resampling interval describes these groups, not immunity to system drift.
The Rack wrapper, target translation, host scheduling, initial attacks in
warmup and the usual eight-second Auto reversal were outside the timed
window. This is neither a general CPU reduction nor a deadline-free host
claim. The retained source change has no audible result to choose by ear.
