# Respira 0.1.0: engine development history

What the engine gained from its first commit in this repository
(2026-10-27) to 0.1.1 (2026-11-01), moved here from the README's Version
history on 2026-11-04. Later versions supersede some of it: Auto's
four-bar strokes are gone since 0.1.13, Room's default is 0.5 % since 0.1.1
and Life's since 0.1.12.

- The accordion engine from the prototype in its own repository: reed cells
  with push and pull tongues, one bellows driven by the mod wheel, eight treble
  registers, bass and chord buttons, and eight factory presets.
- The output is dry: the prototype's room is gone.
- New against the prototype: Assist, so a full chord does not need forty times
  the hand travel of a single reed; a split keyboard (bass and chord buttons
  below C3) for single-channel hosts such as Reason; an air button; a
  reed-tuning table made offline, so a note-on does no simulation; a pitch
  bend of two semitones each way; Momentum, so the hand trails the wheel and
  the bellows reverse smoothly; Oversampling, 1×, 2× or 4× the engine's own
  rate.
- The jet through each reed's gap is turbulent, so a reed breathes a little
  around its tone; the air button is an orifice whose rushing air is heard.
- Channel pressure holds the keys part way down: at full pressure each
  pallet stays open about 1 mm, at the opening where the model's C4 bends
  with the bellows as a measured reed's does, so the note softens and bends
  as a half-pressed key's does. The register switches reach held keys.
- Ordinary playing reads about −9 on Reason's VU meter, and an output
  compressor and peak limiter keep a chord at the arm's full pressure out of
  the red, its peaks under −1 dBFS. The factory presets sit within 1 dB of
  each other on one reference stroke.
- Switching a register changes the colour, not the level: each register is
  trimmed so the eight sit within 1 dB of each other.
- The bass reeds sound through the bass box, a Helmholtz resonator at about
  960 Hz behind the bass end cover's holes, in place of a plain 2.5 kHz
  low-pass: a rounder, less buzzy bass. The air valve opens into it too, so
  its hiss is a darker rush.
- The reeds' breath has the spectrum of the eddies in their gaps, so the
  estimated turbulence is heard rather than three quarters of it falling
  above 24 kHz, at half the estimated level (chosen by ear).
- From D6 up, Oboe and Master no longer double the 8' with the broken-back
  4' in unison, which beat slowly and could almost cancel a held note; the
  top octave holds a steady level.
- The treble is much darker: the Grille's default falls from 9 kHz to 1 kHz,
  fitted to two accordion sample libraries' held notes, which Respira
  out-shone by 6–15 dB above 2 kHz (chosen by ear). The presets' grilles fall
  in proportion and the range now reaches 500 Hz, so the old brightness is
  still a turn away. Ordinary playing still reads about −9 VU; the bass side
  and the keys, which do not pass the grille, sit about 2.5 dB higher against
  the treble.
- A key held while the bellows is pushed hard to the arm's full pressure no
  longer stays silent: many notes from C3 to G#4 could, until the pressure
  eased. Each tongue is now set far enough above its plate to stay short of
  its slot at that pressure.
- A half-pressed key keeps its share of Assist's air, so at a steady wheel the
  pressure rises into the bend, as when the bellows is pushed harder: low notes
  bend instead of fading (chosen by ear).
- The mechanism sounds, with or without air: a key knocks as it lands on its
  felt, harder and brighter for a faster stroke, and its pallet as the spring
  seats it; bass and chord buttons, the register switches and the air button
  knock too. **Mechanics** sets their level, which is estimated, not measured;
  its default, 10 dB above the estimate, was chosen by ear.
- The reed plates are half as thick again, so a low tongue played mezzo
  forte no longer swings out past the far side of its plate: A#2–A3 get back
  the 4th harmonic it nearly cancelled (chosen by ear).
- The output compressor takes hold gently from −9 VU, 5:1 over a 6 dB knee,
  so a harder stroke on the bellows still gets louder at the top (chosen by
  ear); the hardest strokes still stay out of the red.
- Each reed chamber is a duct as long as its reed instead of a single
  resonance, so the tone is brighter above 2 kHz: the low notes' 15th–20th
  harmonics sit on the one published measurement instead of 7 dB under it
  (chosen by ear).
- The low reeds carry lighter tip weights and are set higher above their
  plates, so they speak sooner and bend further, and a half-pressed low note
  at mezzo forte is about 12 dB quieter, as measured, instead of nearly
  silent (chosen by ear).
- The shortest tongues are filed thinner towards the tip, so from A5 up the
  notes speak in 36–127 ms instead of 111–311, against the 50–100 measured
  on a concert accordion (chosen by ear).
- Low notes are in tune once they settle: the tuning was read before a low
  reed's swing had finished growing, so C2–B2 sounded up to 3.5 cents sharp
  at the tuning pressure, the bass buttons up to 2.7, and the 16' rank's
  C1–B1 (Keyboard on every key) up to 17.
- Each pallet hole radiates into the room as a hole in a board does, its air
  meeting the room's resistance as well as its own mass: the reed chambers'
  resonances ring at Q 26–48 instead of 70–370, and the demos are 0.6–1.5 dB
  softer at 4–8 kHz and up to 2.4 dB above (chosen by ear).
- A key let go no longer clicks: as a pallet's pad lands, the air leaving
  through the thin film under its rim slows to rest instead of being cut off
  when the pallet shuts (chosen by ear). A half-pressed key bends a little
  further (9.1 cents instead of 8.6 at 70 dBA).
- Each reed breathes through eddies the size of its own gap, widest near the
  tongue's tip, instead of one 0.3 mm size for every reed: from G2 to C4 the
  breath between the harmonics is 6–12 dB stronger below 1 kHz and 3–6 dB
  weaker above 8 kHz, and D5 and A5 breathe 3–8 dB more from 0.5 to 8 kHz.
  Against a bandoneon's held notes the breath is 7 dB RMS out across seven
  notes and six octave bands, where it was 12 (chosen by ear).
- The bass side is brighter: above 2 kHz the bass box lets out its own depth
  modes, as a layer of air under the cover does, instead of falling 12 dB an
  octave as one lumped resonator, and the bass reeds' chambers are simulated
  as finely as the treble's. Against two sample libraries' bass buttons the
  bass side read 11–44 dB too dark from 2 to 16 kHz; now it sits within
  2.3 dB of both but for the Hohner's top octave. The demos with bass gain
  1–16 dB from 4 to 16 kHz; the air button's hiss and the bass side's knocks
  brighten with it.
- The reeds are tuned as a played accordion's are, each a few cents off its
  pitch (3 cents standard deviation, where it was under half a cent), as both
  sample libraries' reeds sit: the octave ranks beat slowly against each
  other, the musette beats a little faster or slower from note to note, and
  a note's push and pull reeds differ by a few cents.
- **Auto** plays the bellows for you: while a key is held the hand pulls
  for four bars and pushes for the next four (turning sooner, on a bar line,
  when the air runs short), in time with the host, holding an ordinary
  playing pressure whatever is played; the wheel presses harder.
- Changing register knocks twice: the switch pressed down, and the one it
  pushes off springing back up.
- The bellows sound: their folds flip with a soft rustle as the bellows turn
  and their leather rubs while they move, at levels chosen by ear.
- The musette's beat rises with pitch as a measured Hohner's does (f^0.51,
  where it was f^0.7): at Init's 22 cents a C3 triad beats about a quarter
  faster and a C6 a sixth slower, A4 unchanged.
- Auto turns the bellows as a player plans a turn: the hand goes as fast as
  it can until the pressure its arm will bring is the new one, so a held
  chord falls more than 10 dB for 5–15 ms at a turn, where it fell for
  125–190 ms and twice as deep.
- The push reeds' valves slap shut on their plates as the bellows turn from
  push to pull and as a pushed key is let go, harder for a faster turn and
  heavier on the low reeds, at a level voiced very low.
- The stereo output is a coincident pair of microphones 1 m in front of the
  player, as five solo accordion recordings were made, in place of panning
  each key by pitch: the treble and the bass side are each heard from one
  place, the bass side about 2 dB to the left where it was 8–10 dB, and
  **Width** turns the pair's axes apart.
- **Auto** is on by default, on Init and every factory preset: just play,
  and the hand keeps the bellows supplied. CC85 below 64 hands the bellows
  to the wheel, as the demo phrases do first, so they sound as before.
- The room's late field has sixteen slowly drifting delay lines in place of
  eight fixed ones, so it no longer rings metallic on a held chord.
- Auto's hand is a person's: the pressure it wants wanders a little and
  each stroke takes its own level, so a held chord breathes rather than
  sitting dead level.
- **Room**, off by default, adds a room around the microphones: a living
  room full of people, its 0.5 s diffuse field heard by each microphone on
  its own, so at Room 1 the two channels correlate as little as the five
  solo recordings' do. Init stays dry; the eight factory presets play at
  Room 70 %, where the channels correlate 0.49 on Paris Musette, at the dry
  end of the recordings.
- Auto plays the same at any Momentum: the player leads their own arm, so
  a heavy arm softens the turns but no longer starves the reeds. At Momentum
  200 ms a held triad reached 360 Pa in 4.2 s and held 349; it now takes
  0.5 s and holds 400, and the turn's dip below −10 dB is 110 ms where it
  was 165.
- A reset (the host's audio reset, before a bounce) plays as a new instance
  does, to the bit: a new instance no longer glides its gains, the room's
  above all, up from silence over its first notes, nor knocks the register
  switch of the patch it was loaded with.
- The room is a small hall with a hundred people in it, dark and warm:
  1.0 s at 125 Hz falling to 0.56 s at 4 kHz, its reflections and late
  field softened from 2-3 kHz up. Init and every factory preset play it at
  Room 5 %, a touch of air with the instrument still close, where the
  presets played at 70 % and Init dry. Init plays the Clarinet register.
