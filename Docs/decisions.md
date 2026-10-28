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

## 2026-10-28 — the treble pallet hole fitted to Okada's open slope, heard as "ok", not kept (owner; engineering)

Okada's rig (Adachi et al., Forum Acusticum 2026) measured a C4 reed falling
2.5 Hz per kPa with its pallet fully open and 5.1 with it lifted 1 mm; the
model's C4 fell 1.5 and 5.8. Pack `06-pallet` (listening-2026-10-28):
Clarinet Solo on one pull, C4 through a crescendo, A4 through the
decrescendo; A the shipping hole, B the treble pallet holes at 0.56 of their
area, fitted to the open slope, with the half-press travel re-pinned to read
5.1. Verdict, by ear: the candidate is "ok"; the owner also heard the
decrescendo fall too little, which the output stage's compressor causes in
both (about 4.5 dB against 10 without it), not the hole.

Not kept. The tip weights changed the same day (a held key on a hard push)
move the fit: the open slope now needs 0.45 of the area, not 0.56. There the
one harmonic measurement rejects it (Elejalde-García et al. 2021, A#2–A3 at
70 dBA): the 15th–20th harmonics fall 12.8 dB below it, against the shipping
hole's 6.0, and the held A4 on Bassoon sounds 5.8 cents flat against the
5-cent check. A weak preference for one fit does not outweigh that. The
model's C4 now falls 1.3 Hz/kPa open and 4.7 half-pressed, against 2.5 and
5.1 (README).

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

## 2026-10-28 — against a bandoneon: three reed changes not kept (engineering)

The owner supplied a bandoneon sample library (BANDONEON v1, Miguel Lopez,
Pianobook) as a reference. `Tools/measure_bandoneon.py` reads from its held
notes how pitch and the odd harmonics follow loudness, and
`Tools/CompareBandoneon.cpp` sets the 8' cells against that (README, How it
works and Known gaps). Over the loudest 15 dB the cells bend as the
bandoneon does; over 30 dB they bend 0.25 cents per dB against 0.39, and
C2–D3 0.2 against 0.34–0.62. Each change below was scored with both tools,
and against Okada's C4 reed (2.5 Hz/kPa open; the model 1.4).

- **The reed gap's loss coefficient of Adachi et al.'s simulation (8, a
  discharge coefficient of 0.35 in place of 0.65).** The C4 bent 0.7 Hz/kPa,
  half as much as now, and the pitch and harmonic errors grew.
- **The tongue's air drag at 0.5–8× its voicing estimate.** More drag bent
  more, by too little to matter: 1.9 Hz/kPa at 8×, with the harmonic errors
  growing.
- **The clearance round the tongue in its slot at 2.3× and 4.7× (0.07 and
  0.14 mm, the gap of Adachi et al.'s reed).** At 0.14 mm the A5's harmonics
  grew with loudness as the bandoneon's do, but every other note's 5th and
  7th grew 0.3–0.7 dB per dB too fast, loudness rose 8.4 dB per doubling of
  pressure in place of 5.8, and the C4 bent 0.8 Hz/kPa. At 0.07 mm (checked
  again the same day, after the bandoneon benchmark reached the 15th) the
  slopes of the 9th–15th fell to 0.40–0.65 dB per dB of mean error from
  0.55–0.88, but the A#2–A3 15th–20th harmonics at 70 dBA fell 11 dB below
  the one accordion measurement (6 dB now) and a half-pressed key lost 26 dB
  (20 now, 11 measured): the two measurements pull the clearance opposite
  ways, so it stays.

The brass tip weights are what keep C2–B2 stiff, and they are plausible: a
weighted C2 tongue's stiffness is close to a 50 mm steel strip's with a gram
at its tip. Without them a C2 tongue deflects 6 mm under 1 kPa, which no
real reed does, and its pitch rises with pressure. No measurement found gives how loudness grows with pressure, which
decides where the bandoneon's softest notes sit, so the soft-bass gap is
recorded, not tuned.

## 2026-10-28 — the mechanism sounds, with a Mechanics level (owner)

Respira's rule was that every sound comes from air the hand moves, so a key
pressed with a still hand was silent. Real accordions knock: a key on its felt
as it goes down, a pallet on its seat as the spring closes it, the bass
buttons and register switches (Niewczas 2019: the mechanism's sound spans
about 210 Hz–4 kHz, louder on older instruments and troublesome in pp
recordings). No published measurement of its level against the reeds was
found. Offered three options (add with a level control, add calibrated from a
phone recording of a real accordion, leave out), the owner chose to add it
with a Mechanics control whose default is the physics estimate, the default
to be picked by ear. `CLAUDE.md` now names the mechanism as the one exception
to the air-only rule.

Each knock is a collision on a felt law measured on a piano action
(Hirschkorn 2004), at the speed the engine moves the part; masses and travels
are estimates (README, The mechanism). Mechanics is in dB against the key's
own dipole radiation, which leaves out what the keyboard and body radiate;
the listening pack `08-mechanics` offers the estimate and 10 dB either side
of it, the span the estimate's uncertainty allows. Verdict pending.

## 2026-10-28 — the reed plates half as thick again, B chosen by ear (owner; engineering)

`Tools/CompareHarmonics.cpp` found the 4th harmonic of A#2–A3 12 dB under the
one published measurement (Elejalde-García et al. 2021, a concert accordion's
8' reeds at 70 dBA and 50 cm): at that loudness each tongue swung 0.7–2.1 mm
out past the far face of its 1.3–2.3 mm plate, where the gap opens again, and
the second pulse of air each period nearly cancelled the 4th. No measured
plate thickness was found, and it varies between instruments (Cottingham,
Physics Today 2011). Pack `09-plate` (listening-2026-10-28): Clarinet Solo,
A#2–A3 one after another at about 210 Pa, then the A3 through a crescendo; A
the plates as they were, B half as thick again (2.0–3.5 mm), the tuning table
regenerated. Verdict, by ear: B.

B ships. The 4th now sits on the measurement and the 2nd–16th agree within
2.2 dB RMS (4.2 before); the 15th–20th stay 7 dB low. The bandoneon slopes
(`CompareBandoneon`) and Okada's open slope (1.3 Hz/kPa) barely move, the
half-pressed C4 bends 4.2 Hz/kPa (4.7 before, 5.1 measured), the unit, audio
and perf tiers pass, and the demos are 0.5–2.5 dB brighter above 1 kHz. The
choice licenses this estimate of the thickness, not fitting it to the 4th
harmonic.

## 2026-10-28 — the output compressor from −5 VU at 20:1, B chosen by ear (owner; engineering)

The owner heard the decrescendo of pack `06-pallet` fall too little, which the
output compressor caused (about 4.5 dB against 10 without it): its knee ran
from −9 to −1 VU at 10:1, so a swell's top was flattened. Pack `07-swell`
(listening-2026-10-28): Clarinet Solo, a held A4 swelling from nothing to
forte and back on one pull, then a C4–E4–G4 triad doing the same on the push;
A the knee from −9 to −1 VU at 10:1, B from −5 to −1 VU at 20:1, the limiter
the same, B trimmed −1.47 dB to A's RMS. Untrimmed the swells rose 9.6 dB in
B against 7.9 in A. Verdict, by ear: B.

B ships. The hardest strokes read −1.7 VU at most (−2.9 before), still out of
the red, peaking at the limiter's −1.5 dBFS. The demo phrases read −9.6 VU at
their median while a reed sounds, 0.4 dB above A on the same measure (p90
−4.4 against −5.9), so the output reference stays. Two engine checks assumed
the old knee and were re-derived, each with its reason beside it: the
reference gestures' median now reads −4.6 VU (−5.8), and on the presets'
reference stroke the limiter now trims the loudest presets' peaks by up to
0.3 dB (measured with its ceiling lifted), so that check holds the peaks at
the ceiling rather than below it.

## 2026-10-28 — Mechanics at +10 dB by default, B chosen by ear (owner)

Pack `08-mechanics` (listening-2026-10-28): Clarinet Solo, keys fingered with
a still hand, then a melody over bass and chord buttons, a register switch
and a held chord; A the engine before the mechanism (no knocks), B, C and D
Mechanics at +10, −10 and 0 dB (the estimate), not in that order, untrimmed
because level was what was judged. Verdict, by ear: B. Mechanics defaults to
+10 dB in Init and every factory preset: a soft key peaks near 52 dB SPL at
1 m and a hard one near 74, where ordinary playing reads 73 dB RMS. The
choice sets the default within the estimate's stated ±10 dB uncertainty; it
does not measure the mechanism's level, which stays a known gap.

## 2026-10-28 — the breath at half the estimate, C chosen by ear (owner)

Pack `03-clarinet` (listening-2026-10-28), whose first verdict was "B or C,
both good" (above), came back with a firm one: C, the shaped breath at half
the estimated fluctuation, the eddies' turbulence intensity 5 % in place of
10 %. C ships (ReedModel.h, `breath`). Half the estimate lies inside the
range the breath was offered over (0.3× to 2×, pack `breath`, 2026-10-27),
which this verdict supersedes. It licenses the level only: the eddy size,
their count along the gap's edge and the spectrum stay as derived.

## 2026-10-28 — 02-bass C, 04-air B and 05-bend B confirmed (owner)

The owner returned every pack in listening-2026-10-28 on one sheet. Three
confirm what ships: the bass box at its typical geometry (02, C, first heard
as "probably" the best), the air valve's hiss through the bass box (04, B,
first "probably" better) and Assist counting a half-pressed key in full (05,
B). Nothing changes; the earlier entries' hedges no longer apply. Pack
`06-pallet` came back B as well; that candidate is not kept for the reasons
in its entry, and the owner was asked whether to apply it anyway.
