# Microphone colour during Life movement

This local iteration starts from 0.1.6, commit
`f9057b33cbfa0c5230131e44862f4cced159bd2d`. On 2026-11-02 the owner chose
**“B but tiny”** in the frozen A/B. That licenses the reviewed microphone
response as a small preference by ear, not an acoustic calibration or a
larger movement amount. It is finalized as 0.1.8 on top of the separate
0.1.7 CPU improvement; the combined engine reproduces the reviewed B WAV
byte for byte. See the [decision](../decisions.md#2026-11-02--microphone-movement-b-selected-with-a-tiny-preference).

## Physical scope

Life already moves two equivalent point sources relative to a coincident
cardioid pair. It changes broadband level and stereo balance, but its
microphones have no frequency-dependent change with movement.

[DPA's account of proximity effect](https://www.dpamicrophones.com/mic-university/background-knowledge/proximity-effect-in-microphones-explained/)
explains its dependence on distance, direction and source geometry, and why
a microphone's neutral response needs a stated reference distance.
[Zadvitskiy's DAGA 2024 paper](https://pub.dega-akustik.de/DAGA_2024/files/upload/paper/351.pdf),
equations 2–8, describes two spherical-wave pressure samples at a directional
capsule's ports and the internal acoustic delay. Real capsule construction
also matters. These sources support the mechanism, not identification of
the microphones or positions in the owner's recordings.

For a small ideal cardioid in a spherical field, the linear momentum
equation gives the pressure/particle-velocity relation. Combining pressure
and axial gradient gives, apart from common gain,

`H(s) = 1 + cos(theta) + c*cos(theta)/(s*r)`.

The existing panning already supplies the broadband `1 + cos(theta)` factor
and relative distance spreading. Dividing the remaining response by its
reference-position response leaves a first-order shelf:

`Hrelative(s) = (s + a)/(s + a0)`, where
`a = c*cos(theta)/(r*(1 + cos(theta)))`.

The prototype implements this relative response with a bilinear one-pole
state per source and channel. Its coefficients come from the existing
geometry and 343 m/s sound speed. There is no added bass-boost setting,
chosen cutoff or enlarged movement. The reference sound remains the
existing voicing; this is not a calibrated model of a particular capsule.

At the present one-metre distance, the effect is small: extrema of the
allowed motion predict about ±0.3 dB at 33 Hz in a default-width channel,
about ±0.1 dB at 65 Hz and hundredths of a dB at 131 Hz. Typical motion
uses less than the bounds. These are geometry predictions, not measured
accordion response or a claim of audibility.

Only direct sound receives the correction. The surrounding Room field does
not receive it. Life zero adds exactly zero; the reference pose is neutral.
There is no new reed excitation, pressure gesture, pitch modulation,
propagation delay, source directivity model or user parameter. The very-low-
pressure reed-start limitation remains a separate, unresolved problem.

## Checks and CPU work

The filter's rendered complex response is checked against an independent
finite-port capsule calculation at 32–512 Hz, across three rates, three
Width settings and both channels. This includes the bass source crossing
a microphone's 90-degree direction. The oracle uses a shrinking-port approximation rather than
repeating the implementation's pole formula. Existing Life checks cover
reset, host-block independence, held pressure, manual air control, mono and
silence.

An idle-filter shortcut was tried and removed because the paired timing run
showed no clear benefit. A second CPU candidate reused the exact gap area
already calculated for breath, only for the first internal step before the
tongue moves. Twenty-four dynamic render comparisons proved bit identity
for that cache and identity with 0.1.6 when Life is zero, across four rates
and all three quality modes. Its median CPU was unchanged or worse, so it
was removed too. [The native CPU record](../benchmarks/microphone-motion-cpu-2026-11-02.md)
preserves both attempts and their deadline misses. Neither removed attempt
earns a CPU reduction claim. A later, separate active-list optimization does
reduce sparse-playing cost and is included without changing the B audio;
its combined measurements are recorded in that same CPU report.

Turning Life off during a held note snaps the new geometry coefficient to
exactly zero below 1e-20; a test covers this transition at all three filter
test rates. This prevents a persistent subnormal coefficient without relying
on an idle-voice flush. It does not alter the chosen motion or audible depth.

The reviewed A/B uses unchanged MIDI, transport, settings, seed and renderer,
with A frozen at 0.1.6. Whole-file stereo RMS matching trims A by
-0.00162034 dB and B by 0 dB. The original files and separate key remain
frozen outside the repository. The owner's small preference for B supports
this exact sound direction; it does not establish the microphones or
movement in any reference recording, or resolve the low-pressure reed-start
problem. [Validation and evidence](../validation/microphone-motion-2026-11-02.md).
