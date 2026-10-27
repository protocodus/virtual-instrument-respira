#!/usr/bin/env python3
"""Find how far the hand travels in each bellows stroke of the demo phrases.

Each stroke in Tools/write_preset_midi.py names the bellows pressure a player
would be after. This plays each phrase through the engine (RenderRespira, a
research renderer with a bellows trace), measures the pressure each stroke
made over its middle 60 % (its peak, for a shake stroke) and scales that
stroke's travel by sqrt(target / measured), as a player listening to the
instrument would, for a fixed number of passes. Then it rewrites the TRAVELS
block of write_preset_midi.py. Re-run it after an engine change only when the
demo phrases are meant to change too, then run write_preset_midi.py:

    python3 Tools/fit_bellows.py <RenderRespira> [slug ...]
"""
import csv
import math
from pathlib import Path
import re
import subprocess
import sys
import tempfile

sys.path.insert(0, str(Path(__file__).resolve().parent))
import write_preset_midi as phrases  # noqa: E402

PASSES = 8


def stroke_pressures(trace, phrase):
    rows = list(csv.DictReader(open(trace)))
    beat = 60.0 / phrase.bpm
    result = []
    for s in phrase.bellows.strokes:
        a, b = s.b0 * beat, s.b1 * beat
        if not s.peak:
            a, b = a + 0.2 * (b - a), a + 0.8 * (b - a)
        values = [abs(float(r["pressure_pa"])) for r in rows if a <= float(r["seconds"]) <= b and int(r["cells"]) > 0]
        result.append((max(values) if s.peak else sum(values) / len(values)) if values else 0.0)
    return result


def fit(renderer, index, slug, build, workdir):
    phrase = build()
    beat = 60.0 / phrase.bpm
    # First guess: a twentieth of the wheel per second of stroke at 400 Pa.
    travels = [0.05 * (s.b1 - s.b0) * beat * math.sqrt(s.target / 400.0) for s in phrase.bellows.strokes]
    mid, wav, trace = (str(Path(workdir) / f"{slug}{ext}") for ext in (".mid", ".wav", ".csv"))
    for p in range(PASSES + 1):
        travels = [round(t, 4) for t in travels]
        Path(mid).write_bytes(phrase.smf(slug, travels))
        out = subprocess.run([renderer, str(index), mid, wav, trace], check=True, capture_output=True, text=True)
        got = stroke_pressures(trace, phrase)
        if p == PASSES:
            break
        travels = [t * min(2.5, max(0.5, math.sqrt(s.target / g) if g > 1 else 2.5))
                   for t, s, g in zip(travels, phrase.bellows.strokes, got)]
    at, start, end = phrase.bellows.path(phrase.bpm, travels)
    positions = [at(start + k * 0.005) for k in range(int((end - start) / 0.005) + 1)]
    print(out.stdout.strip())
    print("   target -> got Pa:", ", ".join(f"{s.target:.0f}->{g:.0f}" for s, g in zip(phrase.bellows.strokes, got)))
    print(f"   wheel range {min(positions):.2f}..{max(positions):.2f}"
          + ("  ** OUT OF RANGE **" if min(positions) < 0 or max(positions) > 1 else ""))
    return travels


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    renderer, only = sys.argv[1], sys.argv[2:]
    travels = dict(phrases.TRAVELS)
    with tempfile.TemporaryDirectory() as workdir:
        for index, (slug, build) in enumerate(phrases.PRESETS):
            if not only or slug in only:
                travels[slug] = fit(renderer, index, slug, build, workdir)
    block = "TRAVELS = {\n" + "".join(
        f'    "{slug}": [{", ".join(f"{t:.4f}" for t in travels[slug])}],\n'
        for slug, _ in phrases.PRESETS if slug in travels) + "}\n"
    source = Path(phrases.__file__)
    text = source.read_text(encoding="utf-8")
    text = re.sub(r"(# BEGIN TRAVELS\n).*?(# END TRAVELS)", lambda m: m.group(1) + block + m.group(2), text, flags=re.S)
    source.write_text(text, encoding="utf-8")
    print(f"wrote TRAVELS in {source.name}")


if __name__ == "__main__":
    main()
