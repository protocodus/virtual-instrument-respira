# Respira

Respira is a physically modelled accordion with eight treble registers,
bass and chord buttons. Auto supplies bellows air while you play, from
endless bellows that turn only when a keyswitch asks; Life adds irregular
effort and movement at the microphones, even on long held notes.
Select Wheel to shape each pull and push yourself. Every sounding reed is a
simulated steel tongue in its own chamber, driven by bellows air. Mechanical
parts can also knock without air. This repository holds the JUCE-free C++17
engine, tools and tests; the Reason Rack Extension plays the same engine.
There is no native DAW plug-in yet.

## Audio demos

One phrase per factory preset, each written for what its register is played
for and rendered from `Docs/audio/midi/` through the shipping engine. The
bellows in each is a part of the MIDI: the mod wheel rocks through strokes
fitted to a target pressure each (`Tools/fit_bellows.py`).

<!-- audio-demos-begin -->

| # | Preset | Demo | Length | Peak |
| --- | --- | --- | --- | --- |
| 1 | Clarinet Solo | [01-clarinet-solo.wav](Docs/audio/01-clarinet-solo.wav) | 11.2 s | -4.1 dBFS |
| 2 | Bassoon Low | [02-bassoon-low.wav](Docs/audio/02-bassoon-low.wav) | 9.6 s | -5.1 dBFS |
| 3 | Violin Folk | [03-violin-folk.wav](Docs/audio/03-violin-folk.wav) | 10.4 s | -6.4 dBFS |
| 4 | Paris Musette | [04-paris-musette.wav](Docs/audio/04-paris-musette.wav) | 10.7 s | -6.4 dBFS |
| 5 | Tango Bandoneon | [05-tango-bandoneon.wav](Docs/audio/05-tango-bandoneon.wav) | 11.2 s | -1.5 dBFS |
| 6 | Organ Bellows | [06-organ-bellows.wav](Docs/audio/06-organ-bellows.wav) | 10.6 s | -4.4 dBFS |
| 7 | Bright Oboe | [07-bright-oboe.wav](Docs/audio/07-bright-oboe.wav) | 9.8 s | -6.5 dBFS |
| 8 | Master Tutti | [08-master-tutti.wav](Docs/audio/08-master-tutti.wav) | 8.7 s | -2.5 dBFS |

<!-- audio-demos-end -->

## Version history

### 0.1.24 — 2026-11-09 — the top octave keeps its pitch at every rate

- The reeds' tuning table is made at the 192 kHz the cells run at in a 48 kHz session, where it takes out the small sharpness of the tongue's time step (1.35 cents at C8). At any other step rate the same table left a remainder that grows with the square of the pitch: C8 played 1.1 cents flat in a 96 kHz session or at Oversampling 2× and 1.5 flat at 4× (and, by the same arithmetic, a quarter of a cent sharp at 44.1 kHz), and a 4′ reed moved four times as far as the 8′ reed of the same key, so octave registers beat about 2 Hz faster at C7 and more above it. Each tongue's stiffness is now corrected for the step it is played at: C8 holds within 0.2 cents at every setting. A 48 kHz session at Oversampling 1× plays as before, and the eight demos are the same to the bit.

### 0.1.23 — 2026-11-09 — no state left in subnormal numbers

- In a host that does not flush subnormal numbers to zero, three states sat in them, each costing 0.3–0.6 % of a core on the development machine: the key beds' modes above 2 kHz from 1.4 s after every landing until the panel's lowest mode had rung out (2.8 s), and at rest the air meter's smoothing (from 7 s after the last note) and Life's share of movement (from 28 s), both for good. Each is now zero below 1e-30. The eight demos are the same to the bit.

### 0.1.22 — 2026-11-09 — the first stroke no longer thumps

- The first stroke after a patch loads or the engine resets thumped: the bellows' 144 fold panels were all set up in the middle of their play and landed together on that stroke, 9 dB over the key's own knock on Auto's first note (−32 dBFS at the default Mechanics, −18 dBFS at +20 dB) and 33 dB over the folds of the next stroke the same way. The first pressure now finds every panel resting on the hinges it pushes them to, as a player's hold leaves them, so the first note sounds as the second does. A turn still flips the folds; notes and bellows are unchanged.

### 0.1.21 — 2026-11-09 — the mechanism knocks on wood, no longer pops

- Reason Studios' reviewers still heard **Mechanics** as a bug in 0.1.20f2. It was the knock itself: every key, button, pallet, register switch and the air button radiated its landing as one cycle of about 500 Hz, 2 ms long, that started with a jump, a pop louder than the note it began. Now each landing rings the panel the part lands on, the treble keyboard's or the bass side's button board, as a plywood plate: its modes from 210 to 4000 Hz, ringing for about a quarter of a second, each key driving them by where along the panel it lands. The owner chose this panel, a little darker than the first one offered, by ear.
- No two knocks ring alike: each landing moves the panel's modes by up to 4 % (at Life 0.5, scaling with it, none at Life 0). Eight landings of one key at one velocity now match about 72 % (98–100 % before). A harder stroke still knocks louder, by 18.5 dB from velocity 20 to 127.
- The knocks carry the energy they did, spread over their ring, so a single knock peaks 5–8 dB lower; at Mechanics +20 dB the Init patch's mechanism peaks 7.8 dB lower and sits 1.3 dB lower in level. Notes, bellows, folds and valves are unchanged.

### 0.1.20 — 2026-11-07 — reeds set low enough to speak at pianissimo

- Every reed now sits as high above its plate as Okada measured one (0.3 mm at C4, from 0.8–0.9 mm across C2–G#4 before). Soft notes speak about twice as soon: A4 at 55 dBA rises from −50 to −5 dB in 147 ms (311 before) and A5 in 144 (305), against the 90 and 60 Llanos-Vázquez et al. measured on a real accordion. At mezzo forte A3–A5 take 66, 42 and 29 ms (105, 50, 35). The owner chose this voicing.
- As on a real accordion voiced this way, a sudden hard accent can choke a held key: the bellows driven past about 1.3 kPa in 20 ms leaves most of D#3–G#4 silent. Accents up to 1 kPa, and crescendos to the arm's full 2.5 kPa over 0.6 s, speak.
- The demo phrases' bellows strokes are refitted to the new reeds.

### 0.1.19 — 2026-11-06 — turns and first notes arrive at their pressure

- Under Auto, a bellows turn in a short rest no longer comes in too loud. Assist multiplied the arm's motion by the open reeds' demand, so when a chord came back while the arm still swung at full speed, the pressure reached 2–6 times its level (2 kPa against 400 Pa) before settling. The arm now moves the bellows' own air and Assist only lets it go faster; the same turn arrives within 1.15 times its pressure.
- The first note under Auto speaks at once. Its stroke found no pressure and closed on it in a reaction time, so for its first 100 ms it sounded 20–25 dB under itself. A stroke that finds less than half the pressure it wants is now swung up at full speed, as a turn is: 90 % of the pressure in about 20–35 ms instead of 120–220.
- **Mechanics** defaults to +5.6 dB in Init and every factory preset, 60 % of the level of the +10 dB before (the owner heard the knocks too loud in songs). Saved patches keep their own setting.

### 0.1.18 — 2026-11-06 — no two strokes alike

- The owner heard eight chord stabs at one velocity as nearly one sound (their waveforms matched 96–99 %). In 0.1.17 a finger's stroke-to-stroke scatter reached only the key's knock; now it moves the key itself. Each stroke's velocity scatters by the 5.5 % a pianist's repeated strokes do (at Life 0.5, none at Life 0), and through the velocity's bounded travel it sets how fast the key and its pallets open, so each note's attack differs a little: a few tenths of a millisecond of travel, the most on hard strokes. The same stabs now match 41–91 %.
- Each release lets the key and its pallets close a little faster or slower, an assumption the knocks already made; each press of a register switch moves its slides, and each press of the air button its valve, at its own speed.

### 0.1.17 — 2026-11-06 — the mechanism no longer clicks as one

- Reason Studios' reviewers heard **Mechanics** at its higher settings as a bug. Two things made it so. Every reed valve of a chord, or of the whole bellows at a turn, slapped shut on the same sample at the same speed, so up to 28 slaps added into one spike, the loudest sound the mechanism made; and every stroke of a key landed exactly as the last. Now each valve lands its own flight time after it lets go (about 10 ms after a note ends, 5 ms into a turn), with its own lift and curl (±20 %, fixed per instrument), so a chord's valves patter over a few milliseconds and the turn's spike is about 10 dB lower at Mechanics +20 dB. Each key, button, pallet, register switch and air-button stroke lands a little faster or slower than the last, by the 5.5 % a pianist's repeated strokes scatter (at Life 0.5; none at Life 0).
- Notes, bellows and the eight demos' reeds are unchanged; the demos change only in their mechanism.

### 0.1.16 — 2026-11-04 — the wheel scales Auto's level

- Under **Auto**, the mod wheel scales the pressure the velocity asks for, down to 15 % of it at the top, so it fades a held note by the same amount however hard it was struck: about 16 dB on a held A4 on Init at velocity 1 and at 100. In 0.1.15 every velocity went to one 60 Pa at the top, only 9 dB down at velocity 1.
- The softest notes now reach 27 Pa at the top, under what the high reeds need: at velocity 1 a held note from E6 (MIDI 88) up stops there, as one from C7 (MIDI 96) up already did at velocity 100.
- The wheel mode is unchanged: the eight demos render bit for bit as in 0.1.15.

### 0.1.15 — 2026-11-04 — the wheel is Auto's volume at every velocity

- Under **Auto**, raising the mod wheel now slows the bellows down to 60 Pa at the top whatever the velocity: about 16 dB on a held A4 struck at 100 or harder and 9 dB at velocity 1, on Init. From 0.1.12 to 0.1.14 a note struck under 64 swelled instead, by only 3–8 dB, the opposite way from the Rack panel's volume pictures, and the owner heard the wheel do nothing in Reason.
- Velocity still sets where the wheel starts from. The wheel mode is unchanged: the eight demos render bit for bit as in 0.1.14.

### 0.1.14 — 2026-11-04 — every key to C8 at its own pitch

- The treble reeds go on to C8, a piano's top, past an accordion's C7: every key from C#7 up sounds at its own pitch, where C#7–C8 played an octave down. They speak at Auto's 400 Pa, need more air the higher they are (about 100 Pa at C#7, 270 at C8), and fall some 13 dB from C7 to C8 on Init. Touch's softest strokes do not start those from G#7 up.
- Every key below C#7 is unchanged: its reeds and tuning are the same to the bit, and the eight demos render bit for bit as in 0.1.13.

### 0.1.13 — 2026-11-03 — Auto's endless bellows and keyswitch turns

- **Auto** no longer turns the bellows by itself. Its bellows are endless: the hand pulls for as long as keys are held, with no four-bar strokes, no early turn when the air runs short and no use of the host's tempo or song position. In 0.1.12 a held chord turned every 8 s at 120 in 4/4, wherever the phrase was, and every bar on a big chord with Assist at 0.
- Two keyswitches on the treble channel turn them, with Auto on: **C0** (MIDI 12) latches pull and **C#0** (MIDI 13) push. A turn sounds as Auto's turns did; it starts at the keyswitch, so the pressure passes zero 10–35 ms later at the default Momentum. Played in rhythm, they give a bellows shake. With Auto off the two notes are the split keyboard's bass C and C# as before.
- Under Auto, what followed how far the bellows were open (their stiffness, the folds' strain and, with Life, the bass end's position at the microphones) stays at mid-travel.
- The wheel mode is unchanged: the eight demos render bit for bit as in 0.1.12. Front ends no longer pass the host's transport (`Engine::SetTransport` is gone).

### 0.1.12 — 2026-11-03 — Auto/manual bellows speed calibration and Life-scaled reed realism

- In **Auto**, with the mod wheel at `0`, note velocity (**Touch**) sets the bellows playing speed: normal/strong strikes (`velocity >= 100`) run at full playing speed (`400 Pa`, about −9 VU) with an initial attack accent, while softer strikes hold a brief reed-catching floor (`35 ms`) so short staccato notes speak immediately and then settle (`80 ms`) down to a gentler sustaining speed (`~180 Pa` at `velocity = 1`, about 8 dB softer so soft melodies stay warm and clearly audible). Legato velocity transitions smoothly speed up or slow down the bellows in both directions, and releasing a note while other keys remain held returns to the remaining keys' velocity.
- Raising the mod wheel (`0 -> 127`) during a held note moves toward the opposite dynamic: on normal/strong notes (`velocity >= 64`) it slows the bellows down to `60 Pa` (`15 %`, about −16 dB fade), and on soft notes (`velocity < 64`) it swells the bellows up to full playing speed (`400 Pa`).
- While on a stroke, the virtual arm yields when the target pressure drops so the bellows relaxes in `0.1 s` even on a single treble reed rather than waiting for the open reed alone to bleed off trapped air.
- In **Manual** mode (`Auto = 0`), sub-reference wheel speeds (`< 0.095/s`) are shaped by $v^{0.48}$ with wrist slack take-up below `220 Pa` and arm relaxation when slowing down mid-note, preventing quadratic pressure collapse at slow wheel speeds so `pp` passages catch promptly and remain clearly audible.
- **Life** now scales per-reed, per-side (push vs. pull), chamber, pallet-onset, and turbulent vortex-jet imperfections continuously via `ImperfectionScale(life) = 2 * life`: `0 %` is perfection (zero reed/chamber/pallet/vortex variation, push and pull identical), `50 %` (the new default on Init and every preset) models a standard well-made accordion in good shape, and `100 %` doubles all manufacturing and voicing tolerances.

### 0.1.11 — 2026-11-03 — fixes to the 0.1.10 mechanism and the folds' CPU

- The bass strap's knock follows the bellows end's own speed (litres × hand speed over the 0.41 × 0.19 m section: a few mm/s to a few cm/s). 0.1.10 took the hand's speed in wheel ranges a second as m/s, so it hit about six times too hard at the default Bellows and ignored it.
- The 6.5 Hz effort tremor of Life reaches the 12 % share it is written against; its filter was normalised to 0.40 RMS, not 1, so the control signal is 2.5 times what 0.1.10 gave. On a held Auto note the output does not measurably change (5–8 Hz level wobble 0.28 dB before and after).
- Two bass or chord buttons an octave apart open the same reed. It now keeps sounding until the last of them is let go; it used to stop when the later one came up while the other was still held.
- Each bellows panel steps its hinge once per host sample, not four times: 5–28 % less CPU on the eight demos (13 % in all). The demos null against 0.1.10 at −60 dB and no octave band moves by 0.05 dB; only the octave-apart bass reed is audible in a targeted clip (+1.6 dB for the 3.2 s the first button stays down, of a bass and chord held together), and none of it has been judged by ear. [Review and measurements](Docs/validation/review-2026-11-03.md); [CPU](Docs/benchmarks/folds-cpu-2026-11-03.md).

### 0.1.10 — 2026-11-03 — Life's tremor and the bellows' mechanics, recorded after the fact

- One commit changed the model without a ledger entry: Life's tremor, off-axis microphone shading, fold distension, the bass strap, pull-valve slaps and jet-to-reed force. No listening verdict is recorded for them.
- Measured on the eight demos, 0.1.9 and 0.1.10 together against 0.1.8: whole-file RMS within 0.5 dB and peaks from 0.8 dB lower to 2.4 dB higher; octave bands between 700 Hz and 5.6 kHz from 0.7 dB lower to 3.7 dB higher (mostly 1–2 dB up), up to 2.4 dB lower under 700 Hz, and 4–7 dB lower above 11 kHz. Against the published accordion harmonics (`CompareHarmonics`) the 2nd moved from 8.4 to 4.7 dB under, the 9th–14th from −11.0 to −8.8 dB on average (measured −11.3) and the 15th–20th from −13.9 to −11.5 (measured −13.4).

### 0.1.9 — 2026-11-02 — reed and chamber physics, recorded after the fact

- Five commits changed the reed model without a ledger entry: the slot flow scaled by the tongue's profiled tip, damped duct standing waves, a finite tip thickness with a curved slot entry and slit loss, shared pallet-hole coupling between ranks with bellows compliance, and keybed resonances with a pallet-opening chiff. No listening verdict is recorded for them; their measurement is under 0.1.10.

### 0.1.8 — 2026-11-02 — microphone colour during movement

- Life movement now changes the low-frequency colour of the direct microphone pickup through the relative proximity response of the existing ideal cardioids. The owner chose B, **“B but tiny”**, in the level-matched comparison; this is a small preference by ear, not measured acoustic calibration.
- Motion depth, the reference voicing, Room and reed behaviour retain their existing settings. Life zero remains exactly neutral. [Model and scope](Docs/research/microphone-motion-2026-11-02.md); [validation](Docs/validation/microphone-motion-2026-11-02.md).

### 0.1.7 — 2026-11-02 — lower CPU during sparse playing

- Reuses the active reed list within each callback. Same-input baseline and optimized renders remain byte-identical across dynamic controls, rates, quality modes and callback sizes.
- Native M1 Max measurements show about 11.5% lower single-note thread CPU with the 0.1.8 microphone change present; dense-playing gains are small or variable. This is not Reason CPU evidence. [CPU-only measurements](Docs/benchmarks/active-cells-cpu-2026-11-02.md); [combined measurements](Docs/benchmarks/microphone-motion-cpu-2026-11-02.md#combined-with-the-separate-active-list-optimization).

### 0.1.6 — 2026-11-01 — short soft key presses

- Restores bounded 12–3 ms key travel. The 0.1.5 inverse-velocity extension could keep the pallet almost closed throughout a short, very soft note (478 ms travel at velocity 1). Ordinary/fast velocities 64–127, Touch, Life and all factory settings are unchanged.
- This corrects the low-MIDI-velocity regression. It **does not fix** the separate slow growth of fresh reeds at very low bellows pressure. Recording measurements, rejected physical-model candidates and their limits are in the [soft-start investigation](Docs/research/soft-starts-2026-11-01.md).

### 0.1.5 — 2026-11-01 — Touch and gentle key openings

- **Touch** turns keyboard emphasis into a brief, shared Auto bellows gesture. The owner chose C (100%) in the level-matched comparison; Init and all presets use that setting. Manual bellows control is unchanged.
- Deliberately soft velocities below 64 can open their pallets more slowly, changing the attack through the physical air/reed interaction. Ordinary and fast key travel is unchanged. No sustained velocity gain, tuning offset or brightness envelope was added.
- Old patches acquire the Touch default. Set Touch to zero to remove its Auto emphasis; the soft-key mapping remains a behavior change. [Research, listening scope and remaining attack mismatch](Docs/research/touch-model-2026-11-01.md).

### 0.1.4 — 2026-11-01 — Life and prepared bellows turns

- **Life** controls irregular Auto bellows effort and source movement at the microphones, including long held notes. Init and all presets use 100 %, selected by the owner from a level-matched listening comparison.
- Auto prepares a pending reversal during a short rest, so the next fast note has air; with Life, capacity planning can turn on beats when a full bar does not fit.
- New Life properties default to 100 %, including when absent from an older patch. Zero removes the added effort and movement; the rest-turn correction remains.
- Motion targets update at 200 Hz. This is a voiced virtual performer, not a fitted human-motion measurement. See [model and noise audit](Docs/research/life-model-2026-11-01.md).

### 0.1.3 — 2026-11-01 — less work for held aftertouch

- A settled held key skips an unused opening calculation. A native M1 Max
  test of 28 chambers with aftertouch used about 2.5 % less thread CPU;
  ordinary playing showed no reliable change. Twelve dynamic render
  sequences remained bit-identical across four rates and all quality modes.
  This is not a host CPU or deadline guarantee. [Measurement notes](Docs/benchmarks/aftertouch-cpu-2026-11-01.md).

### 0.1.2 — 2026-11-01 — room microphones and consistent bellows friction

- Room follows Width's virtual microphone angle, including mono at zero.
  Floor and ceiling reflections account for their elevation. The diffuse
  tail uses the correlation of ideal coincident cardioids, instead of
  fixed-angle room outputs; its image is narrower than in 0.1.1.
- Bellows friction retains its audible-band level across sample rates.
  The 48 kHz rustle is unchanged; the former approximately 3 dB loss at
  96 kHz is corrected with a setup-only coefficient.
- `CompareAttacks` reports fundamental rise times against a published
  accordion measurement, with estimator sensitivity and apparent latency
  separately. It documents the remaining soft-attack gap; reeds and their
  tuning are unchanged.

### 0.1.1 — 2026-11-01 — finer Room control

- Room uses a sigmoid response for finer adjustment near dry. Init and all
  eight factory presets use 0.005 wet (0.5 %), the owner's new default, and
  the percentage display shows the actual wet amount to one decimal place.
- The mapping changes only the control response; the hall and its
  per-sample processing are unchanged. Old normalized Room positions now
  use the new curve, changing their wet amount except at 0, 0.5 and 1.
  Physical Room values passed directly to the engine keep their meaning.

### 0.1.0 — 2026-10-27 — the accordion engine

- The engine from the prototype: reed cells with push and pull tongues in
  their chambers, one bellows, eight treble registers, bass and chord
  buttons, a split keyboard, the mechanism's knocks, Auto, Room and eight
  factory presets. Its development to 0.1.1 is in the
  [engine development history](Docs/release/engine-development-0.1.0.md).

## Downloads

<!-- build-download-begin -->
No main build yet: there is no plug-in to package. The engine builds from
source (see Build).
<!-- build-download-end -->

---

## How it works

### The hand and the bellows

The mod wheel (CC1, with CC33 for 14 bits) is the hand's position, from the
bellows closed (0) to fully open (1). A wheel sends steps, so the hand keeps
the speed of the last step until the next one arrives, slowing if it does not,
and closes on the latest position: even a 7-bit wheel moved slowly gives a
steady air flow. The arm follows that path with **Momentum**, as a critically
damped mass with that time constant (25 ms by default, voiced): it trails a
steady stroke by twice that time, turns the bellows round in about 84 ms where
the wheel turns at once, and never travels past the wheel; at 0 the wheel
moves the bellows directly. The hand moves air at its speed times **Bellows**, the litres
one full sweep of the wheel moves (2 L by default, a hand's stroke rather than
the whole bellows), and the bellows pressure follows
dp/dt = K (Q_hand − Q_reeds − Q_leak), with K = 4·10⁶ Pa/m³ (the air in the
bellows stiffened by its folds and the player's arm), clamped at ±2.5 kPa,
what an arm can put on a bellows. A still hand moves no air, so the reeds
fall silent as the open reeds use up what is left; only the mechanism still
knocks (see The mechanism).

A real accordion needs far more air for a full chord than for one reed: a
full register with bass takes 4–5 L/s against a tenth of a litre for one A4
reed. **Assist** scales the hand's air by the reeds that are open, so one
wheel speed gives about one pressure whatever is played: at 1 entirely, at 0
not at all (the strict instrument), and halfway by default. The **Air**
switch, or a held sustain pedal (CC64), lifts the pad off the air valve, a
25 mm hole in the bass end, in the time a pallet takes, so the hand can
reposition the bellows without the reeds speaking. Air leaves through it as
through a reed gap, by Bernoulli's law with a laminar term (4 L/s at 100 Pa),
and its jet is turbulent: eddies about the pad's 3 mm lift fluctuate
independently round the hole's edge, and the flow's fluctuation radiates as
the reeds' flow does, through the bass box (see Radiation): the hiss of a
working air button.

### Auto

The pressure figures below describe the baseline at Life and Touch zero;
Life and Touch vary the pressure the hand wants (below).

**Auto** is on by default, on Init and on every factory preset: just play,
and a player's hand keeps the bellows supplied while any key is held; the
wheel no longer moves them. Switch it off to play the bellows on the wheel.
Auto's bellows are endless: the hand goes one way for as long as keys are
held, however long a note or however much air a chord takes, and never turns
by itself (the owner's choice, 2026-11-03). No real bellows is endless; the
departure is in their capacity only, and every note still sounds from the
air the hand moves. Two keyswitches on the treble channel turn them: C0
(MIDI 12) latches pull and C#0 (MIDI 13) push, so a region plays the same
wherever playback starts, and one that repeats the way the bellows already
go changes nothing. They sit an octave under the bass buttons, where the
split keyboard only repeats them, and with Auto off they are keys again.
After a reset the hand pulls, as a player starts with the bellows closed.

The hand does not move at a fixed speed: like a player it feels the air the
reeds take and gives it, and closes on the pressure it wants in a reaction
time of 0.1 s at every Momentum. It knows its own arm: it leads the arm by
the trail it would keep behind a steady stroke, so the arm follows the
stroke the player means and its mass shows only where the stroke changes, as
a softer, slightly longer turn; and it closes on the pressure the arm will
bring once it catches up, so the arm's lag is not in its loop. It wants 400 Pa, the pressure the demo phrases' ordinary
strokes are fitted to and about −9 VU, whatever is played and whatever
Bellows and Assist are set to; raising the wheel scales that down to 15 %
(60 Pa at the top on an ordinary note) and lets the arm yield to the lower pressure,
so moving the wheel from its resting place at 0 softens and swells a held note
by about 16 dB.
A turn is planned rather than reacted to: at the keyswitch, expecting the
reeds to take the air they took, the other way, the hand moves as fast as it
goes (4 L/s of its own air, more with Assist) until the pressure its arm will
bring is the new one, so at the default Momentum a held chord falls more than
10 dB below its level for 4–17 ms. The engine cannot look ahead, so the
pressure passes zero 10–35 ms after the keyswitch at the default Momentum,
less on a big chord, whose reeds' air helps the swing: put a keyswitch just
before the beat it turns on. A heavy arm turns slowly, as its bar-timed turns
did: at Momentum 100–200 ms a bass, chord and triad takes 0.1–0.6 s to pass
zero. Playing the keyswitches in rhythm gives a bellows shake; at the default
Momentum five turns a second still swing the pressure fully round. A
keyswitch in a rest, once the hand has played, turns the pressure with the
pallets closed, so the next note speaks the new way at once. The arm moves
the bellows' own air and the hand asks it for the air the reeds need, so a
chord that comes back while the arm still swings arrives at its pressure (at
most 1.15 times it). A stroke that finds less than half the pressure it
wants, as the first note does, is swung up at full speed as a turn is: from
0 to 200 ms of Momentum a held triad reaches 360 Pa in 10–360 ms and holds
400. Each
stroke starts from rest; let go of every key and the hand stops, the closed
pallets keeping the bellows at their pressure, so the next note speaks at
once. Nothing that depends on how far the bellows are open moves under
Auto: their stiffness, the folds' strain and, with Life, the bass end's
position at the microphones stay at mid-travel (the Bellows control is only
the wheel's gearing). Switched off, Auto leaves the bellows where they are:
the wheel takes over without a stroke. Pressure and reaction time are
voiced, not measured.

The hand is a person's, who never presses quite evenly. The pressure it wants
wanders by 8 % of itself, slowly (below about 1.5 Hz), and each stroke takes
its own level, 6 % either way at one standard deviation, the range force
steadiness reaches in a moving arm (a coefficient of variation of a few
percent in a steady contraction, more when the limb moves). In a held chord
the pressure varies by about 5–6 % within a stroke where it varied by 1 %.
The sound follows only weakly: at 400 Pa the level rises about 0.26 dB per
dB of pressure, so what is heard is mostly the reeds' colour and the
beating's evenness moving, by 0.1–0.2 dB in level.

### Life

**Life** scales both performer movement and physical reed/chamber imperfections:
- `0 %` (`ImperfectionScale = 0`): **Perfection** — zero reed, chamber, pallet-onset, or vortex-jet variation, identical push and pull reeds, and zero performer movement.
- `50 %` (`ImperfectionScale = 1`, the default on Init and every preset): **Standard well-made unit** with quality manufacturing in good shape (`±6 %` tip-set height, `±12 %` Q damping, `~3 cents` RMS tuning jitter, `±12 %` frame slot clearance, `±6 %` plate slot depth, `±8 %` tongue profile, `±10 %` chamber wall loss with inner/outer reed-block bias, `±0.6 dB` cell trim, `±16 %` secondary-rank pallet onset skew, and `0.35` vortex-jet inertia coupling).
- `100 %` (`ImperfectionScale = 2`): **Over-emphasized** (`2x` tolerances across all reed, chamber, pallet, and vortex parameters).

In addition to per-reed and per-chamber tolerances, **Life** adds three slow, irregular movement signals, shared by the whole
instrument. One varies Auto's requested effort; lateral and depth motion
change the instrument's position relative to fixed microphones. These keep
moving through held notes, without repeated key presses or MIDI expression
curves. Zero removes these additions; the earlier Auto wander remains.

Pressure still comes from the hand moving bellows air. No pitch or audio
amplitude modulation is added to the reeds. In Wheel mode the user's hand
and air remain unchanged, so Life changes only microphone pickup. The
pickup uses ideal cardioid reception and relative inverse-distance pressure,
with nominal motion bounded by 5 cm sideways and 8 cm in depth at full Life.
Bass-side position also follows virtual bellows opening. Its relative
proximity response adds a small change in low-frequency microphone colour
as the source moves, using four one-pole states. The reference pose and Life
zero add no correction; Room retains its surrounding-field response. Width
zero remains mono. Rates and movement extents are performer choices, not
measured accordion constants; no Doppler or frequency-dependent instrument
radiation is modelled. See the [microphone model](Docs/research/microphone-motion-2026-11-02.md).

The motion clock runs at 200 Hz, independent of host block size. Microphone
targets retain their 20 ms glide. A separate random stream preserves the
reed-turbulence sequence. Details, sources and the remaining movement-noise
gaps are in the [Life audit](Docs/research/life-model-2026-11-01.md).

### Touch

**Touch** interprets key velocity as shared bellows effort in Auto. Its
default is the owner's chosen 100 %. Strong presses (`velocity >= 100`) add a
brief initial accent (above `velocity = 64`, relaxing over 200 ms) and sustain
at full playing speed (`400 Pa` at `wheel = 0`), while softer presses hold a
brief reed-catching floor (`35 ms`) for short staccato notes to catch at once
and then glide over 80 ms down to a gentler sustaining bellows speed (`45 %` /
`~180 Pa` at `velocity = 1`). Raising the mod wheel (`0 -> 127`) during a held
note scales that speed down to `15 %` of it at the top, the same fade at
every velocity, so the wheel is Auto's volume (0.1.16). A chord requests one
shared gesture from its strongest key, and legato notes or key releases while
other keys remain held smoothly transition the bellows speed in either
direction. At zero the added velocity response is off. It never changes the
manual hand. The control runs at 200 Hz and acts through the existing physical
bellows; its strength and time constants are performer choices, not measured
constants. See the [Touch study](Docs/research/touch-model-2026-11-01.md).

### Reed cells

Each sounding reed is a cell (`Source/DSP/ReedModel.h`): a push reed and a
pull reed on one plate, each a steel tongue (its first cantilever mode, with
the tip as coordinate) that the pressure drop blows into its slot, with a
leather valve so it passes air one way only, both venting through the chamber
and the key's pallet hole. The air in the slot and the hole has inertia, so
the pressure across a closing gap rises while the tongue moves into the slot;
the force leads the displacement and the tongue self-oscillates (Fletcher
1979; Ricot, Caussé and Misdariis 2005; Millot and Baumann 2007). Nothing is
an oscillator: no air, no sound. The radiated sound is the rate of change of
the flow out of the pallet hole, as from a small source 1 m away, with
0.67 Pa (90.5 dB SPL) at digital full scale, where ordinary playing reads
about −9 on Reason's VU meter.

The jet through the gap is turbulent. Its eddies are the size of the gap
where they form, so along the gap's edge each millimetre holds one over the
gap there of them, each passing the jet's speed times the gap squared and
fluctuating by about 5 % (half the 10 % estimated, chosen by ear). The gap is
the tongue's bend, growing as the square of the distance from its root along
both sides to the tip's opening across its width, so the flow fluctuates by
0.05·√(∫h³)/∫h of itself, most of it from the widest part of the gap, near
the tip: a little breath around the tone, its eddies largest and darkest on
the big low reeds. Its spectrum is the eddies': flat up to their rate, the
jet's speed over 2π times their size, falling above it, as the air valve's
does; at 500 Pa about 6 kHz for a C4 tongue 1 mm open and 18 kHz for an A5
tongue 0.3 mm open. The same random draw, white, is the small force that
seeds the tongue.

Tongue length and width follow typical accordion reed sizes scaled with
pitch, the thickness is whatever gives the pitch, and the low reeds carry
brass tip weights, as real ones do, enough that 1 kPa bends a tongue no more
than an eighth of its slot's depth. Every tongue is set far enough above its
set 0.3 mm above its plate at C4, as Okada measured one (more on longer
tongues, less on shorter), low enough to speak at once at pianissimo. Set
that low, a still tongue that a sudden forte accent drives into its slot
chokes, as repairers say of reeds set low: here a held D#3–G#4 struck by
the bellows past about 1.3 kPa in 20 ms. A tongue already swinging carries
on through its slot to the arm's full pressure. Short treble tongues are filed thin towards
the tip: from A5 up a tongue keeps 0.18 of a uniform one's tip mass, a linear
taper to the 0.05 mm a repairer's guide gives the smallest (Accordion
Revival), rising to 0.6 by E4.
The reed plates, and so the slots, are 2.0–3.5 mm deep, an estimate chosen by
ear: at mezzo forte the low tongues swing inside them. Each chamber is a
channel as long as its reed, closed under the tongue's tip and open at the
root end to the pallet hole, simulated as a duct in sections of at most 5 mm
(halving them moves the harmonics by under 1 dB below 8 kHz and 2.5 dB
above); the gap's air enters, and its
pressure acts on the tongue, along the slot where the gap opens. The pallet
hole's outer end radiates: its air mass works in parallel with the radiation
resistance ρc/A, as a piston in a baffle does (Beranek), so the chambers'
standing waves ring at Q 26–48. A chamber whose resonance would fall near its reed
is made smaller, as reed makers do with wax in the top octaves. Against the
one published harmonic measurement of an accordion's 8' reeds (A#2–A3, mezzo
forte, 50 cm; `Tools/CompareHarmonics.cpp`), the 2nd–14th harmonics agree
within 2.9 dB on average at its loudness, the 2nd 1.6 dB under, but the
9th–14th average −6.9 dB against the measured −11.3 and the 15th–20th
−12.0 against −13.4. While the tongue seals its slot, the air it sweeps
through arrives almost opposite the gap's own 2nd harmonic, which holds the
2nd down. A lab measurement of real
accordion-type reed blocks on a tuning jig hears each reed's own flow, its
chamber upstream (Pomberger et al., DAGA 2021: E3, G4 and E6, 26 cm away, at
a pressure not given). Through the paper's processing, Respira's reed flow at
208 Pa agrees with it within 2 dB over the 2nd–5th harmonics, but not above
the 10th (Known gaps, "The top octaves").
Every push and pull tongue differs a little
from its neighbours (set ±10 %, loss ±20 %, fixed per instrument) and is
tuned a little off its pitch, 3 cents standard deviation, as La Melodiosa's
and a Hohner's reeds are (`ReedBank.h`). A reed sounds below its natural
frequency under load, so `Tools/GenerateReedTuning.cpp` simulates every reed
in its chamber at 400 Pa until its pitch settles (a C2 takes some 3 s, the
16' rank's C1 some 10 s) and writes `Source/DSP/ReedTuning.h`, the
natural-to-sounding ratios the engine builds reeds from at note-on. Pitch
still moves with pressure as a real reed's does: an A4 sounds 1.3 cents under
its tuning at playing pressure (about 550 Pa) and about 11 cents under when
pressed at 1.4 kPa. Against a bandoneon's held notes
from C2 to C6 (`Tools/CompareBandoneon.cpp`, from a Pianobook library by
Miguel Lopez), read against loudness so that neither the bellows pressure nor
the microphone matters: over the loudest 30 dB, with the loudest at 1.5 kPa,
single 8' cells go 0.50 cents flat per dB louder on average against the
bandoneon's 0.39, and their 3rd, 5th and 7th harmonics gain 0.36–0.50 dB per
dB on the fundamental against its 0.64–0.80. Over the loudest 15 dB they fall
twice as far, and from E4 up their 9th–15th harmonics hardly grow at all
(Known gaps).

The cells run at four times the host rate (192 kHz at 48 kHz) and are
decimated by two half-band allpass stages; Oversampling at 2× or 4× runs them
at eight or sixteen times the host rate through three or four stages, with
the gap turbulence scaled so its spectrum stays the same at every host rate
and Oversampling, and each tongue's stiffness corrected for its time step, so
the tuning made at 192 kHz holds at every rate. Key travel takes 12–3 ms across the MIDI velocity range,
with 7.46 ms at 64. The unbounded low-velocity extension in 0.1.5 was reverted:
short soft strikes must still open their pallets. This is an authored MIDI
mapping, not a measured key-speed calibration. Pallet area follows the existing raised
cosine of travel, so velocity shapes the attack, and the key closes in 8 ms;
in the last tenths of a millimetre the air leaves through the film under the
pad's rim, a viscous gap that brings it to rest as the pad lands.
Channel pressure lets a held key rise part way: at full pressure its travel
rests at 0.38, a pallet opening of 0.32, where the model's C4 falls the
5.1 Hz per kPa of bellows pressure measured with the pallet lifted 1 mm
(Adachi et al., Forum Acusticum 2026); fully open it falls 2.3 Hz/kPa
against the measured 2.5. At the 70 dBA of a published bend measurement, a
half-pressed A#2–A3 at a steady bellows pressure falls 9.1 cents and loses
12.5 dB, where a player's fell 16 cents and lost 11 with the finger alone
(Known gaps).
Up to 72 chambers sound at once; a new one takes
the oldest released chamber first.

### Registers and the bass side

The treble has five ranks of reeds: L (16', an octave down), M (8'), M− and
M+ (8', tuned below and above M for the tremolo) and H (4', an octave up,
breaking back an octave above C♯7 to sound with the 8'; where M is open it
stays shut there, since it would sound in unison with M a few cents apart and
the two would beat slowly to near silence). Every key from C2 to C8 has its
own reeds, a piano's top octave above an accordion's C7 included, so every
key sounds at its own pitch; keys above C8 play an octave down. Those top
reeds speak at Auto's 400 Pa but need more air the higher they are (Known
gaps, The top octave needs mezzo forte to start), and from C7 to C8 a note
falls some 13 dB on Init. The
eight register switches combine them: Clarinet (M), Bassoon (L), Violin (M,
M+), Musette (M−, M, M+), Bandoneon (L, M), Organ (L, H), Oboe (M, H) and
Master (all five). A switch moves slides over the reed blocks, so it reaches
held keys too: the ranks it opens start to speak as from the softest key
stroke and the ones it shuts fall silent. Each register's treble side is
trimmed so all eight average one level on a C3, a C4 triad and a C6: untrimmed,
Master is 1.9 dB louder than Musette and a single rank 3.5–3.9 dB softer, as
on a real accordion, but a switch here changes the colour, not the level.
**Musette** sets the M+ detune at A4 in cents, and M− sits the same
interval below. The beat rises with pitch as f^0.51, as a Hohner's two-reed
musette beats (Marek Kintosh's samples, C4–C6: 0.25–0.75 at 90 %, its notes
scattering 28 % about the line), 4.6 Hz or 18 cents at A4, between Violin
Folk's 14 cents and Init's 22 (`Tools/measure_musette.py`). So a low chord
beats faster and a high note slower than a fixed number of cents would
have them: against A4, a C3 beats 0.54 times as fast and a C6 1.56 times.
With each reed's own tuning (above), Respira's beats scatter 22–25 % about
their law from C4 to C6.

A bass button sounds two reeds, a 16' (E1–D♯2) and an 8' (E2–D♯3); a chord
button sounds one pitch class on an 8' (F♯3–F4) and a 4' (F♯4–F5), so any
chord can be built from chord buttons. The bass reeds sit behind larger
pallets and longer passages, and sound through the bass box (see Radiation).

### Radiation

The treble pallet holes open into the compartment under the grille, 25–45 mm
deep. It is not a resonator: as a lumped one, a grille 30 % open with 3 mm
slots in 1 mm sheet would put its resonance where the compartment's depth is
over a quarter of a wavelength, outside what a lumped model describes, and as
a layer of air between the pallet board and the grille, averaged over those
depths, it passes the reeds within 3 dB up to 16 kHz. The treble reeds sound
straight out; **Grille** sets the treble side's brightness, a one-pole
low-pass, voiced rather than modelled. Its default, 1 kHz, is fitted to two
sample libraries' held notes (La Melodiosa by Petri Pohjanmies, CC BY, and
a Hohner by Marek Kintosh): at 9 kHz, as before, Clarinet Solo's C3–A5 read
6, 9 and 15 dB more than theirs at 2–4, 4–8 and 8–16 kHz against 1–2 kHz;
at 1 kHz the octave bands from 500 Hz up sit within 4 dB of both on
average (`Tools/compare_accordion_tone.py`). The presets keep their voiced
spread, each scaled the same way (667 Hz–1.6 kHz). Tango Bandoneon's
722 Hz sits at a bandoneon library's fit: against its held notes C2–C6,
the harmonics from 2 to 16 kHz read within 2.5 dB on average, and every
Grille from 500 Hz to 1 kHz fits as well, within 0.3 dB
(`Tools/compare_bandoneon_tone.py`).

The bass pallets open into the bass box, the space under the bass end cover
round the bass mechanism, which vents through the cover's holes: a
perforated-panel Helmholtz resonator, a second-order low-pass on the bass
side. Per unit of cover area its air mass is ρl′/open and its compliance
depth/ρc², so f = c/2π·√(open/(depth·l′)), loaded by radiation (ρc) and the
cloth's flow resistance. Typical values (40 mm deep, 10 % open, 6 mm holes in
3 mm wood, cloth of 30 rayl) give 960 Hz at Q 1.3, a 3 dB rise before the
roll-off; their plausible spread is 0.55–2.0 kHz. The box is shallow enough
there for a lumped model, which holds within 2 dB up to 2 kHz. Above that it
is a layer of air ringing at its depth modes, from c/2·depth (4.3 kHz), where
the lumped resonator alone falls 12 dB an octave, darker even than a box that
absorbs every reflection. Averaged over the box's irregular depth (30–50 mm)
the modes pass what reaches them, less the fraction a of its energy the box
absorbs on each round trip: a leak flat up to 2c·open/(l′√a) and falling
6 dB an octave above, which joins the resonator through two high-passes at
2 kHz (within 2.6 dB of the exact layer in every octave band from 1 to
16 kHz). a, 4 % (6.7 kHz), is fitted to two sample libraries' bass buttons
(La Melodiosa and the Hohner): from 2 to 16 kHz against 0.5–1 kHz the bass
side sits within 2.3 dB of both but for the Hohner's top octave (7 dB),
where the lumped resonator alone read 11–44 dB under them
(`Tools/compare_accordion_tone.py`). The air valve opens into the bass box as
well, so its hiss leaves the same way.

### The microphones

At Life zero, the output is what a coincident pair of cardioid microphones (XY) hears from
1 m in front of the player, in the player's view, the treble on the right.
The treble keyboard stands upright, so its pallet holes and the grille over
them are one place across the pair, 0.20 m right of the bellows' middle; the
bass end cover is the other end, 0.25 m left. A coincident pair hears them
by level alone: each side reaches each microphone as 1 + cos of its angle
off that microphone's axis, at the side's own power. **Width** turns the
axes apart, from one microphone (0) through the usual XY pair's 90° (0.6,
the default: the treble 1.4 dB to the right, the bass side 1.8 dB to the
left) to 150° (1: 2.6 and 3.3 dB). The folds sit between the two, in the
middle. Earlier measurements of five solo recordings found near-zero
interchannel lag and modest level differences, consistent with this model
but insufficient to identify their microphones; the demos with bass now differ by up to 2.1 dB, where panning
each key by pitch put Paris Musette's bass band 10 dB to the left. The
positions are a full-size instrument's, estimated.

**Room**, at 0.5 % wet on Init and on every factory preset, puts a room
around the pair: the late, diffuse field of a small hall with a hundred
people in it, from a sixteen-line feedback delay network (`Source/DSP/Room.h`)
behind four allpass diffusers, so it is dense from its first milliseconds.
Its lines add up to 0.76 s of delay, so its modes lie 1.3 Hz apart, and each line's length drifts slowly by 0.125 ms,
so a held chord's tail never rings metallic; it costs about 1 % of one core
in the development container. Its decay is Sabine's for the estimated hall
(12 × 9 × 4 m, 432 m³) with each person adding the absorption measured per
person in an occupied hall (Chagok et al.), 0.29 m² at 125 Hz rising to
0.73 m² at 4 kHz, and the air its own: 1.03 s at 125 Hz falling to 0.56 s at
4 kHz, fitted as 0.83 s at low frequencies and a quarter of that at the top.
Before it come the first reflections of that hall, by image sources (the
stage, the wall behind the player, the ceiling and the side walls), each
heard by the pair from its own direction: the stage's arrives 4.1 ms after
the instrument, the side walls' at 23 ms. Floor and ceiling pickup includes
the elevation of each image source, and all reflections follow Width. The
reflections are softened above 2 kHz and the late field above 3 kHz,
voiced, for the owner's "quite dark".
The late readout uses two orthogonal combinations of the sixteen lines,
with equal theoretical energy. Their mix follows the ideal diffuse-field
correlation of coincident cardioids: `(3 + cos(angle)) / 4`, where angle is
zero to 150° according to Width. At the default 90° the target is 0.75;
at zero the two outputs are identical. The finite delay network approximates
that diffuse field, and short tonal passages need not have that correlation.
Width changes glide over 20 ms. This changes the room's image and some tail
levels compared with 0.1.1; no gain was fitted to restore the old decorrelation.

The supplied performance recordings do not identify a unique room or stereo
setup. Rechecking the five MP3s on 1 November found one nearly mono and
whole-file ordinary correlations of 0.727–0.886 in the others. These are not
the phase-transform metric previously used to voice Room, and neither
measure alone establishes microphone placement or reverb time. Their notes
rarely stop cleanly enough to infer a room decay.

Room's normalized knob position `x` maps to the physical wet amount through
`x³ / (x³ + (1 − x)³)`, giving finer control near dry. A wet amount of
0.005 (0.5 %) lies at about 14.62 % travel and 0.01 (1 %) at 17.77 %;
the endpoints and midpoint stay 0, 1 and 0.5. The percentage display shows
the wet amount, not the travel, to one decimal place. This calculation runs
when mapping controls; it adds no work to the per-sample room processing.

Older normalized Room values are interpreted through this new curve.
For example, a saved position of 0.05 gives about 0.0001458 wet (0.0146 %),
where it gave 0.05 wet before. Restore an old room level by setting its
displayed wet percentage to the former value. Direct engine callers that
set physical Room values keep the same response at the same value.

### The mechanism

The keys, buttons, pallets, register switches and air button knock on their
stops, with or without air. Each knock is a collision: a part of effective
mass m reaches its stop at the speed the engine moves it and the stop's felt
brings it to rest. The felt's force is a measured law, (a x³ + b x² + c x)(1 +
d ẋ) at compression x, fitted to a grand piano action's stops (Hirschkorn,
Waterloo 2004): the felt under a key's front where a pressed key lands, and
the back rail cloth a released key returns onto, which stands in for the
pallet's leather on its seat. A treble key and the fingertip on it (20 g, 5 mm
of dip) land at the end of the key's stroke, 0.4 m/s for a soft key and
1.7 m/s for a hard one; a pallet (10 g at the pallet, 5 mm of lift, taken to
be the key front's dip) lands when its spring has closed it, 8 ms after the
key is let go, at 0.6 m/s whatever the finger did. Bass and chord buttons
(15 g, 3 mm) land like keys and their pallets like the treble's; a register
switch (30 g, 5 mm) knocks as it is pressed, at the speed of the softest key
stroke, the time its slides take, and the switch it pushes off springs back up
onto its stop in the time a pallet's spring takes; the air button lands like a bass button and
its pad like a bass pallet. A key let go before it lands, or held half
down, does not knock until its pallet seats; one pressed again before its
pallet seats does not knock on the way up. Each key's knock differs a little
from its neighbours' (its mass ±20 %, fixed per instrument), and no two
strokes land alike: each landing speed scatters by 5.5 % (standard
deviation, at **Life** 0.5, scaling with it and none at Life 0), as expert
pianists' repeated strokes at one loudness do (4.1 MIDI velocity at a target
of 75; Tominaga et al., PLoS One 2016). The spring's return as a key is let go
takes the same scatter, for the finger rides it part of the way; nothing
measures that. Masses and travels are estimates from typical sizes.

The felt stiffens as it is compressed, so a harder stroke lands not only
louder but shorter and brighter: a soft key's knock lasts about 3.4 ms and
peaks near 280 Hz, a hard key's 2.6 ms, near 550 Hz. The force rings the
panel the part lands on: the treble keyboard's, or the bass side's button
board, each a simply supported plywood plate (4 mm, 8 GPa, 650 kg/m³; 0.42 and
0.30 m long; Leissa, NASA SP-160, 1969), as deep as puts its lowest mode at
210 Hz, where a real accordion's mechanism sound begins (Niewczas, Łódź 2019,
Fig. 50, which spans 210 Hz–4 kHz with its peak at 300–500 Hz), with every
mode to 4 kHz (ten on the treble side from 210, 323 and 513 Hz, seven on the
bass) at a glued wooden box's loss (Q 25): a knock rings for about a quarter
of a second. A part drives each mode by where along the panel it lands, the
keys from the low end to the high, the buttons by their column. The panel's
sizes and constants are estimates from typical instruments. Each landing
moves the modes by up to 4 % (at Life 0.5, scaling with it, none at Life 0),
for the force reaches the panel through key, guide pin, felt and the player's
hold, never twice alike; nothing measures that spread, which the owner asked
for so that no two knocks sound like one sample. Each panel carries the energy
the key's own radiation would (a compact body of the key's size, 20 mm by
100 mm, driven by the force as a dipole averaged over directions): that
estimate sets the level of the whole mechanism. The bass side's knocks leave
through the bass box, as its reeds do. At 1 m, with **Mechanics** at 0 dB, a
soft key peaks at 36 dB SPL, a moderate one at 39, a hard one at 57 and a
pallet at 33, where ordinary playing reads 73 dB RMS:
heard in quiet passages and between notes, as real mechanisms are. By ear
the mechanism was chosen 10 dB above that estimate; the owner later heard
that too loud in songs and set it to 60 % of that level, so Mechanics
defaults to +5.6 dB in Init and every factory preset.

The bellows' folds sound too, and their levels are voiced by ear, not
derived. Each fold's walls are card panels hinged in cloth and leather; the
pressure bows them out on a push and draws them in on a pull, so when the
bellows turn each of the 144 panels of a full-size bellows (18 folds, 35 mm
deep, on walls 410 and 190 mm long, about 1 kg/m²) crosses its hinges' play
and lands the other way, one after another as their masses and play differ:
a rustle at every turn, harder the faster the pressure turns, and silent
between turns. The first stroke after a load or a reset flips none: its
pressure finds every panel on the hinges it pushes them to, as a player's
hold leaves them. The panels radiate as the keys do, strips of their own size.
How hard they land rests on the play, which nothing measures: from none to
1 mm a turn goes from silent to as loud as the chord, so the play is taken
as 0.5 mm and the folds' level set by ear. While the bellows move, their
leather and cloth rub: noise growing with the speed of the bellows end (the
air leaving the bellows over their 0.41 × 0.19 m section, so a hand pressing
on closed keys moves nothing and rubs nothing), between 210 Hz and 2 kHz,
its level and band voiced; below 1 mm/s (an estimate) the folds stick, so
the bellows' own slow leak rubs nothing. Its white force noise scales with
the square root of sample rate relative to 48 kHz, preserving power per Hz
through the fixed-frequency filters. **Mechanics** sets both with the keys'. The
bass strap only taps: drawn up as the bellows turn from push to pull, its
slack meets the wrist at the bellows end's speed (litres × hand speed over
the section above, a few mm/s to a few cm/s), a bass pallet's knock at that
speed. A bass half of some kilograms on leather rings near 10 Hz, far under
the body's 210 Hz, so no ring is modelled.

The push reeds' valves slap. Each reed has a leather or plastic flap over
its slot on the far side of the plate, so it passes air one way; the push
reed's sits in the chamber, the pull reed's in the bellows. Repairers hear
the valves slap shut as the bellows turn and as a note ends, loudest on the
low reeds. Here each open chamber's push valve (0.4 kg/m², covering the
slot with 3 mm to spare, so 0.08 g at C4 and 0.2 g on the longest bass
reeds), lifted 1.5 mm by the push air, lands on the plate on the pallet's
leather law: as the bellows turn from push to pull, driven shut by the
reversing pressure, at (R t² / 2σ) with t = (6σd / R)^⅓ for a pressure
turning at R (0.74 m/s at 16 kPa/s, slower for a slow turn); as a pushed
key is let go, by its own curl as the chamber fills, at its lift times its
natural frequency (0.24 m/s). Either way it lands its flight time after it
lets go: a quarter of its natural period, 10 ms, as its curl shuts it, and
t, about 5 ms, as the bellows turn. Each valve's lift and curl differ from its
neighbours' by up to ±20 % (fixed per instrument, an estimate like the keys'
masses), so the valves of a chord or a turn land one after another over a
few milliseconds, each at its own speed, as a patter rather than one click.
Landing it stops the chamber air it moves, a
source of its area times its speed, so it radiates through the pallet hole
and the grille, or the bass box, as the reeds do. Turning from pull to push the
pull reeds' valves, on the bellows side of the plate, slap too; their sound
must couple through the slot and the chamber, so each takes 0.35 of the mass
of a push valve over the same slot (an estimate). The flutter the reeds'
pulses shake into an open valve is left out, so their tone stays as it was
fitted. The valves are estimates from typical sizes, so their level is
voiced: 30 dB under that estimate, a push-to-pull turn's slaps about 38 dB
under the chord and a released note's 40 dB under the note (the loudest
50 ms each, Paris Musette), the middle of three levels; **Mechanics** sets
them with the rest.

### The output stage

On Reason's VU meter (the louder channel's RMS over 300 ms, +12 dB) the demo
phrases' notes read −9 at their median, and a chord at the arm's full pressure
would read 23 dB above that. The output stage is the one the newer
Protocodus instruments share (`Source/DSP/OutputStage.h`). Its compressor
follows the louder channel's power over 20 ms and brings it down 5:1 above
−6 VU, over a 6 dB knee from −9 to −3 VU, so it takes hold gently from where
ordinary playing sits and a harder stroke still grows at the top (chosen by
ear); it takes hold at once and lets go over 300 ms, slower than a musette's
beating.
Its peak limiter looks 2 ms ahead (96 samples at 48 kHz, the engine's
latency) and holds what gets past at −1.5 dBFS with no gain step. The hardest
strokes read −1.7 VU at most. Volume sets the level going into the stage.

### Controls, presets and MIDI

| Control | Range | What it does |
| --- | --- | --- |
| Register | 8 switches | the treble ranks that sound |
| Musette | 0–40 cents | tremolo detune at A4 |
| Bellows | 0.5–8 L | air one sweep of the wheel moves |
| Assist | 0–1 | how far the hand's air follows the open reeds |
| Grille | 500 Hz–16 kHz | brightness of the treble side |
| Width | 0–1 | the microphones' XY pair: one microphone at 0, 90° apart at 0.6, 150° at 1 (The microphones) |
| Bass | −24 to +6 dB | bass and chord side level |
| Volume | −36 to +6 dB | level into the output stage |
| Keyboard | split, full | split: C1–B1 bass buttons, C2–B2 chord buttons, C3 up treble |
| Air | off, on | the air valve |
| Momentum | 0–200 ms | how far the arm trails the wheel; 0 follows it exactly |
| Oversampling | 1×, 2×, 4× | the reeds' time step: 1× is four times the host rate; 2× and 4× cost about two and four times the CPU |
| Mechanics | −40 to +20 dB | the keys', pallets', buttons' and switches' knocks, the bellows' folds and the push valves' slaps, against the estimate (The mechanism); +5.6 dB by default, by ear; at −40 dB they are all but silent |
| Auto | off, on | on by default: the hand plays endless bellows, turned by the C0 and C#0 keyswitches, and the wheel softens or swells it (Auto) |
| Room | 0–1 wet | the hall around the microphones: 0 dry, 0.5 % by default, 1 as decorrelated as the solo recordings; sigmoid knob response, actual wet percentage displayed (The microphones) |

`Source/DSP/RespiraControls.h` holds these for every front end;
`Source/DSP/RespiraPresets.h` holds the eight factory presets, one per
register, each setting its Volume so it sits within 1 dB of Init on one
reference stroke (a bass, a chord and a C major triad on a moderate 2 s pull
and push), with its peak below −3 dBFS.

MIDI channel 1 is the keyboard (split as above, or all treble), channel 2 the
bass buttons and channel 3 the chord buttons by pitch class; other channels
play as channel 1. With Auto on, notes 12 and 13 (C0 and C#0) on channel 1 latch the bellows to pull
and push. CC1 + CC33 are the hand (with Auto, how hard it presses), CC85 who moves it
(below 64 the wheel, from 64 Auto; the demo phrases send 0 first), CC64 the air valve, channel
pressure how far the channel's held keys rise, CC120 and CC123 release every
key. Pitch bend retunes every sounding tongue, up to two
semitones each way, by scaling its stiffness; no real reed bends that far, but
a pitch wheel should do what it says.

## Known gaps

- **No plug-in yet.** There is no JUCE VST3, AU, CLAP or standalone build,
  so there is no editor screenshot, no main-build or release workflow, no
  packages and no host validation. The Reason Rack Extension adapter is the
  first front end.
- **Voiced, not measured.** Reed dimensions, chambers, pallets, the bellows
  stiffness, the reeds' loss and the turbulence levels are typical values or
  estimates scaled with pitch, and the full-scale reference is set so the demo
  phrases read −9 VU, not by a measured level. The Grille's default and the
  bass box's absorption are fitted to two sample libraries, but no recording
  of a real accordion is part of the model or its tests.
- **CPU.** A full-register chord over bass and chords (28 chambers) takes
  about a third of one core in the development container at 48 kHz (0.35×
  realtime for the Master Tutti demo, 0.02× for the single clarinet reed),
  and a chord change briefly doubles that while the released chambers close.
  The reed chambers' ducts are about a quarter of it, and the rest is close
  to what the model's arithmetic needs. Rewrites that kept the output bit
  for bit (the cells stepped side by side, the push and pull reeds in two
  lanes, the duct's loops two sections at a time or fused into one) gained
  nothing measurable or ran slower, as did one reciprocal of the gap area per
  reed step and the oversampled steps outside the chamber loop (within the
  container's ±5 % timing noise). Treble sections of 7.5 or 10 mm would save
  about 10–15 % but take 3–21 dB off 8–16 kHz, a cut made by the grid, not
  the physics.
- **The mechanism's level and brightness.** No measurement of how loud an
  accordion's mechanism is against its reeds was found, so Mechanics is in dB
  against an estimate: the key's own radiation, while the force really reaches
  the air through the keyboard and the body, which are not modelled. The felt
  laws are a piano's, and the masses and travels estimates. Against the one
  measured spectrum (Niewczas 2019, Fig. 50, about 10 dB under its peak at 3–4
  kHz) the treble knocks are 14–20 dB darker there and the bass side's about 4
  dB darker again through the bass box: the panels ring, but no metal part or
  spring does, and the panels' sizes, plywood and loss are estimates. Pallets and keys stop on felt and never rattle against each other, and
  the bellows' folds' flip at a turn and their rustle while moving, and the
  push valves' slaps, are voiced by ear: no recording or measurement sets their level, and the
  folds' play, which decides how hard they land, is an estimate. The player
  makes no sound.
- **The room is voiced.** Init and the factory presets play it at 0.5 %, the
  owner's chosen default. Its first reflections come from an estimated hall's
  geometry, not a measured one, and use the pair's current angle with
  three-dimensional arrival directions. Its decay is Sabine's for that
  estimated hall and the owner's hundred people, with people's absorption
  from one study of an occupied hall, fitted by a one-pole to within 21 %
  per octave (its bass short); its darkness above 2-3 kHz is voiced; its
  level at Room 1 retains the earlier voiced scale, while the microphone
  readout now follows an ideal diffuse cardioid field. The recordings do
  not establish one room level, decay, or microphone configuration.
- **Chord buttons are pitch classes**, not Stradella's fixed major, minor,
  seventh and diminished rows, and there are no bass register switches.
- **One turbulence seed.** Two engines given the same input play identical
  noise.
- **The bass box's absorption is fitted.** What the box loses on each round
  trip of its depth modes, 4 % of the energy, is fitted to two sample
  libraries, which fit 4–6 % about equally; bare wood absorbs 1–3 % at
  2–4 kHz, and the felt and leather inside are not modelled. Its depth is
  averaged, so no single mode of a real box's shape rings.
- **The top octaves.** The one published harmonic measurement found
  (Elejalde-García, Macho-Stadler and Llanos-Vázquez 2021, a concert
  accordion's 8' reeds from A#2 to A3 played mezzo forte, about 70 dBA at
  50 cm) reaches only the 20th harmonic, 4.4 kHz, and the reed chamber's duct
  matches it there. Every recording is darker above 2 kHz. With the Grille
  at 9 kHz, as it was, Respira's held notes read 6, 9 and 15 dB more than
  two accordion sample libraries' (La Melodiosa and a Hohner) at 2–4, 4–8
  and 8–16 kHz against 1–2 kHz, and 6, 7 and 15 dB more than a bandoneon's;
  against 500 Hz–1 kHz the demos read 3–14 dB more than
  a polka recording at 2–4 kHz and 11–20 dB more above 4 kHz (Bassoon Low,
  through the bass box, 4 dB), and more again than a Hussong recording.
  None of their microphones, distances or rooms is known, but they agree.
  The Grille's default now takes that difference off at the output
  (Radiation): a voiced low-pass, not a mechanism, so the reeds and chambers
  themselves are as bright as before. Through it, A#2–A3's 15th–20th
  harmonics sit 6 dB further under their 5th–8th than the measurement's;
  La Melodiosa's same notes sit 7 dB further under. The chambers' ringing is not the cause: radiation at the pallet
  hole took their resonances from Q 70–370 to 26–48, and the demos above
  4 kHz by only 0.6–2.4 dB. The duct leaves out its wall losses (they would
  take another 2–4 dB off its resonances' peaks), the chamber's
  irregular shape and the tongue inside it, and puts both tongues' tips at
  its closed end: the measurement favours that (at the pallet hole the
  15th–20th fell 16 dB under it), but no drawing of a reed block was found to
  confirm it (`Docs/decisions.md`). On the reed's side the duct's standing
  waves reach the gap's flow, which the tuning jig of Pomberger et al. (DAGA
  2021) hears straight: at 208 Pa the reed flow's 11th–15th harmonics sit
  4 dB (E3), 7 dB (G4) and 14 dB (E6) above the measured reed blocks', and
  E6's 6th–10th 11 dB, where before the duct they sat 6, 2 and 6 dB
  above; at 800 Pa 10, 8 and 27 dB. Real chambers' resonances barely reach
  their reeds below the top octave (Richter 1985, cited there), so the
  duct's may be too strong, which would brighten the pallet hole too; but
  weakening them would take the A#2–A3 measurement's 15th–20th away again.
  Below the 15th the agreement
  rests on the reed plates' thickness, an estimate chosen by ear
  (`Docs/decisions.md`): with plates two thirds as thick, each A#2–A3 tongue
  swung 0.7–2.1 mm out past the far face of its plate at that loudness, where
  the gap opens again, so every period carried a second pulse of air and the
  4th harmonic fell 12 dB under the measurement. Now the 4th sits on it and
  the 2nd–16th agree within 2.2 dB RMS (4.2 before). A real accordion tongue
  does not pass below its frame (Cottingham, CCRMA lecture notes, 2013), and
  one filmed in its plate spends more of each period inside the slot as the
  flow grows (Nussbaumer and Agarwal, ICA 2016). Respira's still pass their
  plates from about 500 Pa: they swing about a tenth of their length, where a
  free-reed tongue commonly swings 15 % (Cottingham, Physics Today, 2011).
  The one plate measured is thicker: 3.1 mm under a 27 mm B4 tongue
  (Babanis, NTUA diploma thesis, 2014, by caliper), where Respira's is
  2.4 mm. Scaled to it, A#2–A3's 2nd harmonic falls 12 dB under the
  measurement (8 dB now) and a half-pressed key falls silent, so one reed is
  not enough to re-pin them. The measured accordion dips at
  its 4th harmonic too, about 7 dB under its neighbours.
- **The treble breathes more above 4 kHz.** Between the harmonics of held
  G4 and A5 notes on Tango Bandoneon, against the harmonics at 1–2 kHz, the
  floor sits 6–10 dB above the bandoneon samples' at 4–8 kHz and
  10–17 dB above at 8–16 kHz. The gap's breath is all of it: with the
  breath off, the floor there sits 2–15 dB under the samples'. The breath is
  flat up to its eddies' rate, about 18 kHz for an A5 tongue at 500 Pa, and
  the Grille tilts it 6 dB per octave above its corner, where the samples'
  floor falls 4–11 dB per octave from 2 kHz. Measured as the floor over the
  harmonics in each octave band from 250 Hz, which no filter after the reeds
  changes, A5 lies
  1–9 dB above the samples from 0.5 to 16 kHz, G4 up to 12 dB above at
  4–16 kHz and D5 up to 7 dB above, while G2 to C4 lie within 7 dB of them
  from 2 to 16 kHz and up to 14 dB under them below 1 kHz
  (`Tools/compare_bandoneon_tone.py`). Two accordion libraries disagree:
  Clarinet Solo's floor over its harmonics sits 3–6 dB under La Melodiosa's
  single reeds' from 0.5 to 16 kHz, and 5–7 dB over the Hohner's at
  4–16 kHz, the median over their notes (`Tools/compare_accordion_tone.py`).
  Its level was chosen by ear, the floor of a sample library may have been
  cleaned, and the three recordings span more than the gap, so no other
  spectrum is derived from them.
- **How far a loud note's pitch falls rests on an unknown pressure.** Over
  the loudest 30 dB of a bandoneon's held notes its pitch rises 0.39 cents
  per dB softer on average, 0.34–0.62 from C2 to D3; Respira's 8' cells, with
  the loudest at 1.5 kPa, rise 0.46, and 0.30–0.43 from C2 to D3
  (`CompareBandoneon`). Over the loudest 15 dB they fall 0.88 cents per dB
  against the bandoneon's 0.47. Which pressure the bandoneon's loudest notes
  were played at is not known: with the loudest at 0.7 kPa the two agree
  within 0.14 over the loudest 30 dB and 0.03 over the loudest 15. Okada's C4 reed (Adachi et al. 2026) falls 2.5 Hz per
  kPa with its pallet open against Respira's 2.3 (from 0.4 to 1.4 kPa,
  `ProbeReed`), and bends 0.24 mm per kPa at the tip against Respira's 0.32;
  both are set 0.3 mm above the plate.
- **A loud treble is not brighter.** Over the loudest 30 dB of the
  bandoneon's held notes from E4 to C6, its 9th–15th harmonics gain 1.4 dB
  per dB on the fundamental on average (0.9–2.1), so a forte note is much
  brighter than a piano one; at the A5 even the 3rd–7th gain 1.2–1.7.
  Respira's 8' cells, the loudest at 1.5 kPa, gain 0.3 there (−0.2 to 0.8,
  `CompareBandoneon`); from C2 to C4 the two agree (0.7 both), and over the
  loudest 15 dB the gap is the same. With the loudest at 0.7 kPa, where the
  pitch slopes agree (above), they gain 0.8 (−0.4 to 2.3) and C2–C4 0.8: the
  gap halves, but the A5's 3rd–7th still gain only 0.5. None of the
  tongue's air drag (cut until tips up to F5 swing 15 % of their length at
  600 Pa, a typical figure in playing), the
  slot's depth, the tongue's set or where its bend sits, its clearance, the
  slot air's inertia, the volume the tongue sweeps, or a second bending mode
  moves it. A laser vibrometer sees a harmonium reed's own motion gain
  harmonics as it is blown harder (Puranik and Scavone, Forum Acusticum
  2023), but a second mode driven by the same pressure stays far too small
  here to show. Only a much lossier tongue comes close: four times the loss
  (a Q near 75 from F5 to C6, where the model has 290–320) makes them brighten
  as the bandoneon's do, though E4–D5 stay flat and the top tips then swing
  3–5 % of their length. Four times the air drag does the same, but a flat
  plate's measured drag in oscillating flow (Keulegan and Carpenter 1958, as
  Graham 1980 fitted it) puts the tongue's coefficient at 5–10 over these
  swings, around the model's 8, not 32. And impulse tests on three accordion
  reeds in their plates (236–743 Hz) measured a Q of 200–400 (Nussbaumer and
  Agarwal, ICA 2016), where the model has 207–293, so a lossier tongue is not
  the cause either, and it is not traced.
- **Half-pressed keys bend too little at a steady pressure.** In the same
  paper a player bent A#2–A3 by closing the pallet part way at a steady
  bellows pressure: 16 cents flat on average and 11 dB quieter (the mean
  over the first twenty harmonics). Respira's 8' cells at that loudness, with
  the pallet opened 0.32 as full channel pressure holds it, fall 6.2 cents
  and 13.5 dB (`CompareHarmonics`). The player found no bend possible from C6
  up, on tongues with no valves; Respira's all have valves, and a single cell
  from E5 to G6 bends 12–23 cents at 200 Pa and 22–28 at 400.
  At a steady wheel, Assist counts a held key in full however far channel
  pressure lets it rise (chosen by ear over counting the pallet's opening,
  which let the pressure fall), so half-pressing raises the pressure into the
  bend. Bent from sounding on a steady pull at about 200 Pa, a C3 falls
  25 cents and 26 dB where the player reached 24 cents and 14 dB with the
  finger alone and 29 and 8 with bellows and finger; an A3 15 cents and 11 dB,
  against 13 and 10, and 17 and 5. A low key struck with channel pressure
  already full does not start from A#2 to D3 at that pressure: the set that
  keeps hard pushes speaking leaks enough air at rest to hold the half-open
  chamber below the reed's starting pressure.
- **Reeds start more easily than the one accordion reed measured.** From
  rest at a steady draw, Respira's 8' reeds start at 16–38 Pa from A2 to C6
  (84 Pa at A6, 131 at C7), and a D#5 stops again at 18 Pa. A Hohner Verdi I
  reed at 622 Hz, on a wind chest drawing through an artificial reed
  chamber, started at 83–103 Pa and stopped at 77–90 whatever the chamber's
  Helmholtz frequency from 610 to 1400 Hz (Cottingham, ICA 2016, Fig. 3), and
  a reed organ's 268 Hz reed started at 50–150 Pa with its chamber tuned away
  from it (Cottingham, ICA 2019, Fig. 8). With the flow reversed the same
  plate's other reed needed 0.9–1.6 kPa, which no accordion needs in playing,
  so the rig is not an accordion's cell, and the neck it drew through is not
  given. Half or a quarter of the tongue's Q starts the D#5 at 31 or 50 Pa,
  which the impulse-tested Q of 200–400 (above) does not allow, and a set
  half as high again at 31. So soft playing sounds from a lighter bellows
  than an accordion's may need; the gap is not traced.
- **Soft notes speak late.** At the measured accordion's piano (about
  55 dBA at 50 cm), its finger attacks took 60–140 ms from −50 to −5 dB of
  the fundamental from A2 to B6 (Llanos-Vázquez et al. 2014). Respira's 8'
  cells at the **inferred source** 55 dBA take about 1365/950/344/444 ms at
  A2/A3/A4/A5 with `CompareAttacks`' 3 ms pallet and four-period tracker.
  The free-field source estimate is not a calibration of the complete
  accordion's body, microphone direction or room. A soft staccato note can
  still end before the modeled reed speaks. The separate MIDI-velocity
  delay was fixed in 0.1.6; see the [current investigation](Docs/research/soft-starts-2026-11-01.md).
  Its reeds sit
  2.7 times as high above their plates as Okada's measured C4 (0.8 mm
  against 0.3, above), so that a key held while the bellows is slammed to
  the arm's full pressure still speaks. With every set scaled to Okada's they
  take 410, 155, 125 and 135 ms, and at 70 dBA an A2 165 ms instead of 240,
  but then C3–C5 held through `HardPushSpeaks`' stroke bend into their slots
  before they swing and stay silent, C3–G#3 even when its second stroke is
  a crescendo over 1.6 s; sets 0.8 or 0.6 times today's choke on it too.
  Repairers describe the same trade: a reed set close to its plate starts
  well at low volume but chokes on an accent, one set far needs a harder
  push to start (accordionists.info forum, 2022). Adding the gap's own air
  mass (Adachi et al.) neither starts them nor keeps A#2–A3's 4th harmonic
  (11 dB under the measurement). Underneath both
  is how slowly the low swings grow (Low reeds start late).
- **The top octave needs mezzo forte to start.** With the bellows already at
  530 Pa, an A4 comes within 3 dB of its level about 38 ms after the key, a
  C6 about 36 ms and a C7 about 72 ms. Against the one measurement with key
  times (Llanos-Vázquez et al. 2014, Acta Acustica 100: a concert accordion's
  fundamental from −50 to −5 dB of its peak after a finger attack at mezzo
  forte, 70 dBA at 50 cm), played at that loudness A4–B4 take 52–57 ms
  (measured 60–70), A5–B5 36–44 ms (50) and A6–B6 92–127 ms (60–100); where
  the tongue's tip share stops falling, A5, was fitted to it. But a high reed
  needs more pressure to start at all: in its cell, drawing steadily, 24–32 Pa
  from A2 to A5, 39 at C6, 85 at A6 and 130 at C7, and once started it is
  soon at 55 dBA (A6–B6 at 95–120 Pa), the paper's piano. There its A6–B6
  took 80–130 ms; Respira's take 1.1–1.3 s, growing slowly so near their
  starting pressure, so soft high notes swell in late. How fast a swing
  grows depends on how far above its starting pressure the reed is: an A6's
  grows by a factor e every 0.53 s at 85 Pa, 0.12 s at 100 Pa and 20 ms at
  200 Pa, so it reaches the measured times only from about 175 Pa, where it
  is already 10–13 dB louder than at 100 Pa. Neither the oscillating-plate
  law the tongue's drag coefficient comes from (Graham 1980, in place of its
  constant 8) nor a set half or 1.7 times as high changes how steeply the
  level rises just above the starting pressure. A recording has no key time, so the wait
  before a note is heard is unchecked, but how fast it grows once heard can be
  read from the bandoneon library's staccato notes (one take each; 2 ms RMS,
  from −20 to −3 dB of the take's peak): 28–42 ms from C4 to D5, 48–53 ms at
  A5 and C6. Each of those takes also holds a second note about a semitone
  below, as loud as the first at its loudest and gone sooner, so these are
  the two notes' rise together; in the held takes the slopes above come
  from it lies 44–56 dB down, at their floor. Tango Bandoneon played an
  octave up, so that the same two reeds sound, grows in 12–30 ms with the
  bellows at 730 Pa (18–20 ms from F5 to C6) and 7–11 ms at 1.8 kPa, faster
  than the bandoneon at either, though the pressure its staccato notes were
  played at is not known.
  The reeds above C7, there since 0.1.14 so that every key to C8 has its
  own, need more again: drawing steadily, they come within 20 dB of their
  400 Pa level after 2 s from about 100 Pa at C♯7, 140 at D7 and 265–280 at
  C8, where C7 needs 90 by the same measure (`ReedProbe.h`'s `Radiated`, each
  treble rank), so Touch's softest strokes, about 180 Pa, do not start them
  from G♯7 up. No accordion has reeds there to compare them with.
- **Low reeds start late.** When the bellows reaches the arm's full pressure
  before a low reed swings, the tongue sits near the slot's mouth and its
  swing grows slowly: on Bassoon with the wheel swept in 1.5 s, C3–F#3 reach
  their level 0.45–0.65 s after the key, a C4 in 0.35 s. A quicker or gentler
  stroke starts them at once. A finger attack is slow too: at 70 dBA, A2–B2
  take 206–240 ms from −50 to −5 dB and A3–B3 90–102 ms, where
  Llanos-Vázquez et al. (2014) measured 70–100 and 60–110. The bandoneon
  library's left-hand staccato notes (one take each, measured as its right
  hand's above, and holding the same second note a semitone below) grow in
  68–87 ms from E2 to B2 and 40–63 ms from D3 to C4, its C2 in 163 ms;
  Tango Bandoneon an octave up, so the same two reeds sound, at 730 Pa takes
  190–350 ms from C2 to G2 but 26–100 ms from B2 to C4. Respira's swings
  grow by about 1 dB a period from A2 to A5, where the measured attacks take
  about as long at every pitch, and the tip weights set that pace: an A2's
  tip weighs six times the bare tongue's, and without
  the weight, at the same set, it takes 15 ms at 70 dBA and 74 ms at 55 dBA
  (measured 80 and 110), but then 0.4 kPa holds it in its slot. Keeping the
  arm's full pressure out of the slot ties the set to the same lightness:
  with half the weight and the set raised to match, an A2 takes 440 ms at
  70 dBA and a C3 350, against 240 and 190; at today's set it takes 87 ms,
  but then Bassoon's held keys from C3 to D4 stay silent through a hard
  push. Real reeds face the same bind (Soft notes speak late); no
  measurement here says where an accordion's are voiced in it.
- **Low notes rise into pitch.** A reed sounds flatter while its swing is
  small, and Respira's low swings grow slowly, so a low note starts flat and
  rises. Measured as the bandoneon library's held notes are
  (`CompareBandoneon 700`: 100 ms frames; the pitch of the note's own reed
  where the attack first comes within 10 and 3 dB of its loudest, against
  the note's own pitch-against-loudness line), the bandoneon's E2–B2 start
  23–31 cents flat 10 dB down and 9–11 at 3 dB down, and Respira's 8' cells,
  the pull raised to 700 Pa over 0.3–1.5 s, 32–42 and 15–18. From D3 to A3
  the bandoneon is within 11 cents 10 dB down and 2 at 3 dB down, where
  Respira is 22–42 and 1–19, less the slower the pull; its C2 starts flatter
  than Respira's (58 cents against 44–48). Most of the E2–B2 gap 10 dB down
  is the set: sized for 1.5 kPa in place of the arm's full 2.5, the cells
  start 21–35 cents flat there and 7–18 at 3 dB down, but held low keys then
  go silent under a full push (Soft notes speak late). From C4 up the
  bandoneon's single takes scatter by up to 37 cents, so they say little.
  A key pressed while the bellows already draws differs: La Melodiosa's
  single 16' reeds (two takes a note, the same frames against each take's
  pitch from 1 s on) are still 7–22 cents flat on D#2–C3 when their level
  first comes within 1 dB of where it stays, and take 0.1–0.8 s more to come
  within 3 cents; from D#3 up, and on its bass reeds D3–B3, at most 0.12 s.
  Respira's 8' cells, the pallet opened under a steady 0.3–1.2 kPa pull, are
  2–7 cents flat there and within 3 cents at most 0.1 s later: their pitch
  settles with their swing, the recordings' after their level. The
  oscillating-plate drag law (The top octave needs mezzo forte to start)
  lengthens that by at most 0.12 s, and nothing found here says what holds
  a real low reed flat at full level. Neither measure depends on where the
  library trimmed its takes' starts.
- **The demos' bellows is fitted, not played.** Each stroke's travel comes
  from a target pressure through `Tools/fit_bellows.py`; a player's hand would
  be less even.

## Build

The engine, tools and tests need CMake 3.22 and a C++20 compiler (the engine
itself is C++17); the Python checks need Python 3.

```bash
cmake -S . -B build-dsp -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build-dsp --parallel && ctest --test-dir build-dsp --output-on-failure
./build-dsp/RenderRespiraPresetDemos Docs/audio/midi build-dsp/demos   # WAVs + demos.json
```

Every test carries one tier label, so `ctest -L unit`, `-L audio`, `-L fuzz`
or `-L perf` selects a tier. `RespiraDSPCxx17` compiles the engine as C++17
with `-fno-builtin`, as the Rack Extension toolchain does.
`-DRESPIRA_ENABLE_SANITIZERS=ON` builds everything under ASan and UBSan, and
`-DRESPIRA_BUILD_FUZZER=ON` (Clang) adds `RespiraDspFuzz` as a libFuzzer
target.

The offline tools: `RenderRespira <preset> <in.mid> <out.wav> [trace.csv]`
renders any MIDI file with a trace of the hand and the bellows;
`ProbeReed [pressure] [rank]` prints what single reeds sound and the air they
take; `CompareAttacks [--note A2|A3|A4|A5] [--target 55|70]` reports bare-cell
finger-attack rise times, first/last threshold crossings and 2/4/8-cycle
window sensitivity against Llanos-Vázquez et al. (2014), with the source-SPL
and zero-turbulence limitations printed in its output; `CompareAttacks
--self-test` checks its estimator; `CompareHarmonics [pressure]` sets the first twenty harmonics of single
8' notes, and how far a half-open pallet bends them, against a published
accordion measurement; `CompareBandoneon [top pressure]` sets how their pitch
and tone follow their loudness, and how flat their attacks start, against a
bandoneon's, whose numbers
`python3 Tools/measure_bandoneon.py <library folder>` measures from the
recordings (not in this repository); `python3 -I
Tools/compare_accordion_tone.py --melodiosa <dir> --hohner <dir> --render
build-dsp/RenderRespira` sets held notes' octave bands at each Grille
setting against two accordion libraries (not in this repository either),
and compares the breath between their harmonics,
`python3 -I Tools/measure_musette.py --hohner <dir>` measures one's
musette beats, and `python3 -I Tools/measure_tuning.py --melodiosa <dir>`
how far the other's reeds sit off their pitch; `python3 -I
Tools/compare_bandoneon_tone.py --bandoneon <dir> --render
build-dsp/RenderRespira` compares Tango Bandoneon's tone and breath with the
bandoneon's;
`GenerateReedTuning Source/DSP/ReedTuning.h` re-tunes every reed after a
change to the reed model. `python3 Tools/fit_bellows.py
build-dsp/RenderRespira` re-fits the demo strokes and
`python3 Tools/write_preset_midi.py` writes the demo MIDI.

`ci.yml` runs on every push: the unit, audio and perf tiers on Linux, the unit
tier on Windows and macOS, and the unit and fuzz tiers under ASan and UBSan
with the C++17 engine build.

## Licensing

Respira's source is available under the [PolyForm Noncommercial License
1.0.0](LICENSE), with an additional permission to use official builds for any
purpose, commercial music included. It contains no third-party code or data
([THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)). The licensed Reason
Jukebox SDK is not part of this repository.
