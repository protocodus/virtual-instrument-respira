#!/usr/bin/env python3
"""Measure how a bandoneon's pitch and tone follow its loudness, for
Tools/CompareBandoneon.cpp.

The reference: BANDONEON v1 by Miguel Lopez, a free Decent Sampler library
on Pianobook (https://www.pianobook.co.uk/profile/mikelopez/), recorded mono at
44.1 kHz. Pianobook's terms do not allow giving the samples away, so they are
not in this repository; only the numbers measured from them are. Point this at
the library's folder (the one holding "samples b2"):

    python3 Tools/measure_bandoneon.py <BANDONEON folder>

Protocol. The legato and expresivo takes of each note, C2 to C6 (one sample
per third, both hands; der is the right hand, izq the left), are held notes
whose loudness the player moves with the bellows, so pitch and tone can be
read against loudness without knowing the pressure. Each take is cut into
150 ms Hann frames every 50 ms. In each frame: the RMS level in dB, and the
first sixteen harmonics, each the largest spectral peak within 1.5 % of k
times the note (the library is tuned 12 cents sharp of A = 440 Hz), refined
by a parabola through the log magnitudes; the pitch is the power-weighted
mean of f_k / k over k = 1..8. Frames count from 0.3 s after the take first
comes within 20 dB of its loudest frame, while within SPAN dB of it, so the
attack is left out. The slopes are least squares fits against the level of
the pitch in cents and of h_k - h_1 in dB, averaged over a note's takes.

Each note has two reeds an octave apart, so only the odd harmonics (3 to 15)
belong to the note's reed alone; the even ones mix in the octave reed. Read
from the note's own reed (k = 1, 3, 5, 7), the pitch slopes average the same
(0.38 against 0.39 cents per dB over the loudest 30 dB, 0.49 against 0.47
over 15), though single notes move by up to 0.37.

The attack's pitch: the same takes in 100 ms frames every 10 ms, the pitch
of the note's own reed (the power-weighted mean of f_k / k over k = 1, 3, 5
and 7) against the level fitted as above over the loudest 30 dB, and the
frames where the attack first comes within 10 and 3 dB of the take's loudest
set against that line, in cents (negative is flat), averaged over a note's
takes. The octave reed speaks sooner, so the pitch from k = 1..8 read the
low attacks 10-25 cents less flat. A reed sounds flatter while its swing is
small; the player's bellows sets how fast the attack rises, which the
recordings do not give.
"""
from pathlib import Path
import re
import sys
import warnings

import numpy as np
from scipy.io import wavfile

RIGHT = {"C": 60, "E": 64, "G": 67, "B": 71, "D": 74, "F": 77, "A": 81, "C3": 84, "C4": 84}
LEFT = {"C": 36, "E": 40, "G": 43, "B": 47, "D": 50, "F": 53, "A": 57, "C2": 60}
SHARP_CENTS = 12.0
HARMONICS = 16
ODD = (3, 5, 7, 9, 11, 13, 15)
ONSET = (-10, -3)
OWN = np.array([1, 3, 5, 7])

# The library's WAVs carry chunks scipy does not read; the audio is unaffected.
warnings.filterwarnings("ignore", category=wavfile.WavFileWarning)


def note_of(name):
    m = re.match(r"Sband (\w+?)( |\.wav)", name)
    letter = m.group(1)
    letter = letter.upper() if len(letter) == 1 else letter
    return (LEFT if "izq" in name else RIGHT)[letter]


def harmonics(frame, rate, f0):
    n = len(frame)
    spectrum = np.abs(np.fft.rfft(frame * np.hanning(n), 8 * n))
    bins = np.fft.rfftfreq(8 * n, 1.0 / rate)
    levels, freqs = [], []
    for k in range(1, HARMONICS + 1):
        lo, hi = np.searchsorted(bins, k * f0 * 0.985), np.searchsorted(bins, k * f0 * 1.015)
        if hi + 1 >= len(spectrum):
            levels.append(np.nan)
            freqs.append(np.nan)
            continue
        i = lo + int(np.argmax(spectrum[lo:hi]))
        a, b, c = np.log(spectrum[i - 1:i + 2] + 1e-30)
        p = 0.5 * (a - c) / (a - 2 * b + c) if a - 2 * b + c != 0 else 0.0
        levels.append((b - 0.25 * (a - c) * p) * 20 / np.log(10))
        freqs.append(bins[i] + p * (bins[1] - bins[0]))
    return np.array(levels), np.array(freqs)


def frames(path, key, width_s=0.15, hop_s=0.05):
    rate, x = wavfile.read(path)
    x = x.astype(np.float64)
    x = (x.mean(axis=1) if x.ndim > 1 else x) / 2**31
    f0 = 440.0 * 2 ** ((key - 69) / 12 + SHARP_CENTS / 1200)
    width, hop = int(width_s * rate), int(hop_s * rate)
    out = []
    ks = np.arange(1, 9)
    for start in range(0, len(x) - width, hop):
        frame = x[start:start + width]
        rms = np.sqrt(np.mean(frame * frame))
        if rms < 1e-5:
            continue
        levels, freqs = harmonics(frame, rate, f0)
        power = 10 ** (levels[:8] / 10)
        pitch = np.nansum(freqs[:8] / ks * power) / np.nansum(power)
        power = 10 ** (levels[OWN - 1] / 10)
        own = np.nansum(freqs[OWN - 1] / OWN * power) / np.nansum(power)
        out.append((start / rate, 20 * np.log10(rms), 1200 * np.log2(pitch / 440.0), levels,
                    1200 * np.log2(own / 440.0)))
    return out


def slopes(path, key, span):
    rows = frames(path, key)
    t = np.array([r[0] for r in rows])
    level = np.array([r[1] for r in rows])
    cents = np.array([r[2] for r in rows])
    h = np.array([r[3] for r in rows])
    peak = level.max()
    begin = t[np.argmax(level > peak - 20)] + 0.3
    use = (t >= begin) & (level > peak - span)
    result = [np.polyfit(level[use], cents[use], 1)[0]]
    for k in ODD:
        y = h[use, k - 1] - h[use, 0]
        ok = np.isfinite(y)
        result.append(np.polyfit(level[use][ok], y[ok], 1)[0])
    return result


def onset(path, key):
    rows = frames(path, key, 0.1, 0.01)
    t = np.array([r[0] for r in rows])
    level = np.array([r[1] for r in rows])
    cents = np.array([r[4] for r in rows])
    peak = level.max()
    begin = t[np.argmax(level > peak - 20)] + 0.3
    use = (t >= begin) & (level > peak - 30)
    a, b = np.polyfit(level[use], cents[use], 1)
    first = [np.argmax(level > peak + d) for d in ONSET]
    return [cents[i] - (a * level[i] + b) for i in first]


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    folder = Path(sys.argv[1]) / "samples b2"
    takes = {}
    for path in sorted(folder.glob("*.wav")):
        name = path.name
        if any(word in name for word in ("stac", "latigo", "exp ", "exp inz", "exp der")):
            continue
        takes.setdefault(note_of(name), []).append(path)
    print("// key, cents/dB over the top 30 dB and 15 dB, d(h3, h5 .. h15 - h1)/d(level) over the top 30 dB,")
    print("// the attack's cents off the line 10 and 3 dB under the loudest")
    for key in sorted(takes):
        wide = np.mean([slopes(p, key, 30) for p in takes[key]], axis=0)
        narrow = np.mean([slopes(p, key, 15)[0] for p in takes[key]])
        attack = np.mean([onset(p, key) for p in takes[key]], axis=0)
        odd = ", ".join(f"{v:.2f}" for v in wide[1:])
        flat = ", ".join(f"{v:.1f}" for v in attack)
        print(f"    {{ {key}, {wide[0]:.2f}, {narrow:.2f}, {{ {odd} }}, {{ {flat} }} }},   // {len(takes[key])} takes")


if __name__ == "__main__":
    main()
