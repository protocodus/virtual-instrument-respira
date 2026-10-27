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
