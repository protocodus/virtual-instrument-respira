# Decisions

What was decided about Respira's sound and playing model, by whom and on
what grounds. Directions chosen by ear are recorded as chosen by ear.

## 2026-10-27 — the prototype's playing model, kept (owner)

The prototype made the mod wheel the hand on the bellows: opening draws,
closing presses, and only the hand's motion moves air. The owner played its
demos and asked for it to become a product, named Respira. The engine here is
that prototype's reed and bellows model unchanged; with Assist at 0 it follows
the prototype's bellows pressure within 60 Pa on its four demos.

## 2026-10-27 — a dry output (owner)

The owner asked for a dry signal: the prototype's room is removed, and the
output is the radiated sound of the reeds alone, 1 Pa at 1 m to full scale.

## 2026-10-27 — Assist, Bellows scale and the split keyboard (engineering)

Not listening decisions; recorded so they are not taken for physics.

- **Assist** (default 0.5) scales the hand's air by the open reeds' demand.
  The strict instrument needs about forty times the hand travel for a full
  register with bass as for one A4 reed, which a mod wheel cannot give; Assist
  at 0 keeps the strict behaviour.
- **Bellows** is the air one full sweep of the wheel moves, 0.5–8 L, default
  2 L. At the prototype's 10 L a single reed needed under 0.05 of the wheel per
  stroke, a few steps of a 7-bit wheel; at 2 L the demo strokes take 0.1–0.45
  of it.
- **Split keyboard** (default): C1–B1 bass buttons, C2–B2 chord buttons by
  pitch class, C3 up treble, so a single-channel host such as Reason plays the
  bass side from one keyboard. Channels 2 and 3 still reach the bass and chord
  buttons directly.

## 2026-10-27 — Momentum: the arm trails the wheel (owner, voiced default)

The owner asked for the hand to have momentum, since a hand moving a bellows
lags and smooths what it is asked to do. The arm now follows the wheel's
rebuilt path as a critically damped mass with time constant **Momentum**
(0–200 ms): in a steady stroke it trails by twice that time, it never travels
past the wheel, and a reversal takes 3.36 time constants (10–90 % of the
velocity swing). At 0 the engine is the earlier one, sample for sample.

The default, 25 ms, is voiced, not measured or chosen by ear: a reversal then
takes 84 ms, and a bellows shake of four cycles a second keeps 72 % of its
travel. The demo strokes were re-fitted to their target pressures through it
(`Tools/fit_bellows.py`).

## 2026-10-27 — Oversampling 1×, 2×, 4× (owner; mapping by engineering)

The owner asked every new instrument for an oversampling setting of 1×, 2×
and 4×, default 1×, on the back. Respira's reeds already ran at four times the
host rate, which their pitch and stability were voiced at, so the setting
multiplies that rate: 1× is the engine as it was, to the bit; 2× and 4× run
the reeds at eight and sixteen times the host rate through one and two more
half-band stages. The gap turbulence is white per step, so its force scales
with the square root of the step rate and the noise the reeds hear keeps its
spectrum. Measured on the Clarinet, Paris Musette and Master Tutti demos, 2×
and 4× stay within 0.4 dB of 1× and an A4 keeps its pitch to the cent; this
is a quality option, not a voicing choice.

## 2026-10-27 — five realism changes, by-ear verdicts pending (owner asked; engineering)

The owner asked for five audible changes that bring Respira closer to a real
accordion. Each is derived, not fitted; the two turbulence levels are
estimates with a stated range. A–Z listening packs went to the owner on
2026-10-27, each with A as the engine before these changes. Until the verdicts
come back the estimates ship; a verdict is recorded here as chosen by ear.

- **Breath in the reed gap.** The gap flow carries its own turbulence: eddies
  about 0.3 mm across, each fluctuating ~10 %, along the gap's 2L + W edge, so
  the flow fluctuates by 0.1/√N of itself. Pack `breath`: B 0.3×, C the
  estimate (shipping), D 2×. Verdict: pending.
- **The air button as an orifice, with its hiss.** The valve was a linear
  vent (4 L/s at 100 Pa), silent. It is now a 25 mm hole (Cd 0.65, the same
  4.0 L/s at 100 Pa) opening in a pallet's time, whose jet fluctuates 2 % in
  eddies of the pad's 3 mm lift and radiates through the bass side. Pack
  `valve`: B orifice without hiss, C 0.4×, D the estimate (shipping), E 1.9×.
  Verdict: pending.
- **Half-pressed keys.** Channel pressure holds a channel's keys part way
  down; at full pressure the pallet stays lifted 1 mm, where the model's C4
  falls 5.9 Hz per kPa against the 5.1 Hz/kPa Adachi et al. measured (Forum
  Acusticum 2026). Pack `halfpress`: B shipping. Verdict: pending; A takes it
  back out.
- **Registers reach held keys.** A register switch moves slides over the reed
  blocks, so the ranks it opens start under held keys and the ones it shuts
  fall silent. Pack `register`: B shipping. Verdict: pending; A takes it back
  out.
- **One level for every preset.** Not a listening decision. At 1 Pa full
  scale a full register clipped on a demo-like stroke. Full scale is now
  2.5 Pa (102 dB SPL), and each factory preset's Volume puts it within 1.5 dB
  of Init on one reference stroke, peaking below −3 dBFS, which
  `Respira.Engine` checks.

## 2026-10-27 — full scale at 110.5 dB SPL, after Reason's Recon test (engineering)

Not a listening decision. Reason's Recon test of the Rack build (0.1.0d1)
clipped on hard strokes: a bass, a chord and a C major triad with the wheel
swept at 60 % of its range per second, or the whole wheel rocked every half
second, went over full scale on all nine patches. Both strokes reach the arm's
2.5 kPa, where the chord's reeds beat to peaks about 13 dB above its RMS: up to
+8.0 dBFS at 2.5 Pa full scale, the worst of every preset across 44.1, 48 and
96 kHz, blocks of 64 and 512, 7- and 14-bit wheels, Oversampling 1× and 4×,
Momentum 0, 25 and 200 ms and strokes of 0.6, 2 and 4 wheel ranges a second.
Full scale is now 6.7 Pa (110.5 dB SPL), 8.5 dB lower, which puts that worst
case at −0.5 dBFS. `Respira.Engine` (`HardStrokesFit`) checks both Recon
strokes on every preset. Everything is 8.5 dB quieter: the moderate reference
stroke now peaks near −14 dBFS, and the presets keep their levels against
Init. A limiter would have kept the old level, but it is a level that does not
come from the bellows.

## 2026-10-27 — registers at one level (owner)

The owner asked for the mic and register options to sit at one level, so that
switching one does not jump the output. Untrimmed, the registers spread over
5.9 dB on Init's controls (Master 1.9 dB above Musette, Clarinet and Bassoon
3.5–3.9 dB below it), which is what more reeds do on a real accordion. Each
register now trims its treble side (`kRegisterTrimDb`) so that a C3, a C4–E4–G4
triad, a G4, a C5 and a C6 on the reference stroke average Musette's RMS; the
bass side is untouched, and a register change glides over the same 20 ms as
the other level controls. `RegistersSitTogether` checks all eight within 1 dB
on Init's controls, and the factory presets were re-levelled to sit within
1 dB of Init (`PresetsSitTogether`, tightened from 1.5 dB).

## 2026-10-27 — −9 VU typical, with an output compressor and peak limiter (owner)

The owner asked for every new instrument to read "usually" around −7 on
Reason's VU meter with nothing in the red on the hardest chord, then chose
−9 with a clean peak limiter for Aculele and asked for the same limiter in
every new instrument. On that meter (the louder channel's RMS over 300 ms,
+12 dB) Respira's demo phrases read −25.4 VU at their median at 6.7 Pa full
scale, and the reference chord at the arm's full pressure −2.0 VU: a 23 dB
span that no fixed gain fits between −9 and the red.

Full scale is now 1 Pa (94 dB SPL), 16.5 dB louder, and the portfolio's
shared output stage (`OutputStage.h`; the owner asked for the same limiter in
every new instrument) brings the loud end down: its compressor on the louder
channel's 20 ms power, 10:1 above −5 VU over an 8 dB knee, taking hold at
once and letting go over 300 ms, then its 2 ms look-ahead limiter at
−1.5 dBFS. The plucked instruments' guard sits 3 dB higher, at −2 VU over a
4 dB knee: their hardest chords are 7–9 dB above typical, Respira's 23 dB.
The demo phrases read −9.2 VU at their median (p10 −14.7, p90 −5.7), and the
Recon strokes on every preset −2.7 VU at most, peaking at −1.5 dBFS
(`HardStrokesFit`). An engineering setting of the owner's target, not a
listening decision.

The instrument still makes no level that does not come from the bellows: the
output stage is the recording chain after it, there because the owner asked
for it. This supersedes the closing line of the 110.5 dB SPL entry above.

## 2026-10-28 — the treble grille compartment, withdrawn the same night (owner asked; engineering)

The owner asked for audible realism, the body of the sound balanced against
real recordings. Against two accordion recordings (Hussong, and a polka), the
Paris Musette demo had far more energy above 2 kHz: against 500 Hz–1 kHz,
2–4 kHz read −2.3 dB where the recordings read −21.2 and −10.7, and 4–8 kHz
−17.8 dB against their −30.6 and −18.8. The treble reeds sounded straight into the air;
on an accordion their pallet holes open into the shallow compartment under
the grille, which radiates through the grille's slots. That compartment is a
perforated-panel Helmholtz resonator, derived from its depth, the grille's
open area and the slots' effective length (RespiraEngine.cpp,
`kCompartmentDepth`); none of them is measured, so the typical values ship
(2.7 kHz, Q 0.55: 2–4 kHz at −6.5 dB, 4–8 kHz at −29.1 dB on the same demo)
with the plausible extremes as candidates. Pack `01-musette`
(listening-2026-10-28): A the engine before, B 4.0 kHz, C 2.7 kHz (shipping),
D 1.6 kHz.

Withdrawn before any listening, because the lumped model does not describe
this compartment. A Helmholtz resonator needs a cavity shallow against the
wavelength; at its resonance k·depth = √(open·depth/l′), 1.7 for these values
(the depth over a quarter of a wavelength) and at least 1.0 across the
plausible ones. Treated as a plane-wave layer between the pallet board and the
grille, the flow out over the flow in is
1/(cos kd − k(l′/open) sin kd + j(1 + cloth/ρc) sin kd): averaged over 25–45 mm
it stays within 3 dB of flat up to 16 kHz, where the lumped low-pass was
already −10 dB at 4 kHz and −31 dB at 16 kHz. The recordings agree with the
layer above 4 kHz: without the compartment the demos (all but the bass-side
Bassoon Low) read −15 to −22 dB at 4–8 kHz and −24 to −35 dB at 8–16 kHz
against 500 Hz–1 kHz, the polka −18.8 and −29.4; with it they read −27 to −34
and −48 to −58. Their excess at 1–4 kHz is real but has another cause, not
traced (README, Known gaps). The same check passes the bass box: 0.70 at its
resonance, within 2 dB of the layer up to 2 kHz, darker than it above about
2.5 kHz. Pack `01-musette` is withdrawn.

## 2026-10-28 — the bass box, C chosen by ear (owner asked; engineering)

The bass side sounded through a voiced 2.5 kHz one-pole low-pass. The bass
pallets open into the bass box, the space under the bass end cover, which
vents through the cover's holes: a perforated-panel Helmholtz resonator
(RespiraEngine.cpp, `kBassBoxDepth`). The typical values ship, 960 Hz at
Q 1.3. On the Bassoon Low demo, 2–4 kHz falls from −13.9 to −19.5 dB and
4–8 kHz from −27.9 to −33.0 dB against 500 Hz–1 kHz. Pack `02-bass`
(listening-2026-10-28): A the engine before, B 2.0 kHz (Q 0.85), C 960 Hz
(shipping), D 550 Hz (Q 1.9). Verdict, by ear: C, "probably" the best and
not far from B or D. The typical geometry stays; a choice this close licenses
keeping it, not narrowing its depth, porosity or hole size. The packs 02–04 were
re-rendered once the treble compartment was withdrawn; these figures are
from them.

## 2026-10-28 — the breath's spectrum, B or C by ear (engineering)

The gap turbulence's flow fluctuation was white per internal step: at the
four-times rate three quarters of its estimated variance (10 % over the square
root of the eddies along the gap's edge) lay above 24 kHz, so the breath heard
was about 6 dB below its own estimate. A jet's fluctuation has the spectrum of
its eddies, flat up to their rate U / (2π l) and falling above it; the air
valve's hiss was already shaped so. The gap's breath now is too, with the
same 0.3 mm eddies and the jet's speed from the bellows pressure (about
15 kHz at 500 Pa), its variance kept at the estimate; the force that seeds the
tongues stays white. On held Clarinet notes the floor between the harmonics
rises 7–9 dB below 8 kHz. Pack `03-clarinet` (listening-2026-10-28): A the
engine before, B shaped (shipping), C shaped at half the fluctuation. Verdict, by
ear: B or C, both good. The shaped breath stays at its estimate; the verdict
does not choose between the estimate and half of it. The 2026-10-27 breath
verdict, still pending, was asked of the white version.

## 2026-10-28 — the air valve's hiss through the bass box, B by ear (engineering)

The air valve is a hole in the bass end, under the bass end cover with the bass
mechanism, so its jet's hiss leaves through the cover's holes like the bass
reeds' sound; it went straight to the output. It now passes through the bass
box: in the air-button passage its level is unchanged (−55 dBFS), and above
2 kHz it falls 16–44 dB, a darker rush. Pack `04-air` (listening-2026-10-28):
A the engine before, B through the bass box (shipping). Verdict, by ear: B,
"probably" better. It stays.

## 2026-10-28 — what Assist counts while a key is half-pressed, B chosen by ear (owner)

Assist moves extra air for the reeds that are open. It counted each reed by
its pallet's opening, but a half-open pallet's reed still draws 0.3–0.9 of
its open air (single cells at 200–400 Pa), so half-pressing a key took the
hand's extra air away and the pressure fell. Counting a held key in full once
its pallet is down raises the pressure as the pallet throttles, like pushing
the bellows harder into a bend. Both are defensible readings of what a
player's arm does; ordinary playing is bit-identical in both. Pack `05-bend`
(listening-2026-10-28): Clarinet Solo on a steady pull at about 200 Pa, C3
then A3 bent by channel pressure 0 → full → 0; A counting the opening (then
shipping), B counting the held key. Verdict, by ear: B, "good". B ships
(`AssistHoldsHalfPressedKeys`). With the tip weights of the same day, bent
from open a C3 falls 11 cents and 7 dB and an A3 16 cents and 13 dB, against
a concert accordion's 24 and 14, and 13 and 10, with the finger alone, and
29 and 8, and 17 and 5, with bellows and finger (Elejalde-García et al.
2021). The choice licenses the counting rule only: a low key struck under
full channel pressure overbends, A#2–G#3 by 34–64 cents against at most 34
measured, and stays a known gap.

## 2026-10-28 — three experiments not kept (engineering)

Tried during the overnight realism work and left out, each for a reason a
measurement or an estimate gave, so none went to a listening test.

- **A baffle step on the treble side.** The grille's face is a baffle about
  30 cm across, which would lift the treble side by up to 6 dB where it is
  large against the wavelength. The demos were already brighter than both
  recordings from 1 to 4 kHz against 500 Hz–1 kHz; a step would widen that
  gap.
- **Turbulence at the pallet.** The gap's eddy model applied to the jet out
  of the pallet hole put 25–40 dB more noise into a held note than the gap's
  own breath, far beyond any accordion recording.
- **Turbulence only on the reed that flows.** Gating the gap's breath to the
  reed the bellows drives made no measurable difference to the renders, so
  it was not kept.

## 2026-10-28 — the reed chamber in sections, not kept (engineering)

Tried after `Tools/CompareHarmonics.cpp` found the 15th–20th harmonics of
A#2–A3 6 dB below the one published measurement (README, Known gaps): the
reed chamber as two to four compliances along its channel joined by their air
masses, closed at the far end, the reed's air entering where its gap opens
(along the sides as (s/L)², and across the tip), in place of the lumped
resonator.

- With the tongue's tip at the closed end, the first twenty harmonics agreed
  with the measurement within 2.1 dB in every band, at the pressure that
  reads its 70 dBA at 50 cm. With the tip at the pallet hole the 15th–20th
  fell 13 dB below it. Pitch moved by under a cent and air by under 5 %.
- Not kept. Across the demos it lifted 2–4 kHz by 3–12 dB and 4–16 kHz by
  12–15 dB against 500 Hz–1 kHz, leaving the clarinet and tutti demos
  12–14 dB brighter than the polka recording at 4–8 kHz, which the lumped
  model matches within 3 dB. How much depended on the number of sections:
  two and four differed by 15–24 dB at 8–16 kHz, so the top octaves came from
  the discretisation, not from the physics. The channel's wall losses are
  far too small to tame them (about 0.3 dB over a 47 mm channel at 8 kHz), so
  either a real chamber loses more above its resonance in ways not modelled,
  or the gap's flow pulses are sharper than a real reed's and the lumped
  chamber's roll-off had been hiding that; neither is measured. It also cost
  8–22 % more CPU (two to four sections).
