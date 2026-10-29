#!/usr/bin/env python3
"""Compare Tango Bandoneon's tone and breath with a bandoneon's: the preset's
Grille, and the README's known gap "The treble breathes more above 4 kHz".

The reference: BANDONEON v1 by Miguel Lopez, a free Decent Sampler library
on Pianobook, as in Tools/measure_bandoneon.py. Pianobook's terms do not
allow giving the samples away, so they are not in this repository; only the
numbers measured from them are. --bandoneon is the library's folder (the one
holding "samples b2").

    python3 -I Tools/compare_bandoneon_tone.py --bandoneon <dir> --render build-dsp/RenderRespira

Protocol. The library's legato takes C2 to C6 (Sband <note>.wav, the left
hand's Sband <note> izq.wav) each sound the note and a reed an octave above
it, 12 cents sharp. Tango Bandoneon sounds a 16' and an 8' reed, so Respira
plays each note's key an octave up with the whole keyboard treble
(Keyboard = 1) and the Mechanics at -40 dB, held from 0.3 to 7.9 s while the
14-bit bellows wheel rises along 0.02 + 0.01 t + 0.011 t^2. A take's frames
within 3 dB of its loudest (400 ms Blackman-Harris frames every 100 ms) are
averaged into one power spectrum. In each octave band from 250 Hz, the
harmonics' density is the mean over the band's harmonics of the mean within
1.5 % of each (at least 4 bins, at most a quarter of the note); the floor's
is the mean over the same harmonics of the mean within 0.15 of the note of
the point half a note above each. Printed, as Respira's excess over the
library's in dB:

- the harmonics at 0.5-1, 2-4, 4-8 and 8-16 kHz against 1-2 kHz, averaged
  over the notes, for each Grille setting, and the RMS of the three means
  from 2 kHz;
- the floor at 4-8 and 8-16 kHz against the harmonics at 1-2 kHz, for G4
  and A5, with the preset's Grille; a build without the breath shows how
  much of it is the breath;
- the floor over the harmonics in each octave band, for each note, which no
  linear filter after the reeds changes.
"""
import argparse
from pathlib import Path
import struct
import subprocess
import tempfile
import warnings

import numpy as np
from scipy.io import wavfile
from scipy.signal.windows import blackmanharris

TANGO = 4
GRILLE = 722   # Tango Bandoneon's (RespiraPresets.h)
GRILLES = (500, 722, 1000, 1500, 2000, 3000)
EDGES = (250, 500, 1000, 2000, 4000, 8000, 16000)
TAKES = {36: "Sband C izq.wav", 43: "Sband G izq.wav", 50: "Sband D izq.wav", 57: "Sband A izq.wav",
         60: "Sband C .wav", 67: "Sband G.wav", 74: "Sband D.wav", 81: "Sband A.wav", 84: "Sband C3.wav"}
SHARP_CENTS = 12.0

# The library's WAVs carry chunks scipy does not read; the audio is unaffected.
warnings.filterwarnings("ignore", category=wavfile.WavFileWarning)


def densities(path, f0):
    """The harmonics' and the floor's densities per octave band from 250 Hz, in dB."""
    rate, x = wavfile.read(path)
    x = x.astype(np.float64)
    x = x.mean(axis=1) if x.ndim > 1 else x
    width, hop = int(0.4 * rate), int(0.1 * rate)
    starts = range(0, len(x) - width, hop)
    level = np.array([np.mean(x[s:s + width] ** 2) for s in starts])
    window = blackmanharris(width)
    spectrum = np.mean([np.abs(np.fft.rfft(x[s:s + width] * window)) ** 2
                        for s, p in zip(starts, level) if p >= level.max() / 2], axis=0)
    f = np.fft.rfftfreq(width, 1.0 / rate)
    harmonics, floor = [], []
    for lo, hi in zip(EDGES[:-1], EDGES[1:]):
        ks = [k for k in range(1, int(20000 / f0)) if lo <= k * f0 < hi]
        h = [spectrum[np.abs(f - k * f0) < min(max(0.015 * k * f0, 4 * rate / width), 0.25 * f0)].mean() for k in ks]
        n = [spectrum[np.abs(f - (k + 0.5) * f0) < 0.15 * f0].mean() for k in ks]
        harmonics.append(np.mean(h) if ks else np.nan)
        floor.append(np.mean(n) if ks else np.nan)
    return 10 * np.log10(harmonics), 10 * np.log10(floor)


def held_note(key, path):
    """960 ticks a quarter at 120 bpm: the key from 0.3 to 7.9 s, the wheel rising."""
    def var(n):
        out = [n & 0x7F]
        n >>= 7
        while n:
            out.append(0x80 | (n & 0x7F))
            n >>= 7
        return bytes(reversed(out))
    events = []
    for i in range(801):
        t = 0.01 * i
        v = round((0.02 + 0.01 * t + 0.011 * t * t) * 16383)
        events += [(round(1920 * t), bytes([0xB0, 1, v >> 7])), (round(1920 * t), bytes([0xB0, 33, v & 0x7F]))]
    events += [(round(1920 * 0.3), bytes([0x90, key, 90])), (round(1920 * 7.9), bytes([0x80, key, 0]))]
    data, last = b"\x00\xFF\x51\x03\x07\xA1\x20", 0
    for t, m in sorted(events, key=lambda e: e[0]):
        data += var(t - last) + m
        last = t
    data += b"\x00\xFF\x2F\x00"
    path.write_bytes(b"MThd" + struct.pack(">IHHH", 6, 0, 1, 960) + b"MTrk" + struct.pack(">I", len(data)) + data)


def respira(render, note, grille, scratch):
    mid, wav = scratch / f"{note}.mid", scratch / f"{note}.wav"
    held_note(note + 12, mid)
    subprocess.run([render, str(TANGO), str(mid), str(wav), "--set", "keyboard=1", "--set", "mechanics=-40",
                    "--set", f"grille={grille}"], check=True, capture_output=True)
    return densities(wav, 440.0 * 2 ** ((note - 69) / 12))


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--bandoneon", required=True)
    parser.add_argument("--render", required=True, help="the RenderRespira executable")
    args = parser.parse_args()
    folder = Path(args.bandoneon) / "samples b2"
    library = {n: densities(folder / name, 440.0 * 2 ** ((n - 69) / 12 + SHARP_CENTS / 1200)) for n, name in TAKES.items()}
    tone = lambda h: np.delete(h[1:] - h[2], 1)   # 0.5-1, 2-4, 4-8, 8-16 kHz against 1-2 kHz
    print("Harmonics at 0.5-1, 2-4, 4-8 and 8-16 kHz against 1-2 kHz, Tango Bandoneon minus the bandoneon, mean over notes")
    with tempfile.TemporaryDirectory() as scratch:
        for grille in GRILLES:
            ours = {n: respira(args.render, n, grille, Path(scratch)) for n in TAKES}
            mean = np.nanmean([tone(ours[n][0]) - tone(library[n][0]) for n in TAKES], axis=0)
            print(f"grille {grille:5d} Hz: " + " ".join(f"{v:5.1f}" for v in mean) +
                  f" | RMS from 2 kHz {np.sqrt(np.mean(mean[1:] ** 2)):.1f}")
            if grille == GRILLE:
                preset = ours
    print("Floor at 4-8 and 8-16 kHz against the harmonics at 1-2 kHz, minus the bandoneon's:")
    for n, name in ((67, "G4"), (81, "A5")):
        (harmonics, floor), (lib_harmonics, lib_floor) = preset[n], library[n]
        excess = (floor[4:] - harmonics[2]) - (lib_floor[4:] - lib_harmonics[2])
        print(f"  {name}: " + " ".join(f"{v:5.1f}" for v in excess))
    print("Floor over the harmonics in each octave band from 250 Hz, minus the bandoneon's:")
    for n in TAKES:
        excess = (preset[n][1] - preset[n][0]) - (library[n][1] - library[n][0])
        print(f"  {n}: " + " ".join("  n/a" if np.isnan(v) else f"{v:5.1f}" for v in excess))


if __name__ == "__main__":
    main()
