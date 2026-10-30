# Respira

Respira is a physically modelled accordion played with the bellows: the mod
wheel is the player's hand on the bellows, opening draws air through the pull
reeds and closing presses it through the push reeds, and only the hand's
motion moves air, so the wheel has to keep rocking back and forth. Every
sounding reed is a simulated steel tongue in its own chamber, with eight
treble registers, bass and chord buttons and a dry stereo output. This
repository holds the JUCE-free C++17 engine, its tools and tests; the Reason
Rack Extension adapter plays the same engine. There is no DAW plug-in yet.

## Audio demos

One phrase per factory preset, each written for what its register is played
for and rendered from `Docs/audio/midi/` through the shipping engine. The
bellows in each is a part of the MIDI: the mod wheel rocks through strokes
fitted to a target pressure each (`Tools/fit_bellows.py`).

<!-- audio-demos-begin -->

| # | Preset | Demo | Length | Peak |
| --- | --- | --- | --- | --- |
| 1 | Clarinet Solo | [01-clarinet-solo.wav](Docs/audio/01-clarinet-solo.wav) | 11.2 s | -3.8 dBFS |
| 2 | Bassoon Low | [02-bassoon-low.wav](Docs/audio/02-bassoon-low.wav) | 9.6 s | -3.9 dBFS |
| 3 | Violin Folk | [03-violin-folk.wav](Docs/audio/03-violin-folk.wav) | 10.4 s | -6.1 dBFS |
| 4 | Paris Musette | [04-paris-musette.wav](Docs/audio/04-paris-musette.wav) | 10.7 s | -5.7 dBFS |
| 5 | Tango Bandoneon | [05-tango-bandoneon.wav](Docs/audio/05-tango-bandoneon.wav) | 11.2 s | -1.5 dBFS |
| 6 | Organ Bellows | [06-organ-bellows.wav](Docs/audio/06-organ-bellows.wav) | 10.6 s | -4.5 dBFS |
| 7 | Bright Oboe | [07-bright-oboe.wav](Docs/audio/07-bright-oboe.wav) | 9.8 s | -6.0 dBFS |
| 8 | Master Tutti | [08-master-tutti.wav](Docs/audio/08-master-tutti.wav) | 8.7 s | -4.9 dBFS |

<!-- audio-demos-end -->

## Version history

### Unreleased — 0.1.0 — the dry engine

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
- **Room**, off by default, adds a room around the microphones: a 1 s
  diffuse field each microphone hears on its own, so at Room 1 the two
  channels correlate as little as the five solo recordings' do. Init stays
  dry; the eight factory presets play at Room 70 %, where the channels
  correlate 0.49 on Paris Musette, at the dry end of the recordings.

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

With **Auto** on, a player's hand moves the bellows by itself while any key
is held, and the wheel no longer moves them. The hand pulls through four
bars and pushes through the next four, as accordionists are taught to change
the bellows on a full-size instrument, so a pull and a push take 32 beats in
4/4 and 24 in 3/4. At each bar line the player turns early if the bellows
will not last the next bar at the air the reeds take, as players change at a
bar line when the air runs short: at 120 in 4/4 a melody line turns every
four bars, while a bass, chord and triad with Assist at 0 (some 3 L/s) turns
every bar. While the host plays, the strokes turn on its bar lines, and the
four-bar strokes count from the song's first bar wherever playback starts.
With the transport stopped they run at the host's tempo from the first note,
and a phrase that starts after a rest of a stroke or more starts on a pull,
as a player starts with the bellows closed.

The hand does not move at a fixed speed: like a player it feels the air the
reeds take and gives it, and closes on the pressure it wants in a reaction
time of 0.1 s (four times Momentum when that is longer). It feels its own
arm too, so it closes on the pressure the arm will bring once it catches up,
and the arm's trail of twice Momentum is part of that time rather than a lag
inside the loop. It wants 400 Pa, the pressure the demo phrases' ordinary
strokes are fitted to and about −9 VU, whatever is played and whatever
Bellows and Assist are set to; the wheel raises that up to four times, 1.6
kPa at the top, so it is an expression control from its resting place at 0.
A turn is planned rather than reacted to: expecting the reeds to take the
air they took, the other way, the hand moves as fast as it goes (4 L/s of
its own air, more with Assist) until the pressure its arm will bring is the
new one, so a held chord falls more than 10 dB below its level for only 5–15
ms. The hand turns early by twice Momentum plus the time that swing to zero
takes it, so the pressure passes zero 5–25 ms before the bar line at the
default Momentum and the new stroke speaks on the downbeat. Each stroke
starts from rest; let go of every key and the hand stops, the closed pallets
keeping the bellows at their pressure, so the next note speaks at once. A
first stroke starts far enough in from the end of the bellows that the
strokes after it fit; the bellows Auto moves hold 6 L, an estimate for a
full-size accordion (the Bellows control is only the wheel's gearing), and
if a stroke still runs them out the hand turns there and finds the bars
again at the next turn. Switched off, Auto leaves the bellows where they
are: the wheel takes over without a stroke. Pressure, reaction time and
capacity are voiced, not measured.

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
plate that the arm's full pressure, held still, leaves it at least its
clearance short of the slot, so a bellows pushed hard before the reed swings
cannot blow it shut and silent. Short treble tongues are filed thin towards
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
within 2.3 dB on average at its loudness, though the 2nd sits 8 dB under, and
the 15th–20th average −13.9 dB against the measured −13.4. The 2nd is low
because, while the tongue seals its slot, the air it sweeps through arrives
almost opposite the gap's own 2nd harmonic: at A3 the gap flow's 2nd is 7 dB
under its fundamental, their sum's 22 dB. A lab measurement of real
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
single 8' cells go 0.51 cents flat per dB louder on average against the
bandoneon's 0.39, and their 3rd, 5th and 7th harmonics gain 0.40–0.61 dB per
dB on the fundamental against its 0.64–0.80. Over the loudest 15 dB they fall
twice as far, and from E4 up their 9th–15th harmonics hardly grow at all
(Known gaps).

The cells run at four times the host rate (192 kHz at 48 kHz) and are
decimated by two half-band allpass stages; Oversampling at 2× or 4× runs them
at eight or sixteen times the host rate through three or four stages, with
the gap turbulence scaled so its spectrum stays the same at every host rate
and Oversampling. The pallet lifts in 12 ms for a soft key and 3 ms for a hard
one, along a raised cosine, so velocity sets the attack, and closes in 8 ms;
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
breaking back an octave at the top so that no reed sounds above C♯7; where M
is open it stays shut there, since it would sound in unison with M a few
cents apart and the two would beat slowly to near silence). The
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

The output is what a coincident pair of cardioid microphones (XY) hears from
1 m in front of the player, in the player's view, the treble on the right.
The treble keyboard stands upright, so its pallet holes and the grille over
them are one place across the pair, 0.20 m right of the bellows' middle; the
bass end cover is the other end, 0.25 m left. A coincident pair hears them
by level alone: each side reaches each microphone as 1 + cos of its angle
off that microphone's axis, at the side's own power. **Width** turns the
axes apart, from one microphone (0) through the usual XY pair's 90° (0.6,
the default: the treble 1.4 dB to the right, the bass side 1.8 dB to the
left) to 150° (1: 2.6 and 3.3 dB). The folds sit between the two, in the
middle. Five solo accordion recordings were made this way: their channels
align at zero lag, and in every band from 60 Hz to 12 kHz they differ by
0.1–2.2 dB; the demos with bass now differ by up to 2.1 dB, where panning
each key by pitch put Paris Musette's bass band 10 dB to the left. The
positions are a full-size instrument's, estimated.

**Room**, off on Init and at 70 % on every factory preset, puts a room
around the pair: the late, diffuse
field of a voiced room with a 1 s decay at low frequencies and half that at
the top, from an eight-line feedback delay network (`Source/DSP/Room.h`)
behind four allpass diffusers, so it is dense from its first milliseconds.
Each microphone hears four of the lines, so the field reaches the two
decorrelated, as a room's does, while the instrument itself reaches both at
once. In the five solo recordings the two channels' phase-transform
cross-correlation of 170 ms frames peaks at only 0.20–0.51 at zero lag
(median 0.38); the dry demos read 0.97–0.99, and at Room 1 the eight demos
read 0.34–0.42. The decay was chosen by ear from three (0.5, 1 and 2 s); the
recordings' own rooms are not measured, since their notes rarely stop clean
enough to read a decay.

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
from its neighbours' (its mass ±20 %, fixed per instrument). Masses and
travels are estimates from typical sizes.

The felt stiffens as it is compressed, so a harder stroke lands not only
louder but shorter and brighter: a soft key's knock lasts about 3.4 ms and
peaks near 280 Hz, a hard key's 2.6 ms, near 550 Hz. The force radiates as the
key does, a compact body of the key's size (20 mm by 100 mm) driven by it, as
a dipole averaged over directions, and every part radiates through that one
coupling; the body passes it only above its lowest panel modes, a 210 Hz
high-pass, where a real accordion's mechanism sound begins (Niewczas, Łódź
2019, Fig. 50, which spans 210 Hz–4 kHz with its peak at 300–500 Hz). The bass
side's knocks leave through the bass box, as its reeds do. At 1 m, with
**Mechanics** at 0 dB, a soft key peaks at 42 dB SPL, a moderate one at 46,
a hard one at 64 and a pallet at 41, where ordinary playing reads 73 dB RMS:
heard in quiet passages and between notes, as real mechanisms are. By ear
the mechanism was chosen 10 dB above that estimate, so Mechanics defaults to
+10 dB in Init and every factory preset.

The bellows' folds sound too, and their levels are voiced by ear, not
derived. Each fold's walls are card panels hinged in cloth and leather; the
pressure bows them out on a push and draws them in on a pull, so when the
bellows turn each of the 144 panels of a full-size bellows (18 folds, 35 mm
deep, on walls 410 and 190 mm long, about 1 kg/m²) crosses its hinges' play
and lands the other way, one after another as their masses and play differ:
a rustle at every turn, harder the faster the pressure turns, and silent
between turns. The panels radiate as the keys do, strips of their own size.
How hard they land rests on the play, which nothing measures: from none to
1 mm a turn goes from silent to as loud as the chord, so the play is taken
as 0.5 mm and the folds' level set by ear. While the bellows move, their
leather and cloth rub: noise growing with the speed of the bellows end (the
air leaving the bellows over their 0.41 × 0.19 m section, so a hand pressing
on closed keys moves nothing and rubs nothing), between 210 Hz and 2 kHz,
its level and band voiced; below 1 mm/s (an estimate) the folds stick, so
the bellows' own slow leak rubs nothing. **Mechanics** sets both with the keys'. The
bass strap is silent: taken up as the bellows turn, its slack meets the hand
at a few mm/s to a few cm/s, and a bass half of some kilograms on leather
rings near 10 Hz, far under the body's 210 Hz.

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
natural frequency (0.24 m/s). Landing it stops the chamber air it moves, a
source of its area times its speed, so it radiates through the pallet hole
and the grille, or the bass box, as the reeds do. Turning from pull to push
the slaps are in the bellows and are not heard. The flutter the reeds'
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
| Mechanics | −40 to +20 dB | the keys', pallets', buttons' and switches' knocks, the bellows' folds and the push valves' slaps, against the estimate (The mechanism); +10 dB by default, chosen by ear; at −40 dB they are all but silent |
| Auto | off, on | the hand plays the bellows in time with the host and the wheel presses harder (Auto) |
| Room | 0–1 | the room around the microphones: 0 dry (the default), 1 as decorrelated as the solo recordings (The microphones) |

`Source/DSP/RespiraControls.h` holds these for every front end;
`Source/DSP/RespiraPresets.h` holds the eight factory presets, one per
register, each setting its Volume so it sits within 1 dB of Init on one
reference stroke (a bass, a chord and a C major triad on a moderate 2 s pull
and push), with its peak below −3 dBFS.

MIDI channel 1 is the keyboard (split as above, or all treble), channel 2 the
bass buttons and channel 3 the chord buttons by pitch class; other channels
play as channel 1. CC1 + CC33 are the hand (with Auto, how hard it presses), CC64 the air valve, channel
pressure how far the channel's held keys rise, CC120 and CC123 release every
key. Pitch bend retunes every sounding tongue, up to two
semitones each way, by scaling its stiffness; no real reed bends that far, but
a pitch wheel should do what it says. A front end gives Auto the host's
tempo, time signature, song position and bar start before each block
(`Engine::SetTransport`); without them Auto plays at 120 in 4/4.

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
  kHz) the treble knocks are 13–19 dB darker there and the bass side's about 4
  dB darker again through the bass box: no metal part, spring or panel rings
  here. Pallets and keys stop on felt and never rattle against each other, and
  the bellows' folds' flip at a turn and their rustle while moving, and the
  push valves' slaps, are voiced by ear: no recording or measurement sets their level, and the
  folds' play, which decides how hard they land, is an estimate. The player
  makes no sound.
- **The room is voiced.** Init is dry; the factory presets play in the room
  at 70 %, for the owner's "great and expensive".
  Room adds a late diffuse field only, with no early reflections from a
  floor or walls; its decay and its treble loss are chosen by ear, and its
  level at Room 1 is set by one measure, the channels' correlation against
  the solo recordings' (The microphones), on one demo.
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
  the loudest at 1.5 kPa, rise 0.51, and 0.33–0.52 from C2 to D3
  (`CompareBandoneon`). Over the loudest 15 dB they fall 0.88 cents per dB
  against the bandoneon's 0.47. Which pressure the bandoneon's loudest notes
  were played at is not known: with the loudest at 0.7 kPa the two agree
  within 0.04 on both. Okada's C4 reed (Adachi et al. 2026) falls 2.5 Hz per
  kPa with its pallet open against Respira's 2.3; Respira's C4 bends 0.32 mm
  per kPa at the tip against its 0.24, and is set 0.8 mm above its plate
  against its 0.3, because a lower set lets a bellows slammed to full
  pressure hold the tongue in its slot.
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
  the pallet opened 0.32 as full channel pressure holds it, fall 9.1 cents
  and 12.5 dB (`CompareHarmonics`). The player found no bend possible from C6
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
  cells at that loudness take about 960 ms at A2 and A3, 385 at A4 and 425 at
  A5, so a soft staccato note can end before it has spoken. Its reeds sit
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
take; `CompareHarmonics [pressure]` sets the first twenty harmonics of single
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
