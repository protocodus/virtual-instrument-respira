# Soft-start investigation

The owner reports both slow very-low-velocity MIDI starts and slow starts
under very light bellows pressure. They are separate problems. **0.1.6
corrects the MIDI regression; the low-pressure reed-model problem remains
open.** No reed geometry, tuning, solver rate or noise level changed.

## Corrected playing interface

0.1.5 extrapolated pallet speed proportionally to MIDI velocity below 64.
At velocity 1, a full press took 477.73 ms, and at 8 it took 59.72 ms. A
50 ms note could therefore release before the valve passed enough air.
MIDI velocity has no calibrated conversion to finger speed. The existing
12–3 ms mapping from 0.1.4 is restored across the whole range: 11.93 ms at
1, 11.43 at 8, 10.87 at 16, 7.46 at 64 and 3 at 127.

This is a bounded controller mapping, not a new acoustic constant. The
ordinary/fast range, Touch's shared bellows gesture, Life, Room and preset
levels are unchanged. Manual hand control still receives no Touch gesture.

Llanos-Vázquez et al., [Physical and Psychoacoustic Characterization of the
Different Types of Attacks on the Accordion](https://doi.org/10.3813/AAA.918716),
Acta Acustica 100 (2014), distinguish finger, slow-finger and bellows attacks.
Their reported harmonic rises are **not** MIDI-to-key-travel measurements.
The paper supports different physical articulation, but not the previous
near-half-second interpretation of an ordinary velocity-1 note.

The new public-engine regression plays 24 notes across 18 pitches, 50 ms each, at each of
velocities 1, 8 and 16, under preloaded ordinary bellows pressure with full
Life and Touch. All 72 pass the unchanged 0.002 RMS audibility floor in
their final 30 ms. The weakest levels are 0.030392, 0.034387 and 0.036160.
The same test against frozen 0.1.5 fails at velocity 1: 0.000016 RMS.
This is a playability contract, not a psychoacoustic or piano-pressure
calibration. At matched bellows pressure, the velocity-8/127 early-RMS
ratio is 0.4569, while settled level differs by −0.0001 dB and estimated
pitch by less than the estimator's 0.1 Hz resolution.

## What the recordings establish

81 non-loop takes were measured from the owner's La Melodiosa and Hohner
sample libraries, with no sample audio copied into the repository. La
Melodiosa is by Petri Pohjanmies (2026, CC BY); its `ba`/`va` files are
round robins, not velocity layers. Marek Kintosh's Hohner pack (2024)
explicitly reports normalization and pitch correction. Bass-button takes
need not contain just one reed rank. Neither pack supplies synchronized
key travel, bellows pressure or calibrated acoustic level.

`Tools/measure_accordion_attacks.py` measures relative fundamental rises
with centered Hann windows of 2/4/8 periods and 1 ms hops, using the maximum
within the first two seconds as its reference. It flags an already audible
file start and a −30 dB crossing inside the initial half-window, where a
trimmed boundary can distort the apparent rise. It does not infer velocity,
pressure or trigger latency from these edited files.

At four periods, 60 of 81 takes have censored −50 dB starts. All 48 La
Melodiosa takes have that issue; 44 also cross −30 dB inside the initial
half-window. The 24 Hohner takes whose −30 dB crossing lies outside that
window have −30 to −5 dB rises of 56.7/91.3/253.0 ms at the 10th/50th/90th
percentiles. These describe the available performances, including their
unknown gestures; they cannot pin a reed mass or a piano playing pressure.

The supplied MP3 performances remain useful listening references, but their
polyphony, room and compression do not identify isolated reed growth.

## Low-pressure model investigation

The frozen baseline is engine commit `2e0d45d851a8ecab5563746be4f83ddd29c69545`.
The existing `CompareAttacks` protocol uses an isolated, noiseless pulled
M cell, preloaded chamber, 3 ms pallet and a free-field monopole source
level. Its inferred 55 dBA corresponds to approximately 40–47 Pa. Its
four-period −50 to −5 dB rises are 1365/950/344/444 ms for A2/A3/A4/A5.
This is **not** a calibrated SPL comparison of complete instruments: body,
microphone direction and room are absent. The slow modeled growth is real,
but identifying the paper's piano pressure with these values is unproven.

Sensitivity experiments were kept outside the shipping checkout. Unless
stated otherwise they use the same frozen cell/chamber geometry, fixed
pressure and 7.46 ms pallet. Their 10 ms broadband RMS windows screen
candidates; those timings are not the paper's fundamental-rise metric.

| Experiment | Finding | Decision |
| --- | --- | --- |
| 96/192/384 kHz solver, identical chamber | A2/A3 remain about 1490/1160 ms to the final-RMS −5 dB point; A4/A5 move at most 20 ms | Extra oversampling spends CPU without fixing startup |
| Larger slot inertance | Doubling/quadrupling it does not improve the low-pressure starts | Reject |
| Jet fluctuation coupled to force, three fixed seeds | No consistent improvement across A2–A5 | Reject; extra noise is not a fix |
| Full piston-flow term in place of its current attenuation | Much slower growth or no sustained oscillation on some notes | Reject as a standalone change; coupling cannot be changed piecemeal |
| Full pressure force paired with full piston flow, with and without slot inertance | Shipping geometry slows A2 to about 2.1–2.3 s and A5 does not sustain; nominal geometry starts A2 sooner but loses it at 1.2 kPa and still loses soft A5 | Reject; this is a coupling sensitivity check, not a full reproduction of Millot's escape-area geometry |
| Smaller reed sets / lighter diagnostic masses | Some starts become much faster | These sensitivity factors are not measured parameters |
| Nominal set and bare modeled tongue, removing the artificial stiffness floor and full-pressure set clamp | A2 screening onset falls from about 1490 to 90 ms at the same pressure | Fails hard-hand playability at −81.14 dBFS; not shippable |
| Inclined resting tongue with quadratic dynamic bend | Mixed soft-start results; some slower; bare-mass variant still fails hard attacks | Reject |
| Additional slit-viscosity approximation | Does not remove the slow-start problem across the range | Reject; no calibration claim |

The full-pressure set constraint and added low-reed mass are a substantial
part of the tradeoff. Removing them makes ordinary strong notes disappear:
for example the bare-model C2 becomes silent around 800 Pa in a fixed-pull
probe. Replacing late soft notes with missing hard notes is not a fix.

Millot and Baumann's [A proposal for a minimal model of free reeds](https://arxiv.org/abs/2401.01606)
was published in 2007 (the arXiv posting is later). It derives the same
effective area for pressure force and pumped flow and discusses competing
escape-area geometries and bifurcations. It motivates a coherent coupling
revision, not a standalone gain boost. Puranik and Scavone's [clamped-bar
model](https://caml.music.mcgill.ca/lib/exe/fetch.php?media=publications:puranik_clampedbar_fa_2023.pdf)
adds higher bending modes, but its harmonium measurements do not calibrate
accordion pianissimo onset. Babanis's [NTUA thesis](https://dspace.lib.ntua.gr/xmlui/handle/123456789/38465)
reproduces Ricot's unsteady aerodynamic formulation; its equations were
checked visually. Ricot's original full paper could not be retrieved in
this run. No unsupported aerodynamic coefficient was substituted.

Repair practice agrees that setting matters: [Edward Jay's illustrated
reed calibration](https://accordionknowhow.wordpress.com/recalibrating-reed-response/)
shows why an excessive gap responds poorly at low pressure. [Hohner's
reed-adjustment guide](https://www.hohnershop.com/blog/how-to-adjust-your-harmonica-reeds-to-your-personal-playing-style/)
also explains the soft-response/hard-choking tradeoff, on harmonicas. Neither
is a quantitative calibration of this accordion's reeds.

## Scope of the result

There is a verified low-velocity fix and a reproducible investigation of
the remaining low-pressure mismatch. There is **no validated physical-model
replacement yet**. An adequate successor must improve soft growth while
passing hard strokes, staccato, tuning, spectral and air-consumption checks;
the rejected candidates must not be described as such a successor.

Native unit/audio/performance tiers and C++17 compilation pass. CPU and
comparison scope are recorded in the [benchmark report](../benchmarks/soft-starts-cpu-2026-11-01.md).
Raw source snapshots, scripts, measurements, failed candidates and lettered
audio stay in the owner's external `diagnostics/respira-soft-starts-20261101`
folder. Earlier listening and release artifacts remain frozen.
