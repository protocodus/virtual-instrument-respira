#!/usr/bin/env python3
"""Write the committed per-preset demo phrases, Docs/audio/midi/<NN>-<slug>.mid.

One phrase per factory preset (README G3a), each written for what its register
is played for: long tones shaped by the bellows on the single clarinet reed, a
low line on the bassoon reed, a folk tune and a Paris waltz on the tremolo
registers, a marcato tango, a chorale on the organ register, a quick tune on
the oboe and a bellows shake on the full register. Every phrase is original.
A legato line overlaps its keys by KEY_OVERLAP, as a player's legato does.
The .mid files are the committed sources the demo renderer reads; this script
only records how they were written. Re-run it only to change a phrase on
purpose, and commit the changed .mid files on their own:

    python3 Tools/write_preset_midi.py           # write Docs/audio/midi
    python3 Tools/write_preset_midi.py --check   # the committed files match

Every file is format 0 on MIDI channel 1 at 480 PPQ, with a tempo and a track
name, laid out for Respira's split keyboard (its factory setting): C1..B1 are
the bass buttons, C2..B2 the chord buttons, C3 and up the treble. Its last
event falls 5.5-9.5 s in, so with the renderer's 2.5 s tail every demo is
8-12 s long (G3c).

The bellows is the mod wheel (CC1 + CC33, 14 bits, every 5 ms): each phrase
has a list of strokes, each with the bellows pressure a player would be after.
How far the hand travels in each stroke is frozen in TRAVELS, which
Tools/fit_bellows.py finds by playing the phrase through the engine; this
script only reads it, so the files do not change when the engine does.
"""
import argparse
import math
from pathlib import Path
import re
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
MIDI_DIR = ROOT / "Docs/audio/midi"
BANK = ROOT / "Source/DSP/RespiraPresets.h"

PPQ = 480
WHEEL_STEP = 0.005   # seconds between hand positions
# How long a legato line's next key goes down before the last comes up: a
# player's habit, not a measurement. A reed falls silent as its pallet closes
# (8 ms) and the next takes tens of milliseconds to speak, so keys that only
# meet leave a gap 25-40 dB deep at every note.
KEY_OVERLAP = 0.03
NAMES = {"C": 0, "D": 2, "E": 4, "F": 5, "G": 7, "A": 9, "B": 11}
BASS_BUTTONS, CHORD_BUTTONS, TREBLE = 24, 36, 48   # C1, C2, C3 on the split keyboard


def n(name):
    """'C4' is MIDI 60; sharps as 'F#3', flats as 'Bb2'."""
    pitch = NAMES[name[0]]
    rest = name[1:]
    if rest[0] == "#":
        pitch, rest = pitch + 1, rest[1:]
    elif rest[0] == "b":
        pitch, rest = pitch - 1, rest[1:]
    return 12 * (int(rest) + 1) + pitch


class Stroke:
    """One bellows stroke from beat b0 to b1, after `target` Pa (its mean, or its
    peak for a short shake stroke). shape = (start, end) relative hand speed, so
    a stroke can swell or fade; accents = beats of short speed bursts."""

    def __init__(self, b0, b1, target, shape=(1.0, 1.0), accents=(), accent=0.0, peak=False):
        self.b0, self.b1 = b0, b1
        self.target = target
        self.shape = shape
        self.accents = accents
        self.accent = accent
        self.peak = peak


class Bellows:
    """The hand's path through alternating strokes, opening (drawing) first
    unless first="push". A stroke speeds up after a reversal, slows into the
    next and is a little uneven between; travels are wheel ranges per stroke,
    and the path is centred on the wheel."""

    def __init__(self, strokes, ease=0.10, wobble=0.06, first="pull", seed=1):
        self.strokes = strokes
        self.ease, self.wobble, self.first, self.seed = ease, wobble, first, seed

    def path(self, bpm, travels):
        """A function seconds -> hand position, and its start and end in seconds."""
        beat = 60.0 / bpm
        sign = 1 if self.first == "pull" else -1
        pos = [0.0]
        for i, travel in enumerate(travels):
            pos.append(pos[-1] + (sign if i % 2 == 0 else -sign) * travel)
        offset = 0.5 - 0.5 * (min(pos) + max(pos))
        pos = [p + offset for p in pos]
        curves = [self._curve(s, i, beat) for i, s in enumerate(self.strokes)]
        t0s = [s.b0 * beat for s in self.strokes]
        t1s = [s.b1 * beat for s in self.strokes]

        def at(t):
            for i in range(len(self.strokes)):
                if t <= t1s[i] or i == len(self.strokes) - 1:
                    if t <= t0s[i]:
                        return pos[i]
                    u = min(1.0, (t - t0s[i]) / (t1s[i] - t0s[i]))
                    cum = curves[i]
                    k = u * (len(cum) - 1)
                    j = min(int(k), len(cum) - 2)
                    frac = cum[j] + (cum[j + 1] - cum[j]) * (k - j)
                    return pos[i] + (pos[i + 1] - pos[i]) * frac
            return pos[-1]

        return at, t0s[0], t1s[-1]

    def _curve(self, s, i, beat, grid=2000):
        """Cumulative share of the stroke's travel, on `grid` steps."""
        t0, t1 = s.b0 * beat, s.b1 * beat
        e = min(self.ease / (t1 - t0), 0.45)
        speeds = []
        for k in range(grid):
            u = (k + 0.5) / grid
            ramp = 1.0
            if u < e:
                ramp = 0.5 - 0.5 * math.cos(math.pi * u / e)
            elif u > 1 - e:
                ramp = 0.5 - 0.5 * math.cos(math.pi * (1 - u) / e)
            t = t0 + u * (t1 - t0)
            shape = s.shape[0] + (s.shape[1] - s.shape[0]) * u
            human = 1.0 + self.wobble * math.sin(2 * math.pi * (0.83 * t + 0.31 * self.seed + 0.17 * i))
            burst = 1.0 + sum(s.accent * math.exp(-0.5 * ((t - a * beat) / 0.035) ** 2) for a in s.accents)
            speeds.append(ramp * shape * human * burst)
        total, acc, cum = sum(speeds), 0.0, [0.0]
        for v in speeds:
            acc += v / total
            cum.append(acc)
        return cum


class Phrase:
    def __init__(self, bpm, bellows):
        self.bpm = bpm
        self.bellows = bellows
        self.events = []  # (tick, order, bytes); note-offs sort before note-ons

    def tick(self, beat):
        return round(beat * PPQ)

    def note(self, beat, length, pitch, velocity=96):
        pitch = n(pitch) if isinstance(pitch, str) else pitch
        self.events.append((self.tick(beat), 2, bytes([0x90, pitch, velocity])))
        self.events.append((self.tick(beat + length), 1, bytes([0x80, pitch, 64])))

    def treble(self, beat, length, pitches, velocity=96):
        for pitch in [pitches] if isinstance(pitches, str) else pitches:
            if n(pitch) < TREBLE:
                raise ValueError(f"{pitch} is below the treble keys")
            self.note(beat, length, pitch, velocity)

    def line(self, beat, notes, velocity=96, legato=0.95):
        """(pitch, beats) pairs from `beat`; a None pitch rests."""
        for pitch, length in notes:
            if pitch is not None:
                self.treble(beat, length * legato, pitch, velocity)
            beat += length
        return beat

    def slur(self, beat, notes, velocity=96):
        """A legato line of (pitch, beats) pairs from `beat`: each key goes down
        KEY_OVERLAP before the last comes up, and the last is held its full
        length."""
        overlap = KEY_OVERLAP * self.bpm / 60.0
        for i, (pitch, length) in enumerate(notes):
            following = notes[i + 1][0] if i + 1 < len(notes) else None
            if following == pitch:
                raise ValueError(f"{pitch} repeats in a slur; a key cannot overlap itself")
            self.treble(beat, length + (overlap if following else 0.0), pitch, velocity)
            beat += length
        return beat

    def bass(self, beat, length, root, velocity=90):
        """A bass button: the pitch class of `root` ('A', 'F#')."""
        self.note(beat, length, BASS_BUTTONS + n(root + "0") % 12, velocity)

    def chord(self, beat, length, notes, velocity=86):
        """Chord buttons, one pitch class each, so any chord can be played."""
        for root in notes:
            self.note(beat, length, CHORD_BUTTONS + n(root + "0") % 12, velocity)

    def smf(self, name, travels):
        at, start, end = self.bellows.path(self.bpm, travels)
        ticks_per_second = PPQ * self.bpm / 60.0
        step = round(WHEEL_STEP * ticks_per_second)
        events = list(self.events)
        last = None
        for tick in range(round(start * ticks_per_second), round(end * ticks_per_second) + 1, step):
            value = round(min(1.0, max(0.0, at(tick / ticks_per_second))) * 16383)
            if value != last:
                events.append((tick, 0, bytes([0xB0, 1, value >> 7])))
                events.append((tick, 0, bytes([0xB0, 33, value & 0x7F])))
            last = value

        def vlq(value):
            out = [value & 0x7F]
            value >>= 7
            while value:
                out.append(0x80 | (value & 0x7F))
                value >>= 7
            return bytes(reversed(out))

        tempo = round(60_000_000 / self.bpm)
        track = bytearray()
        track += b"\x00\xff\x03" + vlq(len(name)) + name.encode("ascii")
        track += b"\x00\xff\x51\x03" + tempo.to_bytes(3, "big")
        previous = 0
        for tick, _, message in sorted(events, key=lambda e: (e[0], e[1])):
            track += vlq(tick - previous) + message
            previous = tick
        track += b"\x00\xff\x2f\x00"
        return (b"MThd" + struct.pack(">IHHH", 6, 0, 1, PPQ)
                + b"MTrk" + struct.pack(">I", len(track)) + track)

    def seconds(self):
        return max(tick for tick, _, _ in self.events) / PPQ * 60 / self.bpm


def clarinet_solo():
    # Long tones on the one clarinet reed, shaped only by the bellows: a swell
    # on the first draw, a reversal in the middle of the A, a loud push through
    # the turn to C and a press that fades the last note away.
    p = Phrase(80, Bellows([
        Stroke(0.0, 2.4, 450, shape=(0.45, 1.5)),
        Stroke(2.4, 6.1, 700),
        Stroke(6.1, 8.4, 350),
        Stroke(8.4, 11.6, 250, shape=(1.4, 0.35)),
    ], ease=0.10))
    p.slur(0, [("A4", 4), ("C5", 1.5), ("B4", 1.5), ("E4", 4.6)])
    return p


def bassoon_low():
    # A minor line on the 16' bassoon reed, an octave below the keys, over a
    # low bass-and-chord march; a draw for each of the first two bars.
    p = Phrase(96, Bellows([
        Stroke(0.0, 4.0, 420),
        Stroke(4.0, 8.0, 480),
        Stroke(8.0, 11.4, 300, shape=(1.2, 0.5)),
    ]))
    p.slur(0, [("A3", 1), ("C4", 0.5), ("B3", 0.5), ("A3", 1), ("E3", 1),
               ("F3", 1), ("E3", 0.5), ("D3", 0.5), ("E3", 1), ("G#3", 1), ("A3", 3)], velocity=100)
    for bar, (roots, chord) in enumerate([(("A", "E"), ("A", "C", "E")), (("D", "E"), ("E", "G#", "B"))]):
        for half, root in enumerate(roots):
            p.bass(4 * bar + 2 * half, 0.8, root)
            p.chord(4 * bar + 2 * half + 1, 0.45, chord if half or bar == 0 else ("D", "F", "A"))
    p.bass(8, 2.5, "A")
    p.chord(9, 1.8, ("A", "C", "E"))
    return p


def violin_folk():
    # A D minor folk tune in 3/4 on the violin register's two reeds, the
    # bellows turning every two bars.
    p = Phrase(116, Bellows([
        Stroke(0.0, 6.0, 330),
        Stroke(6.0, 12.0, 380),
        Stroke(12.0, 15.4, 260, shape=(1.2, 0.5)),
    ]))
    p.slur(0, [("D5", 1), ("A4", 1), ("F4", 1), ("G4", 1), ("A4", 0.5), ("Bb4", 0.5), ("A4", 1),
               ("D5", 1.5), ("C5", 0.5), ("Bb4", 1), ("A4", 1), ("G4", 1), ("E4", 1), ("D4", 3)])
    harmony = [("D", ("D", "F", "A")), ("D", ("D", "F", "A")), ("Bb", ("Bb", "D", "F")),
               ("A", ("A", "C#", "G")), ("D", ("D", "F", "A"))]
    for bar, (root, chord) in enumerate(harmony):
        p.bass(3 * bar, 0.8, root)
        if bar < 4:
            p.chord(3 * bar + 1, 0.45, chord)
            p.chord(3 * bar + 2, 0.45, chord)
        else:
            p.chord(3 * bar + 1, 1.8, chord)
    return p


def paris_musette():
    # A Paris waltz on the three-reed musette with oom-pah-pah on the bass
    # side; the bellows turns every two bars, at the phrase points.
    p = Phrase(176, Bellows([
        Stroke(0, 6, 380),
        Stroke(6, 12, 420),
        Stroke(12, 18, 480),
        Stroke(18, 24, 300, shape=(1.3, 0.6)),
    ], ease=0.08))
    p.slur(0, [("D5", 1), ("B4", .5), ("C5", .5), ("D5", 1), ("G5", 2), ("F#5", 1),
               ("E5", 1), ("C5", .5), ("D5", .5), ("E5", 1), ("A5", 2), ("G5", 1),
               ("F#5", 1), ("E5", .5), ("F#5", .5), ("G5", 1), ("A5", 1), ("F#5", 1),
               ("D5", 1), ("C6", 1), ("A5", 1), ("F#5", 1), ("G5", 3)])
    harmony = [("G", ("G", "B", "D")), ("D", ("G", "B", "D")), ("C", ("C", "E", "G")), ("G", ("C", "E", "G")),
               ("D", ("D", "F#", "C")), ("A", ("D", "F#", "C")), ("D", ("D", "F#", "C")), ("G", ("G", "B", "D"))]
    for bar, (root, chord) in enumerate(harmony):
        p.bass(3 * bar, 0.8, root)
        if bar < 7:
            p.chord(3 * bar + 1, 0.45, chord)
            p.chord(3 * bar + 2, 0.45, chord)
        else:
            p.chord(3 * bar + 1, 1.5, chord)
    return p


def tango_bandoneon():
    # A marcato tango on the bandoneon register: every chord gets a short dig
    # of the bellows on top of the stroke, then a run and a held tonic.
    accents = lambda beats: [b + 0.03 for b in beats]
    p = Phrase(116, Bellows([
        Stroke(0, 4, 450, accents=accents(range(4)), accent=2.5),
        Stroke(4, 8, 500, accents=accents(range(4, 8)), accent=2.5),
        Stroke(8, 12, 420),
        Stroke(12, 16.8, 600, accents=accents([14]), accent=3.0, shape=(1.0, 0.8)),
    ], ease=0.05))
    am, dm, e7 = ("A3", "C4", "E4", "A4"), ("A3", "D4", "F4", "A4"), ("G#3", "B3", "D4", "E4")
    for beat in range(4):
        p.treble(beat, 0.35, am, 110)
        p.bass(beat, 0.35, "A" if beat % 2 == 0 else "E")
    for beat, chord, root in ((4, dm, "D"), (5, dm, "A"), (6, e7, "E"), (7, e7, "B")):
        p.treble(beat, 0.35, chord, 110)
        p.bass(beat, 0.35, root)
    for beat, length, pitch in [(8, .5, "A4"), (8.5, .5, "B4"), (9, .5, "C5"), (9.5, .5, "D5"), (10, 1, "E5"),
                                (11, .5, "F5"), (11.5, .5, "E5"), (12, .5, "D#5"), (12.5, .5, "E5"), (13, 1, "G#4")]:
        p.treble(beat, length * 0.9, pitch, 100)
    for beat, root, chord in ((8, "A", ("A", "C", "E")), (10, "E", ("E", "G#", "D")), (12, "E", ("E", "G#", "D"))):
        p.bass(beat, 0.4, root)
        p.chord(beat + 1, 0.4, chord)
    p.treble(14, 2.0, am, 110)
    p.bass(14, 2.0, "A")
    return p


def organ_bellows():
    # A chorale on the organ register (16' and 4') over held bass and chords,
    # a reversal on every chord: the full bass side takes a lot of air.
    p = Phrase(80, Bellows([
        Stroke(0, 2, 340, shape=(0.8, 1.2)),
        Stroke(2, 4, 380),
        Stroke(4, 6, 420),
        Stroke(6, 8, 440),
        Stroke(8, 10.8, 220, shape=(1.4, 0.3)),
    ], ease=0.10))
    progression = [(("C4", "E4", "G4", "C5"), "C", ("C", "E", "G")), (("A3", "E4", "A4", "C5"), "A", ("A", "C", "E")),
                   (("A3", "C4", "F4", "A4"), "F", ("F", "A", "C")), (("G3", "D4", "F4", "B4"), "G", ("G", "B", "F")),
                   (("G3", "C4", "E4", "C5"), "C", ("C", "E", "G"))]
    for i, (treble, root, chord) in enumerate(progression):
        length = (2.0 if i < 4 else 2.6) * 0.97
        p.treble(2 * i, length, treble)
        p.bass(2 * i, length, root)
        p.chord(2 * i, length, chord)
    return p


def bright_oboe():
    # A quick tune in G on the oboe register (8' and 4'), detached notes over
    # an oom-pah; one bellows stroke a bar.
    p = Phrase(120, Bellows([
        Stroke(0, 4, 420),
        Stroke(4, 8, 450),
        Stroke(8, 12, 450),
        Stroke(12, 14.6, 300, shape=(1.2, 0.5)),
    ], ease=0.08))
    p.line(0, [("G4", .5), ("B4", .5), ("D5", .5), ("G5", .5), ("F#5", .25), ("G5", .25), ("A5", .5), ("G5", 1),
               ("E5", .5), ("C5", .5), ("E5", .5), ("G5", .5), ("F#5", .5), ("E5", .5), ("D5", 1),
               ("C5", .5), ("E5", .5), ("D5", .5), ("B4", .5), ("A4", .5), ("F#4", .5), ("G4", 1)], legato=0.7)
    p.treble(12, 2, ("G4", "B4", "D5"))
    for bar, (root, chord) in enumerate([("G", ("G", "B", "D")), ("C", ("C", "E", "G")), ("D", ("D", "F#", "A"))]):
        for half in range(2):
            p.bass(4 * bar + 2 * half, 0.6, root if half == 0 else {"G": "D", "C": "G", "D": "A"}[root])
            p.chord(4 * bar + 2 * half + 1, 0.4, chord)
    p.bass(12, 2, "G")
    p.chord(12, 2, ("G", "B", "D"))
    return p


def master_tutti():
    # Every treble reed over bass and chords: a bellows shake (a reversal on
    # each triplet eighth, which pulses the chord), then ordinary strokes.
    shake = [Stroke(i / 3, (i + 1) / 3, 800, peak=True) for i in range(12)]
    p = Phrase(100, Bellows(shake + [
        Stroke(4, 6, 450),
        Stroke(6, 7.9, 380),
        Stroke(7.9, 10.4, 300, shape=(1.3, 0.5)),
    ], ease=0.035, wobble=0.03))
    a_major, d_major = ("A3", "C#4", "E4", "A4"), ("A3", "D4", "F#4", "A4")
    for beat, length, treble, root, chord in ((0, 4, a_major, "A", ("A", "C#", "E")),
                                              (4, 2, d_major, "D", ("D", "F#", "A")),
                                              (6, 4, a_major, "A", ("A", "C#", "E"))):
        p.treble(beat, length, treble)
        p.bass(beat, length, root)
        p.chord(beat, length, chord)
    return p


PRESETS = [
    ("clarinet-solo", clarinet_solo), ("bassoon-low", bassoon_low), ("violin-folk", violin_folk),
    ("paris-musette", paris_musette), ("tango-bandoneon", tango_bandoneon), ("organ-bellows", organ_bellows),
    ("bright-oboe", bright_oboe), ("master-tutti", master_tutti),
]

# Hand travel of each stroke, in wheel ranges, from Tools/fit_bellows.py.
# BEGIN TRAVELS
TRAVELS = {
    "clarinet-solo": [0.2608, 0.4732, 0.2739, 0.1670],
    "bassoon-low": [0.1848, 0.1856, 0.1281],
    "violin-folk": [0.2104, 0.2294, 0.0914],
    "paris-musette": [0.1546, 0.1630, 0.1840, 0.1220],
    "tango-bandoneon": [0.1123, 0.1211, 0.1700, 0.2972],
    "organ-bellows": [0.1188, 0.1269, 0.1392, 0.1425, 0.1127],
    "bright-oboe": [0.1451, 0.1544, 0.1662, 0.0726],
    "master-tutti": [0.0167, 0.0169, 0.0236, 0.0264, 0.0276, 0.0279, 0.0294, 0.0292, 0.0299, 0.0301, 0.0308, 0.0301, 0.1286, 0.0992, 0.1023],
}
# END TRAVELS


def phrase_file(slug, build, travels):
    phrase = build()
    if len(travels) != len(phrase.bellows.strokes):
        raise SystemExit(f"{slug}: {len(travels)} travels for {len(phrase.bellows.strokes)} strokes; "
                         "run Tools/fit_bellows.py")
    data = phrase.smf(slug, travels)
    return phrase, data


def files():
    bank = [re.sub(r"[^a-z0-9]+", "-", name.lower()).strip("-")
            for name in re.findall(r'\{ "([^"]+)",', BANK.read_text(encoding="utf-8"))]
    if [slug for slug, _ in PRESETS] != bank:
        sys.exit(f"phrases {[slug for slug, _ in PRESETS]} do not match the bank {bank}")
    result = {}
    for index, (slug, build) in enumerate(PRESETS, 1):
        phrase, data = phrase_file(slug, build, TRAVELS.get(slug, []))
        end = max(phrase.seconds(), phrase.bellows.strokes[-1].b1 * 60 / phrase.bpm)
        if not 5.5 <= end <= 9.5:
            raise SystemExit(f"{slug}: {end:.1f} s is outside the 5.5-9.5 s phrase length")
        result[f"{index:02d}-{slug}.mid"] = data
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--check", action="store_true", help="fail unless the committed files match")
    args = parser.parse_args()
    expected = files()
    found = {path.name: path.read_bytes() for path in MIDI_DIR.glob("*.mid")}
    if args.check:
        stale = sorted(set(found) ^ set(expected) | {name for name in expected if found.get(name) != expected[name]})
        if stale:
            sys.exit(f"Docs/audio/midi differs from {Path(__file__).name} in: {', '.join(stale)}")
        print(f"{len(expected)} demo phrases match {Path(__file__).name}")
        return
    MIDI_DIR.mkdir(parents=True, exist_ok=True)
    for name in set(found) - set(expected):
        (MIDI_DIR / name).unlink()
    for name, data in expected.items():
        (MIDI_DIR / name).write_bytes(data)
        print(name)


if __name__ == "__main__":
    main()
