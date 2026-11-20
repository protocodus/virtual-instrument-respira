#!/usr/bin/env python3
"""How a held note's partials stand and move: the figures NearField.h is fitted to.

  measure_partials.py single <note> <wav> [<note> <wav> ...]
  measure_partials.py pair   <note> <wav> [<note> <wav> ...]

<note> is the MIDI key the file holds (69 = A4); a file is one held note, a
recorded sample or a render. Stereo files are read as the mean of their
channels. The note's steady part is the longest stretch within 8 dB of the
70th percentile of its 50 ms RMS, less 0.35 s at each end.

Each partial k is taken by heterodyning at k f0 (f0 from the spectrum's peak
near the note) through a fourth-order zero-phase low-pass at min(40 Hz, f0/4)
(`single`) or min(60 Hz, f0/3) (`pair`), read 200 or 400 times a second.

single, per file and as medians over the files:
  jag    the standard deviation, dB, of harmonics 2-12's median levels about
         a straight line against log2(k): how uneven the spectrum is
  rel    the standard deviation over time, dB, of each of harmonics 2-8's
         level against the fundamental's: how far the partials move apart
  cor    the mean correlation of harmonics 2-8's levels (dB) with the
         fundamental's over time: 1 when they swell together

pair (a tremolo pair's note), per file and over the files:
  depth  each of the first four partials' beat depth, (p98 - p2) / (p98 + p2)
         of its envelope
  spread the standard deviation of those four depths within a note: 0 when
         every partial beats to one depth, as two like oscillators do

Figures (2026-11-19). La Melodiosa's single reeds, register I, D#4-C6, push
and pull (Petri Pohjanmies, CC BY): jag 5.9 (4.2-8.3), rel 1.9 (0.7-4.8 from
the 10th to the 90th percentile), cor 0.19. A Hohner's pairs, C4-C6 (Marek
Kintosh, Pianobook): depth 0.76 (0.28-0.94), spread 0.125; La Melodiosa's
register III: 0.51 (0.13-0.90), 0.19. Respira, Clarinet Solo and Violin Folk
on D#4-C6 in threes, each held 6 s on Auto at velocity 80: 0.6.2 jag 3.4,
rel 0.35, cor 0.99, depth 0.85, spread 0.018; 0.7.0 jag 4.0, rel 0.78, cor
0.82, depth 0.81, spread 0.064; 0.7.3 jag 5.0, rel 1.12, cor 0.66, depth
0.79, spread 0.090.

Needs numpy and scipy.
"""
import sys
import wave

import numpy as np
from scipy.signal import butter, sosfiltfilt


def load(path):
    with wave.open(path) as w:
        n, width, channels, rate = w.getnframes(), w.getsampwidth(), w.getnchannels(), w.getframerate()
        raw = w.readframes(n)
    if width == 2:
        x = np.frombuffer(raw, "<i2").astype(float) / 32768
    elif width == 3:
        a = np.frombuffer(raw, np.uint8).reshape(-1, 3).astype(np.int32)
        x = (a[:, 0] | (a[:, 1] << 8) | (a[:, 2].astype(np.int8).astype(np.int32) << 16)).astype(float) / 8388608
    else:
        x = np.frombuffer(raw, "<i4").astype(float) / 2**31
    return x.reshape(-1, channels).mean(axis=1), rate


def steady(x, rate):
    hop = int(0.05 * rate)
    rms = np.sqrt(np.convolve(x * x, np.ones(hop) / hop, "same"))
    on = np.append(rms > np.percentile(rms, 70) * 10 ** (-8 / 20), False)
    best, start = (0, 0), None
    for i, v in enumerate(on):
        if v and start is None:
            start = i
        if not v and start is not None:
            if i - start > best[1] - best[0]:
                best = (start, i)
            start = None
    margin = int(0.35 * rate)
    return x[best[0] + margin:best[1] - margin]


def pitch(x, rate, guess):
    spectrum = np.abs(np.fft.rfft(x * np.hanning(len(x))))
    f = np.fft.rfftfreq(len(x), 1 / rate)
    i = np.argmax(spectrum * ((f > guess * 0.94) & (f < guess * 1.06)))
    a, b, c = np.log(spectrum[i - 1:i + 2] + 1e-20)
    return f[i] + 0.5 * (a - c) / (a - 2 * b + c) * (f[1] - f[0])


def partial(x, rate, hz, width, every):
    t = np.arange(len(x)) / rate
    z = x * np.exp(-2j * np.pi * hz * t)
    sos = butter(4, width / (rate / 2), output="sos")
    z = sosfiltfilt(sos, z.real) + 1j * sosfiltfilt(sos, z.imag)
    edge = int(0.15 * rate)
    return np.abs(z[edge:-edge:int(rate / every)])


def single(x, rate, note):
    f0 = pitch(x, rate, 440 * 2 ** ((note - 69) / 12))
    levels = []
    for k in range(1, 17):
        if k * f0 > 0.45 * rate:
            break
        levels.append(20 * np.log10(partial(x, rate, k * f0, min(40.0, f0 / 4), 200) + 1e-12))
    levels = np.array(levels)
    median = np.median(levels, axis=1)
    n = min(12, len(median))
    k = np.arange(2, n + 1)
    line = np.vstack([np.ones(n - 1), np.log2(k)]).T
    jag = (median[1:n] - line @ np.linalg.lstsq(line, median[1:n], rcond=None)[0]).std()
    rel = (levels[1:8] - levels[0]).std(axis=1)
    moved = levels - levels.mean(axis=1, keepdims=True)
    cor = np.mean([np.corrcoef(moved[0], moved[i])[0, 1] for i in range(1, min(8, len(moved)))])
    return jag, rel, cor


def pair(x, rate, note):
    f0 = pitch(x, rate, 440 * 2 ** ((note - 69) / 12))
    depths = []
    for k in (1, 2, 3, 4):
        e = partial(x, rate, k * f0, min(60.0, f0 / 3), 400)
        high, low = np.percentile(e, 98), np.percentile(e, 2)
        depths.append((high - low) / (high + low))
    return depths


def main(argv):
    if len(argv) < 4 or argv[1] not in ("single", "pair") or len(argv) % 2:
        sys.exit(__doc__)
    jags, rels, cors, depths, spreads = [], [], [], [], []
    for note, path in zip(argv[2::2], argv[3::2]):
        x, rate = load(path)
        x = steady(x, rate)
        if len(x) < 1.5 * rate:
            print(f"{path}: under 1.5 s of steady note, left out")
            continue
        if argv[1] == "single":
            jag, rel, cor = single(x, rate, int(note))
            print(f"{path}: jag {jag:.1f} dB, rel {np.median(rel):.2f} dB, cor {cor:.2f}")
            jags.append(jag)
            rels += list(rel)
            cors.append(cor)
        else:
            d = pair(x, rate, int(note))
            print(f"{path}: depth " + " ".join(f"{v:.2f}" for v in d) + f", spread {np.std(d):.3f}")
            depths += d
            spreads.append(np.std(d))
    if jags:
        print(f"jag {np.median(jags):.1f} ({min(jags):.1f}-{max(jags):.1f}) dB, rel {np.median(rels):.2f} dB "
              f"({np.percentile(rels, 10):.2f}-{np.percentile(rels, 90):.2f}), cor {np.median(cors):.2f}")
    if depths:
        print(f"depth {np.median(depths):.2f} ({min(depths):.2f}-{max(depths):.2f}), spread {np.mean(spreads):.3f}")


if __name__ == "__main__":
    main(sys.argv)
