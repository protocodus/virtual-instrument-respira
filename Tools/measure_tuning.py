#!/usr/bin/env python3
"""Measure how far a played accordion's reeds sit off their pitch, for the
tuning scatter in ReedBank.h (Variation).

The reference: La Melodiosa by Petri Pohjanmies (2026, CC BY), not in this
repository. Register I sounds as its file names say, register II two octaves
under them, register III both; each note D#4..C7 by minor thirds is held in
two takes (ba, va). --melodiosa is the folder holding I-*.wav, II-*.wav and
III-*.wav.

    python3 -I Tools/measure_tuning.py --melodiosa <dir>

Protocol. Single reeds (registers I and II): each take from 1.0 s to 1.0 s
before its last sample above 1e-3 of its peak, in 200 ms Hann frames; a
frame's pitch is the power-weighted mean of f_k / k over k = 1..8, f_k the
largest peak within 3 % of k times the note, refined by a parabola through
the log magnitudes; the take's pitch is the median over its frames in cents
from equal temperament at A = 440 Hz, a note's the mean of its two takes. A
register's reeds scatter by the standard deviation of their notes about a
quadratic in the note number (which takes up the instrument's own stretch
and reference), over n - 3 degrees of freedom.

Reed pairs (register III): each take over the same span as one Hann-windowed
spectrum at 0.02 Hz bins. The low reed's pitch is its strongest of the 1st,
3rd and 5th harmonics (within 3 %) over its number; near 4 times it lie the
low reed's 4th harmonic and the high reed's fundamental, the two strongest
peaks within 1.5 %. The high reed is the stronger, unless that one is within
1 cent of the low reed's harmonic and the other within 6 dB of it. A note's
offset is the mean of its two takes; the pairs scatter by the standard
deviation of those offsets (n - 1), and each reed of a pair by that over
sqrt 2. Each standard deviation keeps the takes' own spread, which a take's
bellows pressure adds; the tool prints that too.
"""
import argparse
from pathlib import Path
import warnings

import numpy as np
from scipy.io import wavfile

NAMES = ["D#4", "F#4", "A4", "C5", "D#5", "F#5", "A5", "C6", "D#6", "F#6", "A6", "C7"]
FIRST = 63   # D#4

warnings.filterwarnings("ignore", category=wavfile.WavFileWarning)


def load(path):
    rate, x = wavfile.read(path)
    x = x.astype(np.float64)
    x = x.mean(axis=1) if x.ndim > 1 else x
    x /= np.abs(x).max()
    end = np.nonzero(np.abs(x) > 1e-3)[0][-1]
    return rate, x[rate: end - rate]


def refined(spectrum, step, lo, hi):
    j = lo + int(np.argmax(spectrum[lo:hi]))
    log = np.log(spectrum[j - 1: j + 2] + 1e-30)
    d = log[0] - 2 * log[1] + log[2]
    return (j + (0.5 * (log[0] - log[2]) / d if d < 0 else 0.0)) * step, spectrum[j]


def frame_pitch(frame, rate, f0):
    n = len(frame)
    spectrum = np.abs(np.fft.rfft(frame * np.hanning(n), 8 * n))
    step = rate / (8 * n)
    found = [refined(spectrum, step, int(k * f0 * 0.97 / step), int(k * f0 * 1.03 / step)) for k in range(1, 9)]
    f = np.array([hz / k for k, (hz, _) in enumerate(found, 1)])
    p = np.array([a for _, a in found]) ** 2
    return float((f * p).sum() / p.sum())


def single_cents(path, key):
    rate, x = load(path)
    f0 = 440.0 * 2 ** ((key - 69) / 12)
    n = int(0.2 * rate)
    return 1200 * np.log2(np.median([frame_pitch(x[s: s + n], rate, f0) for s in range(0, len(x) - n, n)]) / f0)


def peaks(spectrum, step, hz, width, count):
    lo, hi = int(hz * (1 - width) / step), int(hz * (1 + width) / step)
    top = [j for j in range(lo + 1, hi - 1) if spectrum[j] >= spectrum[j - 1] and spectrum[j] > spectrum[j + 1]]
    return [refined(spectrum, step, j - 1, j + 2) for j in sorted(top, key=lambda j: -spectrum[j])[:count]]


def pair_cents(path, key):
    rate, x = load(path)
    size = int(rate / 0.02)
    spectrum = np.abs(np.fft.rfft(x * np.hanning(len(x)), size))
    step = rate / size
    f0 = 440.0 * 2 ** ((key - 69) / 12)
    low = max(((p[0][0] / k, p[0][1]) for k in (1, 3, 5) if (p := peaks(spectrum, step, k * f0, 0.03, 1))),
              key=lambda e: e[1])[0]
    near = [(1200 * np.log2(hz / (4 * low)), 20 * np.log10(a)) for hz, a in peaks(spectrum, step, 4 * low, 0.015, 2)]
    if abs(near[0][0]) < 1.0 and len(near) > 1 and near[1][1] > near[0][1] - 6.0:
        return near[1][0]
    return near[0][0]


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--melodiosa", required=True)
    args = parser.parse_args()
    folder = Path(args.melodiosa)
    for register, shift in (("II", -24), ("I", 0)):
        keys = np.arange(FIRST, FIRST + 3 * len(NAMES), 3) + shift
        takes = np.array([[single_cents(folder / f"{register}-{name}-{s}.wav", key) for s in ("ba", "va")]
                          for name, key in zip(NAMES, keys)])
        cents = takes.mean(axis=1)
        residual = cents - np.polyval(np.polyfit(keys, cents, 2), keys)
        print(f"register {register}: " + " ".join(f"{c:+.1f}" for c in cents) + " cents; "
              f"{np.sqrt(np.sum(residual ** 2) / (len(residual) - 3)):.2f} cents about a quadratic, "
              f"takes {np.std(takes[:, 0] - takes[:, 1]) / np.sqrt(2):.2f} apart")
    offsets = np.array([[pair_cents(folder / f"III-{name}-{s}.wav", key) for s in ("ba", "va")]
                        for name, key in zip(NAMES, np.arange(FIRST, FIRST + 3 * len(NAMES), 3) - 24)])
    note = offsets.mean(axis=1)
    print("register III, high reed against the low reed's 4th harmonic: " + " ".join(f"{c:+.1f}" for c in note) +
          f" cents; mean {note.mean():+.2f}, pairs {note.std(ddof=1):.2f} apart, each reed {note.std(ddof=1) / np.sqrt(2):.2f}, "
          f"takes {np.std(offsets[:, 0] - offsets[:, 1]) / np.sqrt(2):.2f} apart")


if __name__ == "__main__":
    main()
