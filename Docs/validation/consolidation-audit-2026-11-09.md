# Consolidation audit, 2026-11-09

Respira 0.1.21 (`b8a4436`) read end to end before a commercial release: the
engine (`RespiraEngine.cpp`), the reed cell and its bank (`ReedModel.h`,
`ReedBank.h`), the mechanism (`Mechanics.h`), the room, the output stage's
use, the README's How it works and Known gaps, and the recent decisions.
Three defects were fixed (0.1.22 to 0.1.24), every key of every preset was
surveyed, and the CPU was measured against the 10 % block ceiling, which
Respira does not meet
([CPU evidence](../benchmarks/cpu-consolidation-2026-11-09.md),
[plan](../plans/cpu-ceiling-2026-11-09.md)).

## Fixed

### 0.1.22: the first stroke no longer thumps (`7329c2e`)

The fold panels started flat and were only placed when the bellows first
turned, so the first stroke of a session flipped all 144 at once. On the
wheel with closed keys the first 0.3 s peaked at −48.6 dBFS; now −83.2 dBFS,
as every later stroke (−82.5). Under Auto the first note's mechanism peaked
at −32.4 dBFS against −41.4 for the second; now −42.1 (−18.0 to −27.6 with
Mechanics at +20 dB). Test: `FirstStrokeFlipsNoFolds` (first against second
stroke: 33.4, 32.7 and 8.9 dB apart before, within 2.1 dB after).

### 0.1.23: no state left in subnormal numbers (`0f2f71f`)

With flush-to-zero off (a host's choice), three states decayed into
subnormals and stayed: the key bed's modes above 2 kHz (from 1.4 s after
every landing until the panel's lowest mode rang out at 2.8 s), the air
meter's smoothing (from 7 s after the last note, for good) and Life's share
of movement, which the microphones' geometry squared 200 times a second
(from 28 s after the bellows stop, for good). Each cost 0.3–0.6 % of a core
(paired block timings, flush-to-zero off against on) and now ends at zero.
Test: `NoSubnormalsWhilePlayingOrAtRest` reads the processor's sticky flags
around every block of a held chord and of the rest 17 s after it: 261 and
3750 flagged blocks before, none after. The demos render to the same files
as 0.1.22.

### 0.1.24: the top octave keeps its pitch at every step rate (`9f1acab`)

The tongue is stepped by updating its speed, then its position with the new
speed. A tongue of stiffness ω² then swings at (2/dt)·asin(ω·dt/2), sharp of
ω by (ω·dt)²/24: 1.35 cents at C8 at the 192 kHz the tuning table is made
at, which the table takes out. At any other step rate the same table was
wrong by the difference: derived 1.01 cents flat at C8 at 384 kHz (a 96 kHz
host, or Oversampling 2×), 1.27 flat at 768 kHz and 0.25 sharp at 176.4 kHz
(44.1 kHz); measured through the engine −1.14, −1.46 and −1.14 cents at
Oversampling 2×, 4× and a 96 kHz host. The error grows with the square of the
pitch, so a 4' reed moved four times as far as the 8' reed an octave below
it, and octave ranks beat up to 2 Hz faster at the top of the keyboard.
`StepStiffness` now gives the tongue the stiffness that swings at the tuned
frequency at its step rate: the same three figures are now −0.12, −0.19 and
−0.12 cents. At 48 kHz with Oversampling 1× nothing changes, to the bit.
Test: `TopOctaveKeepsItsPitchAtEveryStepRate`.

`Tools/ReedProbe.h` said pitch was rate-independent ("96, 192 and 768 kHz
agree"); that held to a cent, not exactly, and the comment now says what
corrects it.

## Key range

`RespiraKeyboardSurvey` (`922c0cb`) plays every key of the treble keyboard,
the chord buttons and the bass buttons through a preset as it ships, on Auto,
with Room, Life and Touch at 0 and Mechanics at its lowest, at three bellows
pressures (100, 200 and 400 Pa asked for; 95, 190 and 380 Pa measured),
pulling and pushing. It reports level, pitch, onset (the first 10 ms within
6 dB of the settled level), brightness and the pressure at which each key
starts to speak in a 4 s crescendo, and marks keys that stand out from the
median of the three keys either side. The full tables for all eight presets on 0.1.23 are in
[`consolidation-audit-2026-11-09/keyboard-survey.md`](consolidation-audit-2026-11-09/keyboard-survey.md).
0.1.24 changes nothing at the survey's rate, so before and after are the
same tables.

Treble, pulling, at 380 Pa:

| Preset | Level, dBFS | Largest standout | Pitch, cents | Onset: median, slowest |
| --- | --- | --- | --- | --- |
| Clarinet Solo | −41.9 to −20.3 | −0.4 dB at A7 | −0.2 to +0.8 | 30 ms, 106 ms at C8 |
| Bassoon Low | −34.1 to −20.8 | +0.1 dB | −1.5 to +0.7 | 30 ms, 247 ms at C3 |
| Violin Folk | −40.3 to −19.5 | −0.6 dB at A7 | −2.4 to +26.0 (the tremolo rank) | |
| Paris Musette | −38.6 to −19.2 | +0.4 dB at B3 | −48.9 to +50.8 (the tremolo ranks) | |
| Tango Bandoneon | −35.5 to −20.7 | −2.2 dB at A#5 | −0.1 to +1.0 | 27 ms, 127 ms at C#3 |
| Organ Bellows | −32.6 to −19.5 | +2.3 dB at F#6 | −0.1 to +0.8 | |
| Bright Oboe | −39.2 to −18.7 | −2.0 dB at G#4 | −0.2 to +0.8 | |
| Master Tutti | −36.1 to −20.6 | −1.1 dB at A#5 | −22.4 to +20.5 (the tremolo ranks) | |

One reed across the keyboard (Clarinet Solo, 380 Pa):

| Key | Level, dBFS | Onset, ms | Speaks from, Pa |
| --- | --- | --- | --- |
| C3 | −20.3 | 104 | 80 |
| C4 | −21.8 | 33 | 62 |
| C5 | −24.3 | 23 | 62 |
| C6 | −27.5 | 17 | 62 |
| C7 | −31.6 | 41 | 97 |
| F#7 | −35.1 | 61 | 142 |
| A7 | −38.5 | 84 | 185 |
| C8 | −41.9 | 106 | 240 |

Verdict for each thing that stood out:

- **Single-rank registers** (Clarinet, Bassoon): no key more than 0.5 dB off
  its neighbours' trend at any pressure. Nothing to fix.
- **Multi-rank registers, ±1 to ±2.5 dB at single keys** (Bandoneon A#5,
  Organ F#6, Oboe G#4, Master A#5): slow beats between ranks an octave apart.
  A held note's level moves 3–5 dB over seconds even at Life 0 (checked on
  Oboe A4 and A#4 and Bandoneon A5), and the survey's 0.4 s window catches
  each key at a different phase of its beat. The instrument does this; no
  per-key trim was considered.
- **Chords step at F/F♯ and basses at D♯/E** (+2.4 dB and 1.8 times the
  onset at F♯2): the Stradella ranks fold there by design.
- **Bass onsets of 75–182 ms at 380 Pa** (126–295 ms at lower pressures):
  the low reeds' known slow start (Known gaps).
- **Bass pitch −1.3 to +4.1 cents at 380 Pa**: two octave ranks' partials
  mix in the measured peak; each rank alone is within its table.
- **Push against pull**: median difference 0.01 dB (0.29 dB at most on a
  single rank).
- **Soft playing is sharp**: at 95 Pa the middle keys play 3–6 cents sharp,
  the reed's pressure dependence the README describes; the table tunes at
  400 Pa.
- **The top octave**: level falls about 4 dB an octave up to C7 and 10 dB
  over the top octave, onsets lengthen from 41 to 106 ms and the keys need
  97–240 Pa to speak; at 95 Pa the eight keys from F7 are silent. This is
  the Known gap "the top octave needs mezzo forte". It is voiced (reed
  scaling and chamber), there is no owned capture to compare it with, and
  nothing in the model is wrong by a measurement. Not changed.

No key was found where the model, rather than the instrument, is the cause.

## Known gaps re-checked

- **CPU**: rewritten with today's figures.
- **Soft notes speak late**: re-measured with `CompareAttacks` on 0.1.23 and
  rewritten: 486, 156, 149 and 146 ms at A2 to A5 at 55 dBA against the
  paper's 110, 90, 90 and 60; 190, 66, 44 and 30 ms at 70 dBA against 80,
  110, 60 and 50. The README still carried the figures from before 0.1.20
  lowered the reeds.
- **Low reeds start late** and **The top octave needs mezzo forte**: the
  per-key figures there (A2–B2 206–240 ms and so on) date from before
  0.1.20 and were **not re-measured**: `CompareAttacks` covers A2 to A5 only.
  The survey's onsets above use a different measure (within 6 dB of the
  settled level, in bellows already at pressure) and are not comparable with
  them. The gaps themselves
  stand.
- The other gaps were read against the engine and stand as written.

## Stale text fixed

- `kBellowsGuard`'s comment described a compressor the constants no longer
  are (20:1 from −15 dBFS); it now says 5:1 above −18 dBFS over a 6 dB knee,
  as `Docs/decisions.md` (2026-10-28) records.
- README, Reed cells: a garbled sentence about the reed's set.

## Found and not fixed

- **The 10 % ceiling** (see the plan).
- **Subnormal transients that end by themselves**, each too short to
  measure: `OutputStage::Compress` squares a float that underflows about 5 s
  after a release (a shared portfolio file, for its master's owner); the
  room's difference of two tiny buffer values (about 1.3 s, 11–12 s after a
  release); `Decimator::Allpass` (about 5 blocks); one float cast in the
  engine (24 blocks). `NoSubnormalsWhilePlayingOrAtRest` checks the windows
  where a state used to stay, not these.
- **libm on per-sample paths**: the manual hand's `pow` (while a key is held
  and the hand moves slowly) and the pallet's `cos` (while a pallet moves),
  0.16 % of the instructions here. The Rack build links its own maths, where
  the cost is unknown: to be measured in Reason before changing the hand's
  law.
- **The folds at a bellows turn** cost up to 6–8 % of a core for one block
  and 3 % for 0.6–0.8 s after; a cheaper rule changes when panels click.
- **No level tool**: levels are pinned by unit tests (`PresetsSitTogether`,
  `TypicalLevel`), not by a `--check/--write/--report` tool as Aculele has.
  Enough for one bank of eight; noted, not started.

## The Rack device's failing checks

The Rack repository was read-only in this pass. Its Respira host contract
fails on the current engine because of the test fixture, not the adapter or
the engine: since 0.1.20 a sudden hard stroke chokes the lowered reeds
(Docs/decisions.md, 2026-11-07), and the fixture opened the bellows five
times faster than its held A4 can follow, then compared two silences; one
check also delivered a batch's changes out of frame order. A patch to the
fixture and a note are in
`/mnt/project-files/consolidation-2026-11-09/respira/rack/`; with it the
native contract passes against this engine. The Lua half of the Rack suite
needs the SDK and was not run.

## Tests

`unit`, `audio` and `RespiraDSPCxx17` pass. `perf` (`Respira.CpuBudget`)
fails on the 10 % ceiling for every demo but Clarinet Solo, as described
above; it was not changed.
