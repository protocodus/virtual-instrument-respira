# Engine test fixtures and the Windows stack

The Windows engine suite crashes in both the unchanged 0.1.6 baseline and
the approved 0.1.8 microphone candidate:

- [Baseline f9057b3, run 41353439333](https://github.com/protocodus/virtual-instrument-respira/actions/runs/41353439333/job/124988350810).
- [Approved b3b2533, run 41397111549](https://github.com/protocodus/virtual-instrument-respira/actions/runs/41397111549/job/125120710551).

Both compile successfully, then CTest reports `Respira.Engine` as a
segmentation fault. The new microphone tests pass on Windows. The other
three jobs for b3b2533 pass: macOS unit, Linux unit/audio/perf, and the
sanitized unit/fuzz suites. The crash therefore predates both improvements.

Several older fixtures instantiate Engine or Room directly on the stack.
The current Engine occupies 1,263,032 bytes on arm64; the room delay storage
is its largest part. Microsoft's [linker documentation](https://learn.microsoft.com/en-us/cpp/build/reference/stack-stack-allocations)
specifies a default executable stack reserve of 1 MB. Native Release Clang
`-fstack-usage` reports a 2,527,536-byte inlined main frame before the fix.
After moving only the Engine objects, a remaining Room fixture still
requires a 1,117,040-byte frame. These Clang sizes diagnose oversized test
storage; they are not a captured MSVC stack trace.

The fix allocates the four Engine instances in ResetPlaysAsNew,
PresetsSitTogether and HardStrokesFit, and the Room in
RoomDecorrelatesAndRings, on the heap. This follows the existing Make()
fixture's ownership pattern. Fresh construction, Prepare, SetControls,
Reset and render ordering are retained. No DSP, controls, MIDI, assertion,
tolerance, timeout, stack-limit or build-flag change is made.

After the change the largest native test-function frame is 1,408 bytes
(main is 1,344). An external Release harness calls the entire unchanged
EngineTests main on a pthread explicitly limited to 1,048,576 stack bytes;
all contracts complete with zero failures. It links the previously
validated DSP library, and source hashes confirm the approved sound is
unchanged. This constrained native check is separate from the subsequent
Windows CI result.

Raw failed job logs, size probe, before/intermediate/after stack reports,
the constrained-thread harness and its complete passing output remain
outside the checkout in
`diagnostics/respira-mic-cpu-check-20261102/windows-stack/` (job logs in its
parent directory). No failed result was discarded or test weakened.
