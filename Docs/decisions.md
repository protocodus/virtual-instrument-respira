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
in its entry. Asked whether to apply it anyway, the owner chose to revisit it
once the reed chamber's top end is fixed: the fit to Okada's open slope is
then redone and kept if the harmonic measurement allows it.

## 2026-10-28 — the reed chamber as a duct, B chosen by ear (owner; engineering)

Pack `10-chamber` (listening-2026-10-28): Clarinet Solo, A#2, D3 and G3 on a
steady pull at about 210 Pa (the loudness of the one published harmonic
measurement), then C4, G4 and D5 on a push and an A major chord held to the
end; A the lumped chamber, B the chamber as a duct along its reed in sections
of at most 5 mm, the gap's air entering along the slot with the tongue's tip
at the closed end; matched on whole-file RMS (B −1.0 dB). Verdict, by ear: B.
It ships (ReedModel.h, `MakeChamber` and `StepCell`), with the tuning table
regenerated (it moved by at most 1.1 cents).

It adds no constant: the duct has the lumped chamber's geometry and losses.
What was measured before the choice:

- Against Elejalde-García et al. 2021 (`CompareHarmonics`, A#2–A3 at
  70 dBA): the 15th–20th harmonics average −13.7 dB against the measured
  −13.4, where the lumped chamber read −20.2; the RMS error over the 2nd–20th
  fell from 5.1 to 2.7 dB, over the 2nd–16th it rose from 2.2 to 2.5.
- It converges: halving the sections moves the harmonics by under 1 dB below
  8 kHz and 2.5 dB above. The sectioned chamber not kept earlier today (two to
  four sections) moved 15–24 dB between grids because it was far too coarse to
  converge; this is the model it converges to.
- Against the bandoneon samples (harmonics per octave against 1–2 kHz, mean
  distance over seven held notes): 2–4 kHz 3.5 → 5.7 dB, 4–8 kHz 8.5 → 6.1,
  8–16 kHz 5.1 → 13.0, B too bright there. A bandoneon is not an accordion
  and the recording chain is unknown, so this is weak evidence either way.
- A half-pressed A#2–A3 falls 11.1 cents and 18.6 dB (11.2 and 20.5 before;
  measured 16.1 and 11.1). The reeds' own gap flow is unchanged and still
  agrees with Pomberger et al. (DAGA 2021) within 1–6 dB.
- Master Tutti took about 0.32× realtime against 0.25× before.

The choice licenses the duct's form up to the measurement's 20th harmonic
(4.4 kHz), and its top octaves as what that form gives; it does not measure
them (README, Known gaps, "The top octaves"). Both tongues' tips sit at the
closed end because the measurement favours it: at the pallet hole the
15th–20th fell 16 dB below it. A variant with the push and pull tips at
opposite ends was dropped before listening, since a held key fell silent on
a hard push (`HardPushSpeaks`).

## 2026-10-28 — the treble pallet hole refitted on the duct, not kept (engineering)

The owner asked (06-pallet, above) for the fit to Okada's open slope to be
redone once the chamber's top end was fixed. On the duct the treble pallet
holes need 0.40 of their area for C4 to fall 2.5 Hz/kPa open (the full hole
gives 1.3). The harmonic measurement rejects that: at the pressure that reads
its 70 dBA (428 Pa with the smaller holes) the 15th–20th fall to −19.9 dB
against the measured −13.4, and a half-pressed key, the pallet opened 0.2,
falls silent. The shipping hole stays, and the open slope stays a known gap.

## 2026-10-28 — the bass side's chambers in 10 mm sections (engineering)

Not a listening decision; recorded so it is not taken for physics. Asked to
bring the CPU cost down, the bass and chord reeds' ducts take sections of up
to 10 mm instead of 5. Their sound leaves through the bass box, whose
resonance near 960 Hz takes off what the longer sections change: with the
tuning table regenerated (bass entries move by at most 0.19 cents) every demo
stays within 0.1 dB of the 5 mm duct in every octave band from 62 Hz to
24 kHz. Bassoon Low renders 8–9 % faster and Master Tutti 3–5 %. At 15 mm
Master Tutti's lowest octave moved 0.6 dB, at 20 mm 1.4 dB, so 10 mm is the
longest kept. The treble side stays at 5 mm: 7.5 mm there took 3–6 dB off
8–16 kHz and 10 mm up to 21 dB, a cut the grid makes, not the chamber.

## 2026-10-28 — the low reeds' tip weights: B chosen by ear, not kept as rendered (owner; engineering)

Pack `11-attack` (listening-2026-10-28): Bassoon Low then Clarinet Solo,
eight staccato notes each from C3 to A4, struck with the bellows already at a
steady mezzo forte pull. A the shipping tip weights, sized by two rules (no
more than an eighth of the slot depth of static bend at 1 kPa, and short of
the slot at the arm's full 2.5 kPa); B weighted by the first rule only, with
the set raised so that 2.5 kPa still leaves each tongue short of its slot; C
weighted by the first rule only. Matched on whole-file RMS (B and C −2.4 dB
against A). Verdict, by ear: B.

The direction it licenses is lighter low reeds, which speak sooner: from −50
to −5 dB of the fundamental at 70 dBA, A2–B2 take 239–280 ms in B against
285–321 in A (C 159–193; Llanos-Vázquez et al. 2014 measured 70–100), and
A3–B3 112–126 against 131–179 (C 60–70; measured 60–110). Okada's measured C4
reed (Adachi et al., Forum Acusticum 2026) points the same way: it falls
2.5 Hz per kPa with its pallet open, B's C4 2.3, C's 1.9 and A's 1.0.

B does not ship as rendered, because a measurement rejects it. Its higher set
leaks more air at rest, and through a half-open pallet that leak holds the
chamber below the reed's starting pressure: at the 70 dBA of Elejalde-García
et al. (2021), 165 Pa, the 8' cells from C2 to C#4 with the pallet opened 0.2
sound 55–74 dB under their open level (A from C2 to A2 only, 31–63 dB), where
the player measured A#2–A3 bent that way at 11 dB under open. Its set at C4,
0.83 mm, is also well above Okada's measured 0.3 mm. `HalfPressedKeys` misses
by 0.06 cents (the pitch returns 3.06 cents flat of open, limit 3). C was not
chosen and would not ship either: on `HardPushSpeaks`' stroke every factory
preset loses its low keys, from C2 up to E3 or as far as C5, and every bass
button, which is the 0.1.0d6 fix undone. Nothing changes until a candidate in
B's direction keeps hard pushes and half-pressed keys speaking.

## 2026-10-28 — the reed gap's own air mass, not added (engineering)

Not a listening decision; recorded so it is not tried again unexamined. On
its own, with no chamber and a steady pressure drop, a Respira tongue starts
only from 250–400 Pa at A3–A4 and about 1 kPa at A6: the slot's air is all
that delays its flow. Ricot, Caussé and Misdariis (JASA 2005) studied an
accordion reed sounding without acoustic coupling, and a reed organ's 268 Hz
reed on a wind chamber tuned away from it started at 50–130 Pa (Cottingham,
ICA 2019, Fig. 8). Adachi et al. (Forum Acusticum 2026) give the air at the
gap itself a length of 0.5 √(S/π) over the gap's area S, a coefficient they
fitted to Ricot's experiment; added to the slot's, it starts the lone tongue
from 30–50 Pa from A3 to A5. In the cell, where the chamber and pallet hole
already load the reed, it changes how fast a swing grows by under 10 %, the
loudness slopes of `CompareBandoneon` by under 0.05 dB per dB and the
half-pressed level loss not at all, and `CompareHarmonics` rejects it: the
2nd–4th harmonics of A#2–A3 at 70 dBA sit 4.4 dB under the measurement
instead of 2.1 (−10.1 dB on the fundamental against −5.7 measured and the
shipping −7.8). Adachi's model
has no slot, so the two air masses may overlap; the slot's stays alone.

## 2026-10-28 — the output compressor from −9 VU at 5:1, C chosen by ear (owner; engineering)

Pack `12-dynamics` (listening-2026-10-28): Paris Musette, a bass, a chord
button and a treble triad on four strokes, each with the hand twice as fast
as the last. A the knee from −5 to −1 VU at 20:1 (main), B from −12 to −6 VU
at 3.5:1, C from −9 to −3 VU at 5:1; the limiter the same. Not level-matched,
because the level of the harder strokes was what was being judged: their
loudest 300 ms read −13.0, −5.0, −2.6 and −2.6 VU in A, −13.0, −6.5, −4.0 and
−3.3 in C. Verdict, by ear: C.

C ships: a harder stroke still gets louder at the top, at the cost of firm
ordinary playing coming down a little. The demo phrases read −8.9 VU at their
median (with the low reeds of 13-bend and the bellows refitted to them), the
hardest strokes −2.1 VU at most and peak at the limiter's −1.5 dBFS. The
reference gestures' median in `TypicalLevel` moves from −4.6 to −5.7 VU and
is re-pinned with that reason beside it; `HalfPressedKeys` now plays 24 dB
down, below the stage, because its 5:1 squeezed the 7 dB a half-pressed reed
quietens by to 3.

## 2026-10-28 — lighter low reeds set higher, with a wider half-pressed pallet, B chosen by ear (owner; engineering)

Pack `13-bend` (listening-2026-10-28): the 05-bend passage on Clarinet Solo.
A main; B the low reeds weighted only so that 1 kPa bends a tongue at most an
eighth of its slot depth, set higher so that the arm's full pressure still
leaves each short of its slot (11-attack's B), and full channel pressure
holding the pallet at 0.32 of its open area instead of 0.2. Matched on
whole-file RMS (A −0.33 dB). Verdict, by ear: B.

0.32 is a measurement's, not a fit to the ear: it is the opening at which the
model's C4 falls the 5.1 Hz per kPa Okada measured with the pallet lifted
1 mm (Adachi et al., Forum Acusticum 2026); Okada's own model gives that
pallet 0.48 of the open area. 0.2 had assumed 1 mm to be a fifth of a 5 mm
lift, and with B's reeds silenced half-pressed low keys at mezzo forte.

What it licensed, measured on the shipping engine (with 12-dynamics C and
14-treble B):

- At 70 dBA, A2–B2 take 271–314 ms from −50 to −5 dB of the fundamental
  (294–328 before) and A3–B3 119–136 (124–170); Llanos-Vázquez et al. (2014)
  measured 70–100 and 60–110. On Bassoon with the wheel swept in 1.5 s, C3–F#3
  reach their level 0.45–0.65 s after the key (0.65–0.95).
- Okada's C4 falls 2.5 Hz per kPa with its pallet open and 5.1 half-pressed;
  the model's 2.3 and 5.1 (1.3 and 4.3 before).
- At the 70 dBA of Elejalde-García et al. (2021), now 165 Pa, a half-pressed
  A#2–A3 at a steady pressure falls 8.5 cents and 12 dB (`CompareHarmonics`),
  where the player's fell 16 and 11; before, 11 cents and 19 dB.

What it costs:

- `CompareHarmonics`: the 2nd harmonic of A#2–A3 at 70 dBA sits 9.0 dB under
  the measurement (5.5 before), the 2nd–14th 2.3 dB from it on average (1.7).
- `CompareBandoneon`, with the loudest at 1.5 kPa: over the loudest 15 dB the
  cells now fall 0.88 cents per dB louder against the bandoneon's 0.47 (0.46
  before); over the loudest 30 dB, 0.51 against 0.39 (0.25), and C2–D3 now
  rise as the bandoneon's do as they soften (0.33–0.52 against 0.34–0.62).
  With the loudest at 0.7 kPa both slopes agree within 0.04.
- The higher set leaks more air, so the same hand makes less pressure: the
  demo bellows were refitted (`Tools/fit_bellows.py`) after strokes fell up to
  57 % short of their targets. A low key struck with channel pressure already
  full at about 200 Pa does not start from A#2 to D3; bent from sounding it
  does.
- Three engine checks assumed the old opening and were re-derived with their
  reasons: `HalfPressedKeys` reads the returned pitch once the bellows has
  settled (2.1 cents from open; 0.4 s after letting go the bellows was still
  85 Pa above the open take), and `AssistHoldsHalfPressedKeys` asks that the
  pressure rise at all (1.15 times open; counting the opening it fell to
  0.64), not 1.5 times.

## 2026-10-28 — lighter tongue tips from A5 up, B chosen by ear (owner; engineering)

Pack `14-treble` (listening-2026-10-28): a run up to C7 with staccato and
held notes. A main, where a tongue keeps 0.3 of a uniform tongue's tip mass
at 10 mm rising to 0.6 at 30 mm; B 0.18 from A5 (19 mm) up, rising to the
same 0.6 at 30 mm, so nothing below E4 changes. Matched on the held A6's last
half second (B −0.6 dB), since how much of each short note sounds was being
judged. Verdict, by ear: B.

0.18 is a linear taper to the 0.05 mm tip a repairer gives the smallest
piccolo tongues (Accordion Revival, "Accordion Repair 3") and a reed maker
the highest notes. Where the fall ends, A5, was fitted to the one
measurement of attack times (Llanos-Vázquez et al. 2014, Acta Acustica 100,
Table I), so the choice licenses that one fitted hinge; it is recorded as
such beside the code. At 70 dBA A4–B4 now take 52–57 ms (measured 60–70),
A5–B5 36–44 (50) and A6–B6 92–127 (60–100), where they took 65–71, 111–128
and 215–311. The top reeds start from 28–37 % less pressure in their cells
(A5 32 Pa, C6 39, A6 85, C7 130).

What it costs: from A5 to C6 the pitch falls 0.50 cents per dB of loudness
over the loudest 30 dB where the bandoneon's falls 0.36–0.37; a half-pressed
key from E5 to G6 bends 12–28 cents at 200–400 Pa, where the player found no
bend possible from C6 up (on valveless tongues; Respira's all have valves);
and master tune, which rebuilt each reed at the new pitch and so changed its
tip share, put A4 1.1 cents short of +100. Master tune now retunes each tongue
at its root and re-sets it as a reed is made, so a hard push still speaks at
−100 cents; A4 reads −100.3 and +100.4.

## 2026-10-28 — valveless reeds from C6 up, not kept (engineering)

Not a listening decision; recorded so it is not tried again unexamined.
Elejalde-García et al. (2021) found no half-pressed bend possible from C6 up
and put it down to those tongues having no valve strips; practitioners say
valves stop around B5–C6. With the reeds from C6 up valveless (the idle
reed of the pair passing air back, its tongue blown away from its slot), a
half-pressed C6–G6 bends 1–2 cents at 400 Pa, as measured, where the valved
reeds bend 22–28. But the idle reed's set gap, open in parallel with the
sounding one, shunts the chamber: an open C6–C7 cell sounds 15 dB quieter at
200 Pa and starts from 100–300 Pa instead of 40–150, and half-pressed E6–G6
fall silent at 200 Pa. A real valveless reed would leak less only if the top
reeds were set lower than the model's 0.24–0.28 mm, and no set for them was
found; inventing one is outside what a listening test can license, so the
reeds keep their valves.

## 2026-10-28 — the tongue's drag by the oscillating-plate law, not kept (engineering)

Not a listening decision; recorded so it is not tried again unexamined. The
tongue's face-on drag coefficient is a constant 8, the middle of a flat
plate's measured law in oscillating flow, Cd = 10 KC^−1/3 with
KC = 2π (tip swing) / width (Keulegan and Carpenter 1958, as Graham 1980
fitted it). With the law itself, the swing read each step from the tongue's
displacement and velocity, small swings meet more drag and large ones less.
Against the bandoneon (`CompareBandoneon`, loudest at 1.5 kPa) the pitch
slopes moved towards it: over the loudest 15 dB a mean error of −0.22 cents
per dB (−0.41 shipping), over 30 dB 0.01 (−0.12). The odd harmonics'
loudness slopes barely moved (the A5's 9th–15th 0.16–0.87 dB per dB against
0.07–0.76; the bandoneon's 1.42–2.10). Against Elejalde-García et al.
(`CompareHarmonics`) the 2nd–20th fell from 2.9 to 3.2 dB RMS, and a
half-pressed A#2–A3 bent 8.8 cents (8.5). The top octave's slow start at
piano did not move: an A6 at 100 Pa grew by e every 0.13 s (0.12) and sounded
4.6 dB softer. A better pitch slope against a worse fit to the one accordion
measurement, with the bellows pressure behind the bandoneon's slopes
unknown, does not decide it, and the law costs a cube root on every reed
step, so the constant stays.

## 2026-10-28 — a key's ranks through one shared pallet hole, not kept (engineering)

Not a listening decision; recorded so it is not tried again unexamined. On
an accordion every open rank of a key sounds through that key's one pallet
hole, and a bandoneon's two reeds of a note through one pallet, so the
reeds of a note load each other; Respira gives each cell its own hole. A
bandoneon note's odd harmonics could then gain on its fundamental as its
octave reed joins in, which `CompareBandoneon`, reading one 8' cell, cannot
show. Probed offline with Tango Bandoneon's pair (16' and 8' at the
bandoneon's key + 12), each cell's channel meeting at one hole of their
two holes' area: over the loudest 30 dB from G4 to C6, the 9th–15th
harmonics gained 0.49 dB per dB on the fundamental on average, against 0.27
through separate holes and the bandoneon's 1.45. A fifth of the gap, for
an engine that would group cells by key and solve each key's junction every
step, retune every register whose reeds now pull on each other, and put the
musette's beating at risk of locking, is not enough to make the change on
this evidence.
