#!/usr/bin/env python3
"""Measure how fast a two-reed accordion's musette beats across its treble,
against Respira's tuning law (ReedBank.h, RankNote: beat ~ f^0.7).

The reference: an old Hohner by Marek Kintosh (Pianobook, 2024), two reeds a
note, pitch-corrected and normalized, which moves its beats by no more than
its tuning (about 1 %). It is not in this repository. --hohner is its
Samples folder; the treble takes are *_<MIDI note>_single_KLV.wav.

    python3 -I Tools/measure_musette.py --hohner <dir>

Protocol. Each take from 1.0 s to 1.0 s before its end (its last sample
above 1e-4 of its peak): a Hann-windowed spectrum zero-padded to 0.02 Hz
bins; around the fundamental (within 2 %) the two strongest peaks, refined
by a parabola through the log magnitudes, are the two reeds, and their
spacing is the beat. (The 2nd-6th harmonics give the same rate over their
number within 0.05 Hz on most notes.) A least squares line through log beat
against log pitch gives the exponent and the beat at A4, with a 90 %
interval from 2000 bootstrap resamplings of the notes. The detune in cents
about that line, over n - 2 degrees of freedom, is how far the pair's
tuning scatters, and over sqrt 2 each reed's (ReedBank.h, Variation).
"""
import argparse
from pathlib import Path
import re
import warnings

import numpy as np
from scipy.io import wavfile

warnings.filterwarnings("ignore", category=wavfile.WavFileWarning)


def beat(path, key):
    rate, x = wavfile.read(path)
    x = x.astype(np.float64)
    x = x.mean(axis=1) if x.ndim > 1 else x
    x /= np.abs(x).max()
    end = np.nonzero(np.abs(x) > 1e-4)[0][-1]
    seg = x[rate: end - rate]
    size = int(rate / 0.02)
    sp = np.abs(np.fft.rfft(seg * np.hanning(len(seg)), size))
    log = np.log(sp + 1e-20)
    step = rate / size
    f0 = 440.0 * 2 ** ((key - 69) / 12)
    lo, hi = int(f0 * 0.98 / step), int(f0 * 1.02 / step)
    peaks = [j for j in range(lo + 1, hi - 1) if sp[j] >= sp[j - 1] and sp[j] > sp[j + 1]]
    a, b = sorted(peaks, key=lambda j: -sp[j])[:2]
    refined = [(j + 0.5 * (log[j - 1] - log[j + 1]) / (log[j - 1] - 2 * log[j] + log[j + 1])) * step for j in (a, b)]
    return min(refined), abs(refined[0] - refined[1])


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--hohner", required=True)
    args = parser.parse_args()
    rows = []
    for p in Path(args.hohner).glob("*_single_KLV.wav"):
        key = int(re.search(r"_(\d+)_single_KLV", p.name).group(1))
        rows.append((key, *beat(p, key)))
    rows.sort()
    for key, f, b in rows:
        print(f"MIDI {key}: {f:7.2f} Hz, beats {b:5.2f} Hz ({1200 * np.log2(1 + b / f):4.1f} cents)")
    f, b = np.array([r[1] for r in rows]), np.array([r[2] for r in rows])
    x, y = np.log(f / 440.0), np.log(b)
    p, a = np.polyfit(x, y, 1)
    rng = np.random.default_rng(1)
    boot = [np.polyfit(x[i], y[i], 1)[0] for i in (rng.integers(0, len(x), len(x)) for _ in range(2000))]
    print(f"beat ~ f^{p:.2f} (90 % {np.percentile(boot, 5):.2f}..{np.percentile(boot, 95):.2f}), "
          f"{np.exp(a):.2f} Hz ({1200 * np.log2(1 + np.exp(a) / 440):.1f} cents) at A4, "
          f"notes scattering {100 * np.std(y - (a + p * x)):.0f} % about the line")
    detune = 1200 * np.log2(1 + b / f) - 1200 * np.log2(1 + np.exp(a + p * x) / f)
    spread = np.sqrt(np.sum(detune ** 2) / (len(detune) - 2))
    print(f"the two reeds' detune scatters {spread:.2f} cents about the line, {spread / np.sqrt(2):.2f} each")


if __name__ == "__main__":
    main()
