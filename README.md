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
| 1 | Clarinet Solo | [01-clarinet-solo.wav](Docs/audio/01-clarinet-solo.wav) | 11.2 s | -7.4 dBFS |
| 2 | Bassoon Low | [02-bassoon-low.wav](Docs/audio/02-bassoon-low.wav) | 9.6 s | -7.1 dBFS |
| 3 | Violin Folk | [03-violin-folk.wav](Docs/audio/03-violin-folk.wav) | 10.5 s | -6.8 dBFS |
| 4 | Paris Musette | [04-paris-musette.wav](Docs/audio/04-paris-musette.wav) | 10.7 s | -6.4 dBFS |
| 5 | Tango Bandoneon | [05-tango-bandoneon.wav](Docs/audio/05-tango-bandoneon.wav) | 11.2 s | -3.7 dBFS |
| 6 | Organ Bellows | [06-organ-bellows.wav](Docs/audio/06-organ-bellows.wav) | 10.6 s | -3.9 dBFS |
| 7 | Bright Oboe | [07-bright-oboe.wav](Docs/audio/07-bright-oboe.wav) | 9.8 s | -6.2 dBFS |
| 8 | Master Tutti | [08-master-tutti.wav](Docs/audio/08-master-tutti.wav) | 8.7 s | -4.2 dBFS |

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
  pallet stays open about 1 mm, so the note softens and bends with the
  bellows as a half-pressed key's does. The register switches reach held
  keys.
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
  above 24 kHz: a little more air around soft notes.

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
what an arm can put on a bellows. A still hand makes no sound; the pressure
falls as the open reeds use up what is left.

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
the flow out of the pallet hole, as from a small source 1 m away, with 1 Pa
(94 dB SPL) at digital full scale.

The jet through the gap is turbulent. Eddies about 0.3 mm across, each
fluctuating by about 10 %, form independently along the gap's edge (twice the
tongue's length plus its width), so the flow fluctuates by 10 % over the
square root of their number: a little breath around the tone, more on a big
low reed's long gap than on a high one's. Its spectrum is the eddies': flat
up to their rate, the jet's speed over 2π times their size (about 15 kHz at
500 Pa), falling above it, as the air valve's does. The same random draw, white,
is the small force that seeds the tongue.

Tongue length and width follow typical accordion reed sizes scaled with
pitch, the thickness is whatever gives the pitch, and a chamber whose
Helmholtz resonance would fall near its reed is made smaller, as reed makers
do with wax in the top octaves. Against the one published harmonic
measurement of an accordion's 8' reeds (A#2–A3, mezzo forte, 50 cm;
`Tools/CompareHarmonics.cpp`), the 2nd–14th harmonics agree within 2.5 dB on
average at its loudness, and the 15th–20th sit 6 dB low (see Known gaps).
Every push and pull tongue differs a little
from its neighbours (set ±10 %, loss ±20 %, tuning ±0.8 cents, fixed per
instrument). A reed sounds below its natural frequency under load, so
`Tools/GenerateReedTuning.cpp` simulates every reed in its chamber at 400 Pa
and writes `Source/DSP/ReedTuning.h`, the natural-to-sounding ratios the
engine builds reeds from at note-on. Pitch still moves with pressure as a real
reed's does: an A4 is within 2 cents at playing pressure (about 550 Pa) and
about 8 cents flat when pressed at 1.4 kPa.

The cells run at four times the host rate (192 kHz at 48 kHz) and are
decimated by two half-band allpass stages; Oversampling at 2× or 4× runs them
at eight or sixteen times the host rate through three or four stages, with
the gap turbulence scaled so its spectrum stays the same at every host rate
and Oversampling. The pallet lifts in 12 ms for a soft key and 3 ms for a hard
one, along a raised cosine, so velocity sets the attack, and closes in 8 ms.
Channel pressure lets a held key rise part way: at full pressure its travel
rests at 0.30, a pallet opening of 0.2 (about 1 mm), where the reed's pitch
follows the bellows pressure as a half-pressed key's does. The model's C4
falls 5.9 Hz per kPa there against the 5.1 Hz/kPa measured with the pallet
lifted 1 mm (Adachi et al., Forum Acusticum 2026), and 1.5 Hz/kPa fully open
against 2.5. At the 70 dBA of a published bend measurement, a half-pressed
A#2–A3 at a steady bellows pressure falls 13 cents, close to the 16 a player
reached with the finger alone, but loses 20 dB where the player's lost 11
(Known gaps).
Up to 72 chambers sound at once; a new one takes
the oldest released chamber first.

### Registers and the bass side

The treble has five ranks of reeds: L (16', an octave down), M (8'), M− and
M+ (8', tuned below and above M for the tremolo) and H (4', an octave up,
breaking back an octave at the top so that no reed sounds above C♯7). The
eight register switches combine them: Clarinet (M), Bassoon (L), Violin (M,
M+), Musette (M−, M, M+), Bandoneon (L, M), Organ (L, H), Oboe (M, H) and
Master (all five). A switch moves slides over the reed blocks, so it reaches
held keys too: the ranks it opens start to speak as from the softest key
stroke and the ones it shuts fall silent. Each register's treble side is
trimmed so all eight average one level on a C3, a C4 triad and a C6: untrimmed,
Master is 1.9 dB louder than Musette and a single rank 3.5–3.9 dB softer, as
on a real accordion, but a switch here changes the colour, not the level.
**Musette** sets the M+ detune at A4 in cents; the beat
rises with pitch as f^0.7, as tuners set tremolo reeds, and M− sits the same
interval below.

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
straight out; **Grille** sets the treble side's brightness.

The bass pallets open into the bass box, the space under the bass end cover
round the bass mechanism, which vents through the cover's holes: a
perforated-panel Helmholtz resonator, a second-order low-pass on the bass
side. Per unit of cover area its air mass is ρl′/open and its compliance
depth/ρc², so f = c/2π·√(open/(depth·l′)), loaded by radiation (ρc) and the
cloth's flow resistance. Typical values (40 mm deep, 10 % open, 6 mm holes in
3 mm wood, cloth of 30 rayl) give 960 Hz at Q 1.3, a 3 dB rise before the
roll-off; their plausible spread is 0.55–2.0 kHz. The box is shallow enough
there for a lumped model, which holds within 2 dB up to 2 kHz. The air valve
opens into the bass box as well, so its hiss leaves the same way.

### The output stage

On Reason's VU meter (the louder channel's RMS over 300 ms, +12 dB) the demo
phrases' notes read −9 at their median, and a chord at the arm's full pressure
would read 23 dB above that. The output stage is the one the newer
Protocodus instruments share (`Source/DSP/OutputStage.h`). Its compressor
follows the louder channel's power over 20 ms and brings it down 10:1 above
−5 VU, over an 8 dB knee, so playing below about −9 VU passes untouched; it
takes hold at once and lets go over 300 ms, slower than a musette's beating.
Its peak limiter looks 2 ms ahead (96 samples at 48 kHz, the engine's
latency) and holds what gets past at −1.5 dBFS with no gain step. The hardest
strokes read −3.0 VU at most. Volume sets the level going into the stage.

### Controls, presets and MIDI

| Control | Range | What it does |
| --- | --- | --- |
| Register | 8 switches | the treble ranks that sound |
| Musette | 0–40 cents | tremolo detune at A4 |
| Bellows | 0.5–8 L | air one sweep of the wheel moves |
| Assist | 0–1 | how far the hand's air follows the open reeds |
| Grille | 2–16 kHz | brightness of the treble grille |
| Width | 0–1 | treble keys across the right, bass side to the left |
| Bass | −24 to +6 dB | bass and chord side level |
| Volume | −36 to +6 dB | level into the output stage |
| Keyboard | split, full | split: C1–B1 bass buttons, C2–B2 chord buttons, C3 up treble |
| Air | off, on | the air valve |
| Momentum | 0–200 ms | how far the arm trails the wheel; 0 follows it exactly |
| Oversampling | 1×, 2×, 4× | the reeds' time step: 1× is four times the host rate; 2× and 4× cost about two and four times the CPU |

`Source/DSP/RespiraControls.h` holds these for every front end;
`Source/DSP/RespiraPresets.h` holds the eight factory presets, one per
register, each setting its Volume so it sits within 1 dB of Init on one
reference stroke (a bass, a chord and a C major triad on a moderate 2 s pull
and push), with its peak below −3 dBFS.

MIDI channel 1 is the keyboard (split as above, or all treble), channel 2 the
bass buttons and channel 3 the chord buttons by pitch class; other channels
play as channel 1. CC1 + CC33 are the hand, CC64 the air valve, channel
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
  estimates scaled with pitch, and the full-scale reference is set so the
  demo phrases read −9 VU, not by a measured level; no recording of a real accordion is
  part of the model or its tests.
- **CPU.** A full-register chord over bass and chords (28 chambers) takes
  about a quarter of one core in the development container at 48 kHz (0.23×
  realtime for the Master Tutti demo, 0.02× for the single clarinet reed),
  and a chord change briefly doubles that while the released chambers close.
  Only the reed gap's constants are made once per reed (about 7 % of the
  time). Running the oversampled steps outside the chamber loop, and one
  reciprocal of the gap area per reed step in place of three divisions,
  were tried and changed nothing measurable (within the container's ±3 %
  timing noise), so the reed step itself stays as written.
- **Chord buttons are pitch classes**, not Stradella's fixed major, minor,
  seventh and diminished rows, and there are no bass register switches.
- **One turbulence seed.** Two engines given the same input play identical
  noise.
- **The bass box above 2.5 kHz.** Its lumped resonator falls 12 dB per
  octave there. As a layer of air under the cover the box rings at its depth
  resonances instead, and what it really loses depends on the felt, leather
  and wood inside it, which are not modelled; the bass side's top octaves
  are probably too dark.
- **The top harmonics, against a measurement.** The one published
  harmonic measurement found, a concert accordion's 8' reeds from A#2 to A3
  played mezzo forte, about 70 dBA at 50 cm (Elejalde-García, Macho-Stadler
  and Llanos-Vázquez 2021), falls without a peak from the fundamental to about
  −13 dB at the 15th–20th harmonics. Respira's 8' cells read 70 dBA at 50 cm
  at about 200 Pa, as a small source in free field (`CompareHarmonics`);
  there its 2nd–14th harmonics agree with the measurement within 2.5 dB on
  average, but its 15th–20th sit 6 dB low, and 3–8 dB low at every pressure
  from 150 to 1200 Pa. The likely cause is the reed chamber, a lumped
  resonator (1.3 kHz for C3 rising to 3.7 kHz for C7) that cuts 12 dB per
  octave above its resonance; from A#2 to A3 that takes 5–16 dB off the
  14th–20th harmonics on average. The chamber is as long as its reed, so the lumped
  model holds only up to about its resonance (k L ≈ 1 there). A chamber in
  sections, the reed's air entering towards its closed end, matched the
  measurement within 2.1 dB but made the demos 12–15 dB brighter at
  4–16 kHz, by an amount the number of sections set; what a real chamber
  loses above its resonance is not known, so it was not kept
  (`Docs/decisions.md`). Separately, against 500 Hz–1 kHz the demos read 3–9 dB more than a
  polka recording at 1–4 kHz; the recording's distance, room and registers
  are not known, and the cause is not traced.
- **Half-pressed keys lose too much level.** In the same paper a player
  bent A#2–A3 by closing the pallet part way at a steady bellows pressure:
  16 cents flat on average and 11 dB quieter (the mean over the first twenty
  harmonics). Respira's 8' cells at that loudness, with the pallet opened 0.2
  as full channel pressure holds it, fall 13 cents and 20 dB
  (`CompareHarmonics`): nearly twice the level for a slightly smaller bend.
  The player found no bend possible from C6 up; a single C6 cell still bends
  17–28 cents at 200–400 Pa, while from E6–G6 up the half-open pallet
  silences the reed.
  At a steady wheel, Assist changes the picture: it counts each reed by its
  pallet's opening, but a half-open pallet's reed still draws 0.3–0.9 of its
  open air, so half-pressing takes the hand's extra air away and the pressure
  falls; a low note then barely bends (C3: 2 cents where the player reached
  24, 11 dB quieter), and one struck under full channel pressure can fade out. Counting held keys
  in full instead raises the pressure into the bend but overbends A#2–C#3
  (37–81 cents against at most 34 measured); a listening test of the two is pending.
- **The top octave speaks slowly, unchecked.** With the bellows already at
  540 Pa, an A4 reaches its level about 50 ms after the key (some 20
  periods), a C6 about 90 ms and a C7 about 130 ms (some 270 periods): the
  pallet's 3–12 ms lift is slow against a high reed's period, so it grows from
  the gap's turbulence rather than from a kick. No measured onset times of
  accordion reeds were found to check this against.
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
accordion measurement; `GenerateReedTuning Source/DSP/ReedTuning.h` re-tunes every reed after
a change to the reed model. `python3 Tools/fit_bellows.py
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
