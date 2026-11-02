# Reusing the active reed list within an audio callback

Baseline: 0.1.6, `f9057b33cbfa0c5230131e44862f4cced159bd2d`.
This is a separate local CPU change; the microphone-motion listening
candidate and its A/B audio remain frozen. No A/B approval is implied.

## Change

`Engine::Render` used to scan all 72 possible cells on every host sample.
Notes, register changes and resets occur between Render calls; inside a
call, cells can only become inactive as releases finish. The engine now
builds its ordered active list once at callback entry and compacts that list
after processing each sample. A retired cell disappears on the same next
sample as before. Diagnostics retain their previous one-sample convention.

Reed equations, precision, oversampling, note timing, random draws, summation
order, presets, filters and output processing are unchanged. The list is
stack storage already used by the old render loop; no allocation or
persistent engine state was added.

## Paired native measurements

Apple M1 Max, arm64, Apple Clang 21, C++17 `-O3 -fno-builtin
-fno-exceptions`. Each process warms for 0.5 s and measures 2 s of audio at
48 kHz in 64-frame callbacks. Life and Touch are 100%, automatic bellows,
default quality. Five baseline/trial pairs alternate order per workload.
No other builds or renders from this task ran during these timings.

Numbers below are median **rendering-thread CPU as percent of one core**.
They are not Reason/target CPU or a promise of glitch-free playback.

| Workload | Active cells, maximum | Baseline | Trial | Relative change |
| --- | ---: | ---: | ---: | ---: |
| Idle | 0 | 3.198% | 2.708% | −15.33% |
| Init, one note | 1 | 6.220% | 5.485% | −11.83% |
| Paris Musette triad | 9 | 17.583% | 16.584% | −5.68% |
| Master with bass/chords | 28 | 45.821% | 45.315% | −1.10% |
| Master, changing chords and release overlap | 56 | 47.590% | 46.788% | −1.69% |

Idle, one-note and triad comparisons improved in every pair. The one-note
relative pair changes were −11.03% to −14.01%, and the triad −1.32% to
−6.68%. Dense workloads show smaller savings; the changing-chord workload
had one pair 0.35% slower. The supported practical claim is a useful
reduction for sparse playing, without an established large dense-chord gain.

Output hashes match baseline in all 50 timed renders. There were no
thread-time deadline overruns in the first four workloads. Changing chords
had 115 baseline versus 100 trial overruns across 7500 measured callbacks
per arm. Wall-time overruns occurred in every workload; sampled system load
and both timing distributions are retained. Scheduling interference and
note-start work are not fixed by the active-list optimization.

## Audio identity and coverage

Twelve 2 s stereo float renders match baseline **byte for byte** at
22.05/44.1/48/96 kHz and three initial quality settings. The shared fixture
includes treble/bass/chord notes, releases, aftertouch, register and quality
changes while active, the air valve, wheel/Auto transitions, pitch bend,
master tuning, Room/Width changes and Reset.

Six additional stereo renders match byte for byte at actual callback sizes
1, 7, 64, 256, 512 and 4096 frames. Their fixture fills all 72 cells, steals
voices, releases overlapping notes and reaches zero active cells. This
comparison includes all tiny tail samples: **no tolerance is applied to the
baseline/trial identity checks**.

## Correcting the block-partition test fixture

The old `BlockPartitionInvariance` called `Play()`, whose innermost loop
always calls `Render(..., 1)`. Its `block` argument grouped the test loop
without changing the renderer's callback length. It did not prove the
coverage its name and output claimed.

The replacement sends the same events through actual callbacks and asserts
that every requested callback size is reached and all 72 cells are exercised.
It initially required exact bits across different partitions. That exposed
the **same pre-existing failure in shipping and trial engines**: silent
callbacks deliberately flush filter states below 1e-20 at callback end.
The largest difference is 9.05229324e-22 (about −421 dBFS), with identical
failure reports from both engines. An initial exception based on both
sample magnitudes being below the state cutoff was also too strict: mixing
can put the output just above that per-state cutoff (up to 1.19476e-20 in
the differing samples).

The corrected cross-partition contract bounds **absolute output error** by
the existing 1e-20 cleanup floor, with the reason beside the assertion.
This is a correction of the previously unexercised fixture assumption,
not an allowance for new optimization error. The independent same-partition
baseline/trial checks still require every byte to match. Both initial failed
test runs and the unchanged-baseline diagnosis are retained.

## Validation and delivery

Release build and `RespiraDSPCxx17` pass, including `-fno-builtin` and
`-fno-exceptions`. Touch, Life, ReedTuning, DspFuzzReplay, DemoMidiSources
and AudioDemos passed in the first suite run. After the documented fixture
correction, the complete Engine suite passes in 95.22 s. Those results cover
all six unit tests and the audio tier; the earlier Engine failures remain
recorded and are not described as passes.

The official `Respira.CpuBudget` passes its first run for this candidate
(42.37 s, 48 kHz, up to 256 frames, three repetitions of every factory
phrase). The largest median real-time factor is 0.348583492 for Master
Tutti, below the unchanged 0.75 budget. The largest wall-block share is
1.76356256, so this pass does not establish zero deadline misses. This
separate elapsed-time gate is not used to derive the paired speedup above.

No Rack build, install, publication or push is part of this local
optimization. Its unshipped version number must follow the final release
order when the separate microphone candidate is resolved.

## Raw evidence

Owner workspace: `diagnostics/respira-active-cells-20261102` under the Rack
workspace. It contains `cpu-probe.cpp`, `measure-cpu.py`, `cpu-preflight.json`,
`cpu-raw.json`, `cpu-summary.json`, both identity suites and their hashes,
the initial failing fixture/patch/logs, baseline diagnosis, build logs and
gate results. No failed run is replaced by a successful one. All public
engine/test changes are in this commit; local research records stay outside
the repository.
