# Touch, finger speed and the shared bellows

## Evidence and its limits

Llanos-Vázquez, Elejalde-García, Macho-Stadler and Agos-Esparza,
[Physical and Psychoacoustic Characterization of the Different Types of
Attacks on the Accordion](https://doi.org/10.3813/AAA.918716), Acta Acustica
100 (2014), 375–384, distinguish a pre-tensioned bellows followed by a key
press from a key followed by bellows motion. Slower finger attacks change
the growth of individual harmonics as well as duration. Table II reports
A4 fundamental rises of 60/80/390 ms for ordinary finger / slow finger /
bellows at mezzo forte, and 90/220/410 ms at piano. Section 4 also finds a
higher attack centroid for the slow keystroke: a blanket “harder = brighter”
filter is not supported. These are acoustic rises from −50 to −5 dB of
each harmonic's maximum, not valve travel times or MIDI velocities.

The primary text was read in full, including methods, Tables I–II and
section 4. An accessible accompanying paper is
[Acústica de la lengüeta libre del acordeón](https://documentacion.sea-acustica.es/publicaciones/Murcia14/AMS-0%20003%20CI.pdf).
No key displacement/force recording or MIDI calibration is provided.
The recordings supplied by the owner do not provide synchronized valve
position and bellows pressure either. They cannot identify that calibration.

## What changed in 0.1.5

`Touch.h::KeyOpeningSeconds` retains the previous 64–127 velocity mapping
(7.46–3 ms). Below 64, a fixed travel at speed proportional to velocity
takes `T(64)*64/v`: 14.93 ms at 32, 29.86 ms at 16, 59.72 ms at 8 and
477.73 ms at 1. This is an authored controller mapping to deliberate slow
presses, not a newly measured mechanism. Velocity zero still means the
existing 12 ms register slide internally. MIDI note-on zero remains note-off.
The pallet's effective-area law and reed constants are unchanged. Key stop
collisions use the same actual travel speed as their pallets.

The new **Touch** parameter controls a separate virtual-performer gesture
in Auto. Velocity above/below 64 requests stronger/lighter shared effort.
At 100%, the gesture spans ±30% of the ordinary pressure request and
relaxes with a 200 ms time constant. Those values are musical choices.
The player updates at 200 Hz and uses the strongest press within a 10 ms
cluster; it never sums accents by chord size or reed rank. Keys themselves
are not delayed. The existing hand momentum, air consumption and pressure
feedback determine the response. Life continues varying held-note effort
and microphone pickup; Touch relaxes after a press.

The combined effort stays in the existing 0.65–1.45 bounds. Air planning
reserves for both gestures and may turn on a beat. No independent voice
gain, pitch envelope, brightness filter or noise bed was added. Manual
hand pressure and audio are bit-identical between Touch zero and full.
Touch zero bypasses the new Auto gesture, but deliberately slow keys still
use the extended opening range. To reproduce older songs' low-velocity
transients exactly, use the earlier engine; this is a behavior change.

## Measurements

`CompareAttacks` now accepts 3–500 ms travel experiments; its calibrated
source-SPL method and estimator are unchanged. At A4 / inferred 70 dBA,
the four-cycle tracker measures 44.4 ms rise for 3 ms travel, 56.6 ms for
7.46 ms, and 73.9 ms for 29.86 ms. The last also has an 83.2 ms apparent
key-to-−50 dB delay. Its two-cycle tracker detects an earlier isolated
transient and measures a 154.3 ms first-crossing rise. This sensitivity is
why the apparent agreement with the paper's slow-finger rise is **not a
calibration or validation of the complete attack**.

At inferred 55 dBA, 7.46/29.86 ms travel gives 434.3/384.6 ms rises. The
existing soft-pressure onset mismatch remains. Slower openings alone do
not repair reed startup. None of the reed, geometry or tuning constants
was changed to fit this interface experiment.

The public-engine finger fixture preloads the bellows before opening the
key. At velocity 8 versus 127, first-23-ms RMS is 0.0234 times as large;
settled level differs by 0.0018 dB, mean pressure agrees to six decimal
places and the pitch estimate differs by less than its 0.1 Hz resolution.
The original from-rest fixture tested a bellows attack and was unsuitable
for that assertion; its failed result is retained in the external evidence.

A strong first note at Touch 0/50/100% averages 599.3/639.6/679.9 Pa over
80–300 ms, then returns to the wheel's steady request. Tests also cover
manual bypass, reset, event/block partition identity, non-stacking chords,
and 24 ordinary 50 ms staccato notes at full Touch and Life.

Eight frozen musical excerpts were checked at both Touch 50% and 100%:
563 melody notes, including 192 under 180 ms, passed the existing per-note
treble audibility floor. Csárdás contributes 166/154 of those notes.
The probe preserves every reed and air load and changes only the final
output observation. This is an audibility regression, not a substitute for
listening. Full output remains finite, below full scale including a 4×
interpolated peak estimate, and quiet after release.

## CPU and exact calculation reuse

A moving key can open several ranks at the same travel. The renderer now
reuses the raised-cosine opening only when that travel is exactly equal
within the current internal sample. Every chamber still computes its own
vent law and reed/air state; no control rate or physical precision changed.
The selected C render and twelve dynamic rate/quality sequences remain
bit-identical before/after this optimization. Timings and limitations are
in the [CPU report](../benchmarks/touch-cpu-2026-11-01.md).

## Listening choice and remaining gaps

The owner chose **C, Touch 100%**, over unchanged 0.1.4 (A) and Touch 50%
(B), on identical Fascination MIDI / transport with Life 100%. Velocities
are 91–108, so the separate soft-key extension is inactive in this test.
Whole-file stereo RMS trims were A 0, B −0.07746 and C −0.10792 dB.
This choice licenses the virtual bellows emphasis; it is not evidence for
the low-velocity mapping or acoustic constants. Init and all eight presets
now use Touch 100%, Life 100%, Auto, and Room 0.5%.

The slow-key A/B is preserved separately (A unchanged 0.1.4, B extended
key travel; both Life/Touch zero), with RMS trims −0.07378/0 dB.
The velocity map is an interface engineering choice, not an owner listening
verdict. Strap friction and loose hardware remain unmodelled; the existing
mechanical/air noise inventory is in the [Life audit](life-model-2026-11-01.md).

Evidence, raw audio, primary-paper cache, exact source/binary hashes and
the listening key are kept outside the public checkout in
`diagnostics/respira-touch-model-20261101` in the owner's Rack workspace.
The frozen 0.1.4 listening tests and release artifacts were not overwritten.
