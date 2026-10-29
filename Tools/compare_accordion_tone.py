#!/usr/bin/env python3
"""Compare the tone of Respira's held notes with two accordion sample
libraries, and with each Grille setting, to fit the Grille's default.

The references, neither in this repository (only numbers measured from them
are):

- La Melodiosa by Petri Pohjanmies (2026, CC BY): two treble ranks two
  octaves apart. Register I sounds as its file names say, register II two
  octaves under them; each note is held in two takes (ba, va), its tail cut
  to digital silence. --melodiosa is the folder holding I-*.wav and II-*.wav.
- An old Hohner by Marek Kintosh (Pianobook, 2024): two reeds a note
  (musette), recorded with an MXL 2010 at home, then pitch-corrected and
  normalized, so only its tone is used. --hohner is its Samples folder; the
  treble takes are *_<MIDI note>_single_KLV.wav.

    python3 -I Tools/compare_accordion_tone.py --melodiosa <dir> --hohner <dir> --render build-dsp/RenderRespira

Protocol. Each take from 1.0 s to 1.0 s before its end (its last sample
above 1e-4 of full scale): the Welch power spectrum (100 ms Hann frames,
half overlapping), its power in the octave bands 0.5-1, 2-4, 4-8 and
8-16 kHz in dB against the 1-2 kHz band, a library's takes of one sounding
note averaged in dB. Respira plays Clarinet Solo (one 8' reed) through
RenderRespira: the note held from 0.3 s while the bellows wheel travels 18
of its 127 steps over 3 s (about 150-420 Pa), measured from 1.0 to 3.0 s,
both channels averaged; C3 to A5 by minor thirds, against every library
note there. The Hohner starts at C4, and its G5 is left out: its 1-2 kHz
band holds only a weak 2nd harmonic and sits 20 dB under its neighbours'.
For each Grille setting the tool prints Respira's mean excess over each
library in each band, and the RMS of those eight means.
"""
import argparse
from pathlib import Path
import re
import struct
import subprocess
import tempfile
import warnings

import numpy as np
from scipy.io import wavfile
from scipy.signal import welch

EDGES = (500, 1000, 2000, 4000, 8000, 16000)
KEYS = range(48, 82, 3)
GRILLES = (16000, 9000, 4000, 2000, 1500, 1200, 1000, 800, 600, 500)
NAMES = {"C": 0, "C#": 1, "D": 2, "D#": 3, "E": 4, "F": 5, "F#": 6, "G": 7, "G#": 8, "A": 9, "A#": 10, "B": 11}

# The libraries' WAVs carry chunks scipy does not read; the audio is unaffected.
warnings.filterwarnings("ignore", category=wavfile.WavFileWarning)


def bands(x, rate):
    """Octave band powers 0.5-1, 2-4, 4-8, 8-16 kHz in dB against 1-2 kHz."""
    f, p = welch(x, rate, nperseg=int(0.1 * rate))
    b = np.array([10 * np.log10(p[(f >= lo) & (f < hi)].sum()) for lo, hi in zip(EDGES[:-1], EDGES[1:])])
    return np.delete(b - b[1], 1)


def take(path):
    rate, x = wavfile.read(path)
    x = x.astype(np.float64) / (np.iinfo(x.dtype).max + 1.0 if x.dtype.kind == "i" else 1.0)
    x = x.mean(axis=1) if x.ndim > 1 else x
    end = np.nonzero(np.abs(x) > 1e-4)[0][-1]
    return bands(x[int(rate): end - int(rate)], rate)


def melodiosa(folder):
    notes = {}
    for p in Path(folder).glob("*.wav"):
        m = re.fullmatch(r"(I|II)-([A-G]#?)(\d)-(ba|va)\.wav", p.name)
        if m:
            key = 12 * (int(m.group(3)) + 1) + NAMES[m.group(2)] - (24 if m.group(1) == "II" else 0)
            notes.setdefault(key, []).append(take(p))
    return {k: np.mean(v, axis=0) for k, v in notes.items()}


def hohner(folder):
    notes = {}
    for p in Path(folder).glob("*_single_KLV.wav"):
        key = int(re.search(r"_(\d+)_single_KLV", p.name).group(1))
        if key != 79:
            notes[key] = take(p)
    return notes


def held_note(key, path):
    """Format 0, 960 ticks a second: the note from 0.3 s, the wheel 0 -> 18 over 3 s."""
    def var(n):
        out = [n & 0x7F]
        n >>= 7
        while n:
            out.append(0x80 | (n & 0x7F))
            n >>= 7
        return bytes(reversed(out))
    events = [(0, bytes([0xB0, 1, 0])), (288, bytes([0x90, key, 90]))]
    events += [(288 + int(9.6 * i), bytes([0xB0, 1, round(18 * i / 300)])) for i in range(301)]
    events.append((288 + 2880, bytes([0x80, key, 0])))
    data, last = b"\x00\xFF\x51\x03\x07\xA1\x20", 0
    for t, m in sorted(events, key=lambda e: e[0]):
        data += var(t - last) + m
        last = t
    data += b"\x00\xFF\x2F\x00"
    path.write_bytes(b"MThd" + struct.pack(">IHHH", 6, 0, 1, 480) + b"MTrk" + struct.pack(">I", len(data)) + data)


def respira(render, grille, scratch):
    notes = {}
    for key in KEYS:
        mid, wav = scratch / f"{key}.mid", scratch / f"{key}.wav"
        held_note(key, mid)
        subprocess.run([render, "0", str(mid), str(wav), "--set", f"grille={grille}"], check=True, capture_output=True)
        rate, x = wavfile.read(wav)
        notes[key] = bands(x.astype(np.float64).mean(axis=1)[rate: 3 * rate], rate)
    return notes


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--melodiosa", required=True)
    parser.add_argument("--hohner", required=True)
    parser.add_argument("--render", required=True, help="the RenderRespira executable")
    args = parser.parse_args()
    libraries = {"La Melodiosa": melodiosa(args.melodiosa), "Hohner": hohner(args.hohner)}
    print("Respira (Clarinet Solo) minus each library: the mean over shared notes of each octave band, 0.5-1, 2-4,")
    print("4-8 and 8-16 kHz, in dB against 1-2 kHz, and the RMS of all eight")
    with tempfile.TemporaryDirectory() as scratch:
        for grille in GRILLES:
            ours = respira(args.render, grille, Path(scratch))
            means = [np.mean([ours[k] - lib[k] for k in ours if k in lib], axis=0) for lib in libraries.values()]
            cells = " | ".join(f"{name} " + " ".join(f"{v:5.1f}" for v in m) for name, m in zip(libraries, means))
            print(f"grille {grille:5d} Hz: {cells} | RMS {np.sqrt(np.mean(np.square(means))):.1f}")

if __name__ == "__main__":
    main()
