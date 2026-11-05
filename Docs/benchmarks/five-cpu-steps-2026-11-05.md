# Five CPU steps: idle, the room, the folds and silent filters

Baseline: 0.1.15, `5e278df`. Five commits, each a CPU change that leaves the
sound as it was:

| Commit | Change |
| --- | --- |
| `9812e83` | With no reed sounding the internal steps do nothing; only the key groups in use are zeroed; the knock and valve-slap loops stop at the last active entry. |
| `2f23c31` | The room rests while it holds only silence: it turns its drift phasors and counts samples, and its write positions catch up when sound returns. Its buffers store values below the smallest normal float as zero, which ends a subnormal limit cycle that kept it running for ever. |
| `4a7f60b` | The room's per-line state is one array per quantity instead of beside each 64 kB buffer; the lines and the diffusers share their common write position. |
| `091b99a` | A moving fold panel starts its hinge step from the force its last step already took, and the step length is divided once per sample for all panels. |
| `4204857` | The mechanics' body filters, the bass box and the microphones' integrals are cleared after every block once all their states are below 1e-30, as they already were in silence: through a phrase they sat in subnormals. |

## Sound

A harness rendered every demo phrase through its preset, and 13 variants:
44.1, 96 and 22.05 kHz, block sizes 37-512, Oversampling 0 and 2, Life,
Touch, Room and Musette at their limits, Auto off, the air button, pitch bend
and channel pressure, and three phrases repeated after 18-25 s of silence
(Room 1, 44.1 kHz, 96 kHz with Life 1). 18 of the 21 renders are
bit-identical to the baseline after every step. The three with a long
silence differ only from step 2, by at most 6.2e-38 (-744 dBFS): the
baseline's output carried the room's subnormal residue through the silence.
`Respira.AudioDemos` and the unit tier pass.

## CPU

Intel Xeon @ 2.10 GHz (4 threads, shared cloud container), GCC 13.3,
Release. Development-machine figures, not Reason's.

`RenderRespiraPresetDemos --cpu --repeats 5`, baseline and final run
alternately twice, the lower median of each, as realtime factors:

| Demo | Before | After | Change |
| --- | ---: | ---: | ---: |
| Clarinet Solo | 0.0381 | 0.0318 | -16.6 % |
| Bassoon Low | 0.0623 | 0.0529 | -15.2 % |
| Violin Folk | 0.0644 | 0.0575 | -10.8 % |
| Paris Musette | 0.0735 | 0.0680 | -7.4 % |
| Tango Bandoneon | 0.0638 | 0.0558 | -12.6 % |
| Organ Bellows | 0.1601 | 0.1570 | -1.9 % |
| Bright Oboe | 0.0610 | 0.0561 | -8.1 % |
| Master Tutti | 0.2445 | 0.2412 | -1.4 % |

An idle engine at 48 kHz in 256-frame blocks, Init, best of five 20 s runs:

| Idle | Before | After |
| --- | ---: | ---: |
| From a reset | 1.67 % of a core | 0.48 % |
| 30 s after a 2 s note | 2.03 % | 0.81 % |

Measured alone, the room fell from 1.04 % to 0.65 % of a core and the folds,
in a phrase that turns the bellows, from 0.80 % to 0.69 %. Repeated timings
of one binary vary by about 3 % here.

## Tried and dropped

Dense chords are bound by each reed's serial chain of square roots and
divisions. None of these moved the Master Tutti demo beyond that 3 %, so none
was kept: fusing the chamber's three section loops (and carrying the
tongues' pressure to the next step), splitting them into vectorisable
passes, skipping the divisions while a tongue is in its slot, stepping two
cells interleaved, stepping cells grouped by chamber length, taking every
cell's reeds before every cell's chamber, and skipping a silent reed at rest
(the chamber's pressure ripple wakes it within a few steps, even with Auto).
