# CPU in the consolidation pass

Measured on 0.1.21 (`b8a4436`) to 0.1.24. The question was the portfolio
ceiling: no 256-frame block above 10 % of one core at default settings.
Respira does not meet it, and no numerically neutral change found here moves
it. The plan from here is
[`Docs/plans/cpu-ceiling-2026-11-09.md`](../plans/cpu-ceiling-2026-11-09.md).

## How it was measured

Intel Xeon @ 2.10 GHz, 4 threads, a cloud container shared with four other
sessions; GCC 13.3, Release (`-O3`), plain x86-64 (no `-march`, so SSE2, no
FMA or AVX), the configuration the Rack build's `llc` targets
(x86-64 `+sse3`). Development-machine figures, never Reason's or a DAW's.

- **Instruction counts** (`valgrind --tool=callgrind`, `Ir`) decide: they do
  not move with the machine's load.
- **Block times** come from a harness that renders each demo phrase through
  its preset as `RenderRespiraPresetDemos` does (48 kHz, 256-frame blocks,
  Auto, the preset's room), three times over, and keeps each block's
  shortest time. A block's share is its time over 256 / 48000 s. The harness
  also hashes the float output, so every timing run doubles as a
  bit-exactness check.
- `RenderRespiraPresetDemos --cpu` (one pass, no best-of) is what
  `Respira.CpuBudget` runs; its figures are the first table.

## Where 0.1.21 stood

`RenderRespiraPresetDemos --cpu`, one pass on the loaded machine:

| Demo | Mean, × realtime | Worst block |
| --- | --- | --- |
| Clarinet Solo | 0.029 | 8.5 % |
| Bassoon Low | 0.061 | 13.4 % |
| Violin Folk | 0.055 | 14.1 % |
| Paris Musette | 0.063 | 14.8 % |
| Tango Bandoneon | 0.064 | 18.9 % |
| Organ Bellows | 0.171 | 34.5 % |
| Bright Oboe | 0.079 | 24.0 % |
| Master Tutti | 0.322 | 78.3 % |

## Where 0.1.23 stands, per block

Best of three per block. "Half rate" is the listening candidate described
below, not the shipping engine.

| Demo | Shipping: mean | worst | sounding cells at the worst | Half rate: mean | worst |
| --- | --- | --- | --- | --- | --- |
| Clarinet Solo | 2.55 % | 10.34 % | 1 | 2.13 % | 8.79 % |
| Bassoon Low | 4.92 % | 12.32 % | 9 | 2.76 % | 8.20 % |
| Violin Folk | 4.67 % | 12.17 % | 10 | 3.15 % | 9.44 % |
| Paris Musette | 6.15 % | 15.86 % | 12 | 3.86 % | 10.85 % |
| Tango Bandoneon | 5.58 % | 19.11 % | 10 | 3.55 % | 13.23 % |
| Organ Bellows | 12.95 % | 23.72 % | 16 | 7.49 % | 16.45 % |
| Bright Oboe | 5.28 % | 20.72 % | 14 | 3.50 % | 14.71 % |
| Master Tutti | 18.74 % | 49.35 % | 44 | 11.62 % | 26.52 % |

Repeated runs under other load put Master Tutti's worst block at 57–60 %
(half rate 30–34 %) and Organ Bellows' at 28.6–29.2 % (13.6–16.8 %); the
ranking does not change. The worst block is a held chord, not a note-on, in
every demo but the single-reed one, whose worst blocks are bellows turns
(see Folds).

The differences from the 0.1.21 table are the measure (best of three against
one pass), not a speed-up: 0.1.22 and 0.1.23 change no cost in a sounding
block.

## Where the instructions go

Master Tutti's demo on 0.1.21, 20.907 G instructions:

| Part | Share |
| --- | --- |
| The reed cells (`ReedModel.h`: two tongues, the duct, the vent) | 64.9 % |
| of which the duct's sections | about 32 % |
| `Engine::Render`'s own body (per-sample breath, mixing, the bellows) | 19.5 % |
| `DspMath.h` (square roots, clamps) | 5.7 % |
| The room | 3.1 % |
| The folds | 2.9 % |

A sounding cell costs about 640 instructions per internal step in the engine
(426 in the cell step alone, about 34 ns), 192 000 times a second: 1.1–1.3 %
of a core per sounding cell at the worst block. Ten per cent of a core is
therefore six or seven sounding cells, and Master Tutti's chords hold 44.

## What 0.1.23 saved

With flush-to-zero off, three states decayed into subnormal numbers and
stayed there (`0f2f71f`): the key bed's modes above 2 kHz from 1.4 to 2.8 s
after every landing, the air meter's smoothing from 7 s after the last note
(for good) and Life's share of movement from 28 s after the bellows stop (for
good). Each cost 0.3–0.6 % of a core in paired block timings, flush-to-zero
off against on. All three now end at zero; the demos render to the same
files as 0.1.22. This saves between notes and at rest: no block of a held
chord got cheaper.

## Neutral levers measured, none shipped

Each was measured on the portable build. None pays, so none was committed;
the figures are here so they need not be measured again.

| Lever | Instructions | Share of the Tutti demo | Verdict |
| --- | --- | --- | --- |
| The manual hand's `pow(u, 0.48)`, per sample while a key is held and the hand is slow | 24.0 M | 0.115 % | libm, not on the Auto path; its cost in Reason's runtime is unknown |
| The pallet's `cos(pi * open)`, per internal step while a pallet moves | 8.65 M | 0.041 % | same |
| `MakeVent` while a pallet moves | 7.3 M | 0.035 % | too small to cache |
| Note-on: `Engine::Midi` inclusive, of which `StartCell` designs the cells | 0.51 M | 0.002 % | designing cells ahead saves nothing and would add a worse block at a Musette or Life change |
| Two sounding cells per 2-lane `double` vector | 440 against 426 per cell-step | +3.3 % | bit-identical, slower |
| Anything per sample that only changes per block | none found | | the per-sample breath setup (167.8 M, 0.80 %) varies per sample |

**Two cells per vector.** A pair step was written with GCC/Clang generic
vector types (`double` × 2, `vector_size(16)`), bitwise selects for the
branches and `_mm_sqrt_pd`, keeping the scalar step as the reference. Its
output is bit-identical to two scalar steps (division and square root are
exact per lane; no fused multiply-add on this build). On 1.824 M cell-steps
it took 803.3 M instructions against the scalar step's 777.3 M, and the best
of six alternating one-second runs was 35.0 ns against 34.0 ns per cell-step.
The step is bound by instruction throughput, not by its dividers: the scalar
code already runs two tongues' independent chains side by side in the
out-of-order core, and the selects execute both sides of every branch the
scalar step predicts. A structure-of-arrays rewrite of the whole cell loop
was estimated at about 5 % and not attempted. The sources are in
`/mnt/project-files/consolidation-2026-11-09/respira/cell-pair-experiment/`.
[Five CPU steps](five-cpu-steps-2026-11-05.md) already recorded that stepping
two cells interleaved gained nothing.

Whether the Rack build's `llc` lowers a vector square root to `sqrtpd` is
unverified until a U45 is built; this result does not depend on it.

## The folds

In the single-reed demo the worst blocks are bellows turns. One sounding
cell costs 2.4 % of a core; for 0.6–0.8 s after each turn the block costs
5.4 %, with a peak of 8.8–10.3 %. The cause: the 144 fold panels that flip at
a turn stay "moving" while the pressure ramps, because they track it
quasi-statically above the settle threshold of 0.1 mm/s. An exact speed-up
of the bookkeeping is worth about a tenth of the folds' cost; settling the
panels earlier changes when they click, which is audible in the mechanism
and was not done.

## The half-rate candidate

Stepping the cells at twice the host rate instead of four times
(`kBaseOversample = 2`, one decimator stage fewer, the tongue's stiffness
corrected for the step as 0.1.24 does at every rate):

- 9.461 G instructions against 17.083 G on a ten-key Master Tutti chord
  phrase, × 0.554;
- the block table above;
- below 8 kHz every phrase of the listening set is within 0.25 dB of the
  shipping engine. A piccolo reed held loud gains 1.9 dB at 12–16 kHz and
  5.8 dB at 16–20 kHz (34 and 52 dB under its own 2–4 kHz band); a musette
  chord gains 0.8, 1.5 and 0.9 dB at 8–12, 12–16 and 16–20 kHz; a low bass
  reed from pianissimo stays within 0.2 dB. No discrete aliased tone stands
  above the breath's floor (−96 dBFS per bin);
- Clarinet Solo, every key at 380 Pa: level within 0.4 dB, pitch within
  0.15 cents;
- the top reed needs more air: B7 starts at 230 Pa against 213 Pa, and at
  190 Pa it is 8.5 dB quieter than the shipping engine's −63 dBFS.

It is audible in principle and has more than one defensible outcome, so it
is a listening set (`/mnt/project-files/consolidation-2026-11-09/respira/half-rate/`),
not a commit. Even adopted, it leaves Master Tutti near 27–34 %.

## 0.1.24 costs nothing

The step-rate stiffness (`StepStiffness`) is computed when a cell is
designed and when Oversampling changes; the cell step reads one field in
place of another. At 48 kHz with Oversampling 1× it returns the stiffness
unchanged, and all eight demos render bit-identical to 0.1.23.
