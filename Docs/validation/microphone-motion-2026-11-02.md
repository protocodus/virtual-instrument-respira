# Microphone motion — native validation and listening verdict

Baseline: 0.1.6, `f9057b33cbfa0c5230131e44862f4cced159bd2d`.
Candidate DSP source/CMake manifest SHA-256:
`88e450aa83dc89dae3fea2776c098a285ffa3b9fa15d6952e089ccf380a58ca4`.
This identifies the original microphone-only review candidate. The owner
chose **“B but tiny”** on 2026-11-02. Final 0.1.8 combines the identical
sound change with the separately committed 0.1.7 CPU improvement. Native
validation and sound approval do not establish a delivered Rack build.

## Passed

- Release build, including `RespiraDSPCxx17` with `-fno-builtin` and
  `-fno-exceptions`.
- All eight tests in the `unit` and `audio` tiers: MicrophoneMotion, Touch,
  Life, Engine, ReedTuning, DspFuzzReplay, DemoMidiSources and AudioDemos.
- ParameterMatrix rotation: 17 controls, 361 cases, 393 runs, zero failures.
- The microphone oracle checks complex response against independent
  finite-port geometry, both stereo channels, three Width positions and
  three sample rates. The reference pose and reset are exactly neutral.
  Turning Life off settles exactly during held input.
- The shipping baseline's 33 source/tool/CMake files were checked against
  its git archive. Both native renderers are Release builds from their
  declared source directories.

The eight preset audio checks cover finite, audible, peak-safe output and
deterministic repeat rendering. They do not establish perceptual realism.
No reed geometry, tuning, breath excitation, bellows law, preset or MIDI
phrase is changed by this candidate.

## Native performance gate: passed on a recorded retry

The candidate's native `Respira.CpuBudget` run failed on Organ Bellows:
0.8359 times real time against the existing 0.75 limit. The unchanged
baseline subsequently failed on five of the eight phrases, including
Organ Bellows at 1.2182 times real time. Large wall-time block overruns are
retained in both records. The runs were under changing system contention;
their ordering does not demonstrate a candidate speedup or regression.

The two efficiency experiments were removed because neither demonstrated
a gain. See [the CPU record](../benchmarks/microphone-motion-cpu-2026-11-02.md).
Thread-CPU diagnostics help distinguish processor work from elapsed time;
they do not replace a native gate or prove Reason performance.
The paired full-phrase diagnostic measured median rendering-thread CPU of
25.2301% for baseline and 25.2515% for candidate, with overlapping trial
ranges. No meaningful change in CPU cost is established by those samples.

After system load fell substantially, one further baseline/candidate pair
ran with source and MIDI identity checked, unchanged binaries and limits.
The candidate passed `Respira.CpuBudget` in 85.90 seconds, all eight phrases
below the 0.75 median budget (maximum 0.6130, Master Tutti). Baseline still
failed Organ Bellows at 0.9356. Both results and load samples are retained
in `cpu-retry-20261102-0229/`; they do not demonstrate a speedup. The
candidate's worst wall block still exceeded its callback period by a factor
of 6.47. The gate measures median cost, not absence of deadline misses.
Native performance-tier passage is now recorded; host performance is not.

## Review and remaining gates

Listening files and their key live outside the checkout in
`diagnostics/respira-mic-motion-20261102`. A uses the unchanged baseline,
B the local candidate, with the same frozen Bassoon Low MIDI and preset.
The preparation script matches whole-file stereo RMS and records its trims.
Both clips are 9.6041458 seconds (460,999 stereo frames), 48 kHz/24-bit,
with the complete release tail. A is trimmed by -0.00162034 dB and B by
0 dB; the measured RMS mismatch after quantization is under 1e-8 dB.
All 1,920 printed 5 ms diagnostic rows agree for hand position, hand speed,
pressure, reed airflow and sounding-cell count, at the trace's printed
precision. The matched waveform difference is -46.11 dB relative to A's
RMS; this is a numerical difference, not an audibility or preference claim.
The owner's verdict is **“B but tiny”**: B is preferred by a small amount.
This is recorded as a by-ear direction, not a measurement of realism and
not permission to increase the effect beyond the reviewed implementation.

The original A/B files, key and review archive remain frozen. Their
historical pre-approval status has not been rewritten. The decision is in
[`Docs/decisions.md`](../decisions.md#2026-11-02--microphone-movement-b-selected-with-a-tiny-preference).

## Final 0.1.8 integration

The final DSP source is exactly the original microphone candidate plus the
active-list optimization in `ae221afa18bfe5963d96619457ae68d6746a98fa`.
CMake version 0.1.8 follows that CPU commit's 0.1.7. Source hashes and an
independent patch composition check establish this relationship.

- The combined renderer reproduces the complete original B WAV and CSV
  trace byte for byte, with no new trim or replacement listening material.
- Twelve dynamic all-rate/all-quality renders match the microphone-only
  candidate exactly. Another twelve with Life zero match the CPU-only
  engine exactly.
- Release and C++17 `-fno-builtin -fno-exceptions` builds pass; all eight
  unit/audio suites pass in 187.01 s; ParameterMatrix rotation passes in
  89.92 s.
- Native CpuBudget passes on its first integration attempt in 68.99 s:
  all eight medians below 0.75, maximum 0.493858462. The worst wall-block
  share is 6.13371094, so no zero-deadline-miss or Reason CPU claim follows.
- Five paired native comparisons give about 11.5% lower median single-note
  thread CPU. Dense-playing results are small or variable. The
  [CPU report](../benchmarks/microphone-motion-cpu-2026-11-02.md#combined-with-the-separate-active-list-optimization)
  retains the workload, timing spread and overruns.

Before finalization, all 92 integration evidence files, five built
executables, sixteen combined source/CMake files, both changed test files,
the original microphone source and both original listening WAVs were
verified against their manifests. All 52 archived source/input files match
the combined checkout. No further source or test change was needed after
the owner's approval; only the verdict and documentation were finalized.

Integration evidence is outside the checkout in
`diagnostics/respira-mic-cpu-check-20261102`, including `gates.json`,
`source-composition.json`, `listening-check/result.json`,
`active-mic-identity.json`, `life-off-identity.json`, `cpu-raw.json`,
`cpu-summary.json`, `official-cpu-gate.json`, `manifest.json` and
`handover-verification.json`. These native checks do not include SDK
translation, Rack installation or compatible-host performance for 0.1.8.

External evidence: `final-build.log`, `final-tests.log`, `final-matrix.log`,
`matrix-detail.log`, `final-perf.log`, `candidate-cpu-gate-failed.json`,
`baseline-perf.log`, `baseline-cpu-gate.json`, `phrase-cpu-raw.json`,
`phrase-cpu-summary.json`, `comparison/measurements.json` and
`source-manifest.json`. `cpu-retry-20261102-0229/` contains the subsequent
native gate pair. `candidate-review.tar.gz` remains the original frozen
candidate build sources, tests, MIDI inputs and pre-retry iteration notes;
only the working documentation has been updated with the new evidence.
