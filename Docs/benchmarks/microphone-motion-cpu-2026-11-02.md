# Microphone motion CPU experiments

Baseline: 0.1.6, `f9057b33cbfa0c5230131e44862f4cced159bd2d`.
The earlier microphone-only experiments and failed gates are preserved below.
The owner subsequently chose **“B but tiny”** on 2026-11-02. The final
section records integration with the separate 0.1.7 CPU commit; native
measurements do not establish Reason performance or delivery approval.

## Protocol

Native arm64, Apple Clang 21, C++17 `-O3 -fno-builtin`; 48 kHz, 64-frame callbacks, default quality, Life 1 and Touch 1. Each case has 0.5 s warm-up and 2 s measured audio. Five trials rotate and reverse the order of the three variants. The table reports median rendering-thread CPU time as percent of one core; wall time is measured separately. This is not Reason/target CPU.

The machine also had other user-owned DSP jobs running. The logs retain wall and thread-CPU deadlines separately. Neither the trials nor their scheduling interference establishes glitch-free host playback.

## Empty-filter shortcut

| Case | Baseline CPU % | Mic candidate CPU % | CPU trial % | Trial vs mic candidate | Thread deadlines, baseline / candidate / trial |
| --- | ---: | ---: | ---: | ---: | --- |

| idle | 3.188 | 3.171 | 3.223 | +1.65% | 0 / 0 / 0 |

| init_single | 6.169 | 6.169 | 6.163 | -0.10% | 0 / 0 / 0 |

| master28 | 43.470 | 44.188 | 43.600 | -1.33% | 0 / 0 / 0 |

| master_changes | 45.718 | 46.462 | 44.929 | -3.30% | 34 / 59 / 32 |


Rejected: no demonstrated CPU benefit. Output hashes match the unoptimized microphone candidate in every timed case. The gap cache also passed twelve dynamic all-rate/all-quality bit comparisons; twelve Life-off comparisons matched 0.1.6. Both trial implementations have been removed.

## Reuse the first reed-gap measurement

| Case | Baseline CPU % | Mic candidate CPU % | CPU trial % | Trial vs mic candidate | Thread deadlines, baseline / candidate / trial |
| --- | ---: | ---: | ---: | ---: | --- |

| idle | 3.143 | 3.178 | 3.222 | +1.36% | 0 / 0 / 0 |

| init_single | 6.350 | 6.364 | 6.364 | +0.00% | 0 / 0 / 0 |

| master28 | 46.404 | 47.028 | 47.357 | +0.70% | 0 / 0 / 0 |

| master_changes | 47.157 | 47.033 | 48.286 | +2.66% | 91 / 82 / 100 |


Rejected: no demonstrated CPU benefit. Output hashes match the unoptimized microphone candidate in every timed case. The gap cache also passed twelve dynamic all-rate/all-quality bit comparisons; twelve Life-off comparisons matched 0.1.6. Both trial implementations have been removed.

## Final candidate

The retained sound candidate uses four one-pole states, with geometry updates at the existing 200 Hz Life control rate. A coefficient approaching zero after Life is turned off is snapped below 1e-20 to avoid a persistent denormal during held notes. The broad final validation and native CPU gate are recorded with the listening evidence. No CPU reduction is claimed.

## Final native gate and paired phrase diagnostic

The regular Release CMake `CpuBudget` uses 48 kHz, 256-frame maximum
callbacks and three timed repetitions of every factory phrase. Its
elapsed-time budget remains 0.75 times real time. On this Apple M1 Max,
the candidate failed Organ Bellows at 0.8359; the unchanged baseline then
failed five phrases, including Organ Bellows at 1.2182. These gate runs
are retained as failures. Their changing system load and sequential order
do not establish a speedup or regression.

To inspect that specific workload, `phrase-cpu-probe.cpp` keeps the canonical
renderer, MIDI, preset, event splitting and native Release library. It adds
thread-CPU measurements around `Voice::Midi` and `Voice::Render`. This
diagnostic uses C++20/native CMake Release flags, unlike the earlier
`-fno-builtin` optimization probes. Three alternating pairs each warm with
the complete Organ Bellows phrase and then measure it once.

| Measurement | Baseline | Candidate |
| --- | ---: | ---: |
| Median rendering-thread CPU, percent of one core | 25.2301% | 25.2515% |
| Rendering-thread CPU range | 24.5974–25.2431% | 24.9623–25.8810% |
| Median elapsed-time factor | 0.6897 | 0.8857 |
| Elapsed-time factor range | 0.6316–0.9162 | 0.7522–1.0546 |
| Worst measured wall block | 179.50 ms | 280.63 ms |

The paired median CPU difference is +0.085% relative, much smaller than
the trial spread. No reduction or meaningful regression is established by
this sample. Repeat hashes are stable within each variant. This diagnostic
reports aggregate callback CPU and wall-block maxima, not a full per-block
CPU distribution. The earlier 64-frame probes retain their own per-block
distributions and deadline counts.

These measurements cannot retroactively diagnose every block of the
earlier gate runs, replace their failures, or establish Reason performance.
The native elapsed-time performance gate was unresolved at that point.

## Retry after the system load changed

A single further baseline/candidate pair was declared after system load
dropped from over 100 to about 10, with an observed CPU idle fraction near
50%. Source-manifest hashes and the identical MIDI were checked before
running the existing binaries. No source, limit, repeat count or timeout
changed. Baseline ran first; the candidate ran through the unchanged CTest
`Respira.CpuBudget`. All earlier failures remain in the evidence folder.

The candidate passed all eight phrases (85.90 seconds for CTest). Its
largest median factor was Master Tutti at 0.6130, below the 0.75 budget;
Organ Bellows measured 0.3484. Baseline still failed Organ Bellows at
0.9356, with its other seven phrases passing. The one-minute load samples
ranged from 12.48–26.40 during baseline and 18.02–25.44 during candidate.
This sequential pair is not a controlled CPU comparison and does not
establish a speedup. The earlier alternating thread-CPU diagnostic remains
the comparison of processing cost.

The candidate therefore now has a passing native performance-tier run,
alongside the retained failed run. Its largest wall-block share was 6.47
times the callback period. The median-cost gate does not require every
block to meet its deadline; this pass does not prove glitch-free playback
or clear SDK/Reason host performance. Review audio and the DSP source are
unchanged. Raw commands, exit codes, binary hashes, per-phrase JSON and
five-second load samples are in `cpu-retry-20261102-0229/`.

## Combined with the separate active-list optimization

The 0.1.7 CPU commit `ae221afa18bfe5963d96619457ae68d6746a98fa` reuses the
ordered active-cell list within a Render callback. Combining it with the
unchanged microphone mechanism produces 0.1.8. No other DSP change is
included, and the original B WAV and its CSV trace are reproduced byte for
byte. Twelve dynamic renders match the microphone-only candidate exactly;
twelve Life-zero renders match the CPU-only engine exactly.

Five alternating pairs per workload, 48 kHz / 64 frames, 0.5 s warmup and
2 s measured audio, default quality, Life/Touch 1, Apple M1 Max, Clang 21,
C++17 `-O3 -fno-builtin -fno-exceptions`. The reference in this table is the
microphone candidate, not letter A of the listening comparison. Figures are
median rendering-thread CPU as a percentage of one core.

| Workload | Microphone candidate | Combined | Relative change |
| --- | ---: | ---: | ---: |
| Idle | 3.094% | 2.246% | -27.42% |
| One note | 6.192% | 5.477% | -11.55% |
| Musette triad | 17.384% | 16.916% | -2.69% |
| Master, 28 cells | 45.091% | 45.074% | -0.04% |
| Changing Master chords, up to 56 cells | 48.093% | 47.391% | -1.46% |

Idle and single-note workloads improved in all five pairs; single-note
paired changes range from -8.61% to -11.72%. Triad and dense cases have
overlapping timings and some non-improving pairs, so no universal dense-
playing improvement is established. All 50 timed render hashes match.
Changing-chord thread deadline overruns fall from 126 to 99, but wall-time
overruns remain in every workload. This is not a glitch-free playback claim.

All eight unit/audio suites and the ParameterMatrix rotation pass. The
official native CpuBudget passes on its first integration run, in 68.99 s,
with a maximum median factor of 0.493858462 against the unchanged 0.75
budget. The worst wall block is 6.13371094 callback periods. No failed run
was discarded and no timing threshold changed.

The integration evidence is outside the checkout in
`diagnostics/respira-mic-cpu-check-20261102`: paired raw timings, exact audio
comparisons, source composition, build and test logs, frozen review bundle
and hash manifest. The CPU-only commit has its own
[benchmark and fixture diagnosis](active-cells-cpu-2026-11-02.md).

## Evidence

External review folder: `diagnostics/respira-mic-motion-20261102` in the Rack workspace. `cpu-probe.cpp`, `measure-cpu.py`, `measure-cpu-gap.py`, raw JSON and summaries preserve the trials, including failures and deadline counts. Frozen baseline and pre-optimization sources make the variants reconstructible. These temporary native measurements do not replace SDK or compatible-host validation.
