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
