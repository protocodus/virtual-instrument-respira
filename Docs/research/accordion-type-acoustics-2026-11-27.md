# Accordion Type construction and anti-phaser acoustics (2026-11-27)

Engineering and acoustic research behind Respira `0.10.0`. Builds on
[accordion construction by country (2026-11-19)](accordion-construction-by-country-2026-11-19.md)
and [accordion schools by country (2026-11-24)](accordion-schools-by-country-2026-11-24.md),
addressing two goals requested for **Type**: making Irish, German and Balkan
physically distinct and authentic to their schools, and removing the phaser-like
swept comb filtering present in `0.9.0`.

## Summary

In `0.9.0`, the four shipped types (`Default`, `Irish`, `German`, `Balkan`)
diverged mainly in Musette detuning, rank mask (`LMM` vs `LMMH` vs `LMMMMH`),
B/C bellows-direction switching on Irish, a global `cover` factor on `Grille`,
and Balkan's `cassottoBox`. Every type shared Default's reed tongue lengths and
widths, identical reed plates (`German`), identical 6 L bellows air volume and
stiffness, identical 960 Hz Stradella bass box, and zero body/grille voicing on
single-reed registers—leaving **German Clarinet C4 within `1.3 dB` max (`1.0 dB`
RMS) of Default** across 125 Hz–8 kHz octave bands. Meanwhile four structural
defects produced audible phaser-like sweeping on held notes:

1. **Over-driven swept early reflections (`Source/DSP/NearField.h`)** carving
   `-24.7` to `-35.5 dB` comb notches at `Life 0.5` (`-36.5` to `-55.2 dB` at
   `Life 1.0`), exacerbated on Irish (`-35.5 dB`) by assuming a 6 L bellows in
   a `0.29 x 0.16 m` frame (implying `129.3 mm` of bellows travel).
2. **`cassottoBox` phase cancellation (`Source/DSP/RespiraEngine.cpp`)**
   summing a 2nd-order low-pass (`1344 Hz`) in opposite phase to a 2nd-order
   high-pass depth-mode leak (`2800 Hz`), creating an `-8.85 dB` notch at
   `2529 Hz` followed by a rebound to `-3.3 dB` at `6 kHz`.
3. **Balkan 3+1 cassotto placing both dry `8'` reeds inside the tone
   chamber** with independent manufacturing scatter (`setScale`, `plateScale`,
   `clearanceScale`), producing different pressure-pitch slopes (`df/dp`) and
   slow unison phaser sweeps (`3.34 dB` at C4, `3.14 dB` at A4 over a `0.4 s`
   window on Balkan Violin).
4. **Synchronized multi-harmonic tremolo beats** on pairs with identical chamber
   depth/hole geometry and uniform `±4.5 dB` random offsets landing near `0 dB`
   on ~1/3 of keys.

`0.10.0` replaces shared assumptions with school-specific physical modeling
across reeds, chambers, blocks, bellows, bass boxes, keybeds, and passive
body/grille acoustics, while eliminating all four phasing mechanisms.

---

## Quantified phaser root causes (0.9.0 vs 0.10.0)

| Mechanism | 0.9.0 Root Cause & Measurement | 0.10.0 Physical Fix & Measurement |
| --- | --- | --- |
| `NearField` early-reflection comb notches | `kPathLevel = 1.6`, reflection weights `0.48 / 0.42 / 0.35`, Path 2 bellows-travel projection `1.0` instead of `~sin(14°) = 0.25`, `kClothCorner = 3500 Hz`, linear `2 * Life`, and `129.3 mm` Irish travel -> worst notches **-24.7 dB (Default), -35.5 dB (Irish), -29.6 dB (German), -31.5 dB (Balkan)** at `Life 0.5`, and **-36.5 to -55.2 dB** at `Life 1.0`. | Bounded physical reflection weights (`0.14 / 0.11 / 0.10`, `kPathLevel = 1.0`), geometric `0.25` bass-travel projection, `kClothCorner = 2200 Hz`, spread rank pallet-hole spacing `kRow`, soft-saturated Life curve `1.5 * amount / (0.5 + amount)` (`1.0x` at `Life 0.5`, `1.2x` at `1.0`), per-build `bellowsVolume` (`58.2 mm` Irish travel) -> worst notch **-2.15 to -2.73 dB (`Life 0.5`)** and **-2.65 to -3.39 dB (`Life 1.0`)**, `-3.382 dB` verified by FFT across all builds and travels in `NoPhaserCombNotches`. |
| `cassottoBox` high-mid phase notch | `depth = 0.030 m`, `open = 0.20`, `neck = 0.011 m`, `rayl = 25`, `absorbed = 0.04`, `modes = 2800 Hz`, `leakScale = +1.0` -> `f_H = 1344 Hz, Q = 1.28`, peaking only `+2.0 dB` at `1078 Hz`, scooping **-8.85 dB at 2529 Hz** from `-155°` LP vs `+45°` HP summation, then rebounding to `-3.3 dB` at `6 kHz`. | True deep wooden tone chamber (`depth = 0.048 m`, `open = 0.14`, `neck = 0.015 m`, `rayl = 18`, `absorbed = 0.12`, `modes = 2200 Hz`, signed `leakScale = -0.18`) -> **`f_H = 761 Hz, Q = 1.43`**, **`+3.76 dB` formant peak at `663 Hz`**, monotonic roll-off (**-13.4 dB @ 2 kHz, -16.7 dB @ 3 kHz, -18.6 dB @ 4 kHz, -21.8 dB @ 6 kHz**), matching Wikipedia/Elejalde-García et al. (2021) cassotto reinforcement in `500–1000 Hz`. |
| Balkan dry-unison slow phaser drift | Both `M` and `M+` sat inside cassotto with independent random `Variation` (`setScale`, `plateScale`, `clearanceScale`), giving divergent `df/dp` pressure-pitch slopes -> **`0.4 s` RMS spread `3.34 dB` (C4), `3.14 dB` (A4)** on held Violin. | True 2+2 double cassotto (`L` & `M` inside, `M+` & `H` outside) plus `BuildVariation` matching `setScale` and `plateScale` and halving scatter between dry unison `M` and `M+` (`build.tremolo == 0`) -> **`0.4 s` RMS spread `0.85 dB` (`EngineTests`) / `1.00 dB` (`render_set`) at C4 and `0.94 dB` / `2.10 dB` at A4**. |
| Tremolo multi-harmonic cancellation wipes | Equal inner/outer chamber blocks (`block`, `hole`) and `kTremoloSpreadDb = ±4.5 dB` uniform random per key (zero-crossing keys cancel all harmonics in phase). | Outer tremolo ranks (`M-`, `M+`) get build-specific `outerBlock`, `outerHole`, and negative `outerGainDb` offset (`-1.5 dB` Irish/German; `-2.5 dB` Balkan outer radiation contrast) plus bounded random variation (`±3 dB`), preventing full-depth multi-harmonic LFO wipes. |

---

## Real-world construction research & 0.10.0 physical mapping

Each school maps its documented organology (Maugein, Hohner, Castagnari,
Guerrini/Dallapè/Scandalli, Harmonikas, Liberty Bellows, Hergert FA 2023,
Elejalde-García et al. 2021, Pomberger et al. DAGA 2021) onto respira's
reed, block, bellows, bass-box, keybed, and passive body-voicing parameters.
Per `CLAUDE.md`, reed dimensions, stiffness multipliers, bass-box geometries,
and passive body-voicing filters remain physically grounded estimates (no
measured accordion laboratory corpus exists per school).

### 1. Irish (`kTypeIrish` — 2-Row B/C Diatonic Hardwood Button Box)

- **Reference tradition:** Castagnari, Saltarelle, Paolo Soprani 2-row B/C
  boxes (`accordion-schools-by-country-2026-11-24.md`, McNeela's guide).
- **Reed configuration:** `LMM` (`16'`, `8'`, `8'+`), **6¢ demi-swing** at the
  default 22¢ Musette knob (`tremolo = 6/22`), flatter keyboard beat growth
  `beatExp = 0.38` so the shimmer stays brisk in the low octave without buzzing
  in the upper reel register (`f^0.38`, inside Hergert 2023's sub-octave
  survey range).
- **Reed & block geometry (`ReedBank.h` / `ReedModel.h`):** Smaller compact
  plates and reeds (`lengthScale = 0.90`, `widthScale = 0.92`, `plate = 0.86`,
  `q = 0.95`, `set = 1.00`, `clearance = 1.00`, `profile = 0.88`, shallow
  blocks `block = 0.72`, `hole = 0.82`; outer block `0.86`, `outerHole = 0.92`,
  `outerGainDb = -1.5`). Keeping `set = 1.00` with `lengthScale = 0.90` avoids
  static choking on bellows accents (`x(1 kPa)/rest = 0.83..1.11` across
  C2–C8, verified before finalizing).
- **Bellows & action:** Small 2-row box (`0.29 x 0.16 x 0.13 m`, `reach = 0.72`,
  buttons, `bisonoric = true`), real **`2.7 L` bellows volume** (producing
  realistic `58.2 mm` full travel rather than `0.9.0`'s `129.3 mm`),
  `bellowsStiffness = 1.8`, `bellowsLeak = 0.9`, `keybedLength = 0.29 m`
  (`Mechanics.h::KeyBed::SetLength`). Measured in `IrishBellowsTurnsSnappily`:
  push-to-pull reversal under Auto reaches `-300 Pa` in **`0.556x` Default's
  time**.
- **Bass side & hardwood body:** 8/12-bass shallow wooden casing (`depth = 0.024 m`,
  `open = 0.14`, `neck = 0.0065 m`, `rayl = 25`, `absorbed = 0.05`,
  `modes = 2800 Hz`, putting `f_H ≈ 1.6 kHz`), reduced `16'` contrabass weight
  (`bass16Gain = 0.45`, `bass8Gain = 0.90`, `chordGain = 0.85`). Small rigid
  hardwood treble casing modeled by `cover = 1.45` and `BodyVoicing` (`260 Hz`
  low shelf `-3.0 dB`, `900 Hz` woody presence `+3.0 dB`, `Q 1.1`).
- **Factory presets (`RespiraPresets.h`):** `Irish Button Box` (`Musette` `reg 3`,
  `momentum = 15 ms`, `width = 0.5`, `grille = 1000 Hz`, `bass = -2 dB`) and
  `Irish Session` (`Master` `reg 7` -> `LMM`, same frame/momentum/bass).

### 2. German (`kTypeGerman` — Trossingen 4-Voice Piano Accordion)

- **Reference tradition:** Hohner Atlantic / Verdi / Trossingen 4-voice Piano
  models (`LMMH`), standard German 2-reed tremolo (`440 & 444 Hz` = `15¢`,
  Liberty Bellows / Hergert 2023).
- **Reed configuration:** `LMMH` (`16'`, `8'`, `8'+`, `4'`), **`15¢`** two-reed
  sharp tremolo (`tremolo = 15/22`), steeper Hohner-like beat growth
  `beatExp = 0.65`.
- **Reed & block geometry:** Factory-stamped machine tolerances modeled via wider
  slot clearance (`clearance = 1.35`, matching Harmonikas machine-reed gap vs
  *a mano*), narrower compact tongues (`lengthScale = 0.94`, `widthScale = 0.88`,
  `plate = 0.88`, `q = 0.85`, `set = 1.08`, `profile = 0.95`, `block = 0.92`,
  `hole = 0.95`; outer block `0.86`, `outerHole = 0.92`, `outerGainDb = -1.5`).
- **Bellows & action:** Full 41-key Hohner casing (`0.44 x 0.20 x 0.15 m`,
  `reach = 1.06`, piano keys, `keybedLength = 0.44 m`), `bellowsVolume = 5.5 L`,
  `bellowsStiffness = 1.1`, `bellowsLeak = 0.85`.
- **Bass side & stamped aluminum grille:** Snappy 4-voice Stradella bass box
  (`depth = 0.032 m`, `open = 0.12`, `neck = 0.0072 m`, `rayl = 22`,
  `absorbed = 0.04`, `modes = 2400 Hz`, `f_H ≈ 1.25 kHz`, `bass8Gain = 1.12`,
  `chordGain = 1.05`, `reedClearance = 1.15`). Open stamped-aluminum grille and
  un-cassotto treble compartment (`cover = 1.65`, `BodyVoicing`: `260 Hz` low
  shelf `+0.5 dB`, `750 Hz` scoop `-3.5 dB @ Q 0.9`, `3200 Hz` presence peak
  `+4.5 dB @ Q 0.85`).
- **Factory presets:** `German Club` (`Violin` `reg 2`, `width = 0.6`,
  `grille = 1000 Hz`, `bass = 0 dB`) and `German Master` (`Master` `reg 7` ->
  `LMMH`, `width = 0.8`, `bass = +1 dB`).

### 3. Balkan (`kTypeBalkan` — Serbian 6-Row B-System Dugmetara)

- **Reference tradition:** Serbian / Italian double-cassotto 6-row dugmetara
  (Guerrini President, Dallapè Supermaestro, Scandalli Super VI, Bugari
  Balkan DM).
- **Reed configuration:** `LMMH` with **true 2+2 double cassotto** (`cassotto =
  { true, true, false, false, false }`: `16'` and concert `8'` inside the deep
  tone chamber; second `8'` and `4'` outside on the open reed block), **tuned
  dry** (`tremolo = 0.0`, chosen by ear on `2026-11-25`).
- **Reed & block geometry:** Broad, heavy hand-made *a mano* reeds
  (`lengthScale = 1.08`, `widthScale = 1.12`, `plate = 1.12`, `q = 1.18`,
  `set = 0.94`, tight `clearance = 0.75`, `profile = 1.05`, deep inner blocks
  `block = 1.20`, `hole = 1.08`; outer `M+`/`H` contrasted with `outerBlock =
  0.90`, `outerHole = 0.95`, `outerGainDb = -2.5`). For dry unison (`build.tremolo
  == 0`), `BuildVariation` matches `setScale` and `plateScale` between `M` and
  `M+` and halves scatter on `clearanceScale`, `qScale`, `profileScale`, and
  `chamberScale`.
- **Cassotto & body:** Deep wooden tone chamber (`f_H = 761 Hz, Q = 1.43`,
  monotonic roll-off, `cover = 0.82`), complemented by warm chest radiation
  (`BodyVoicing`: `380 Hz` `+1.5 dB`, `750 Hz` `+0.8 dB @ Q 1.2`).
- **Bellows & 5-voice bass side:** Large 6-row button frame (`0.43 x 0.21 x 0.16 m`,
  `reach = 1.05`, buttons, `keybedLength = 0.43 m`), `bellowsVolume = 6.8 L`,
  `bellowsStiffness = 0.92`, tight gasket `bellowsLeak = 0.55`. Deep 5-voice
  bass chamber (`depth = 0.056 m`, `open = 0.08`, `neck = 0.010 m`, `rayl = 35`,
  `absorbed = 0.06`, `modes = 1600 Hz`, `f_H ≈ 650 Hz`, boosted `16'` contrabass
  `bass16Gain = 1.35`, tight `reedClearance = 0.75`).
- **Factory presets:** `Balkan Kolo` (`Master` `reg 7`, `width = 0.8`,
  `grille = 1000 Hz`, `bass = +1 dB`) and `Balkan Cassotto` (`Bandoneon`
  `reg 4` — `16' + 8'` both inside cassotto, `width = 0.7`, `bass = 0 dB`).

---

## Verification & Before/After Measurements (48 kHz)

- **Types spectral separation (`Clarinet` C4, 125 Hz–8 kHz octave bands,
  mean-subtracted, `render_set.py`):**
  - `Default vs German`: `1.3 dB` max (`1.0 dB` RMS) in `0.9.0` -> **`3.3 dB` max (`2.1 dB` RMS)** in `0.10.0`.
  - `Default vs Irish`: `3.3 dB` max (`2.5 dB` RMS) -> **`4.3 dB` max (`2.7 dB` RMS)**.
  - `Default vs Balkan`: `7.5 dB` max (`5.4 dB` RMS) -> **`8.4 dB` max (`6.0 dB` RMS)**.
  - `Irish vs German`: **`2.8 dB`** max; `Irish vs Balkan`: **`10.5 dB`**; `German vs Balkan`: **`11.0 dB`**.
  - In `EngineTests::TypesAreSpectrallyDistinct` (`1.0–1.68 s` steady window):
    smallest pairwise max band delta is **`3.062 dB` on Clarinet** and
    **`3.594 dB` on Master**.
- **Anti-phaser tests (`EngineTests::NoPhaserCombNotches`):**
  - Worst `NearField` early-reflection comb notch across all 4 types, `Life
    0.5–1.0`, `travel 0–1`: **`-3.382 dB`** (pass `> -6.0 dB`).
  - Balkan Violin (`2+2` dry unison) held-note `0.4 s` RMS spread: **`0.8475 dB`
    at C4** (`1.00 dB` on full phrase), **`0.9425 dB` at A4** (`2.10 dB` on full
    phrase), down from `3.34 / 3.14 dB` in `0.9.0`.
  - Balkan Clarinet C3 cassotto roll-off (`500–1k` minus `2–4k`): **`21.34 dB`**
    with no 4–8 kHz rebound.
- **Level calibration & limits (`RespiraEngineTests`):**
  - `TypesSitTogether`: all four types within **`0.04 dB`** of Default (`Irish
    -0.04`, `German -0.03`, `Balkan +0.03`).
  - `PresetsSitTogether`: all 14 factory presets within **`0.30 dB`** of Init
    (`-0.30` to `+0.30 dB`, max peak `-2.54 dBFS`).
  - `Default` tuning table (`Source/DSP/ReedTuning.h`): unchanged to the digit;
    all `3304` push/pull ratios verified by `GenerateReedTuning --check`.
- **CI suites on Apple M1 Max Release build:** `ctest -L unit` (`8/8`),
  `ctest -L audio` (`1/1`), `ctest -L perf` (`1/1`: Clarinet `0.0183x`, Master
  Tutti `0.0866x` realtime), and `RespiraParameterMatrixTests --rotation --jobs 4`
  (`607 cases, 639 runs, 0 failed`), plus `RespiraDSPCxx17` (`-fno-builtin
  -fno-exceptions`) with zero warnings.
