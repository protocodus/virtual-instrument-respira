#!/usr/bin/env python3
"""Relative onset measurements; no SPL, pressure or MIDI calibration is inferred.

Single reeds from La Melodiosa, plus non-loop Hohner takes. These edited sample
files have unknown key and bellows gestures. Censored onsets/noisy thresholds
are explicitly marked. Requires NumPy and SciPy. Example:

  python3 Tools/measure_accordion_attacks.py --melodiosa /path/LAMelodiosa \
      --hohner /path/Accordion.dsbundle/Samples --output /tmp/attacks.json

A centered Hann tracker uses 2/4/8 periods at the
strongest channel's approximate fundamental, with 1 ms sampling. Reference
amplitude is the largest tracked fundamental in the first two seconds.
"""
from pathlib import Path
import argparse
import hashlib
import json
import re
import warnings
import numpy as np
from scipy.io import wavfile
from scipy.signal import fftconvolve

names = {'C': 0, 'D': 2, 'E': 4, 'F': 5, 'G': 7, 'A': 9, 'B': 11}


def pitch(name):
    m = re.fullmatch(r'([A-G])(#?)(\d)', name)
    return 12 * (int(m[3]) + 1) + names[m[1]] + bool(m[2])


def envelope(x, rate, hz, cycles):
    radius = round(cycles * rate / hz / 2)
    n = np.arange(-radius, radius + 1)
    w = .5 + .5 * np.cos(np.pi * n / radius)
    kernel = w * np.exp(2j * np.pi * hz * n / rate)
    a = 2 * np.abs(fftconvolve(x, kernel, mode='same')) / w.sum()
    hop = round(rate * .001)
    return np.arange(0, len(x), hop) / rate, a[::hop]


def crossing(t, a, fraction):
    indices = np.flatnonzero(a >= fraction)
    if not len(indices):
        return None
    j = indices[0]
    if j == 0:
        return float(t[0])
    lo, hi = np.log(np.maximum(a[j-1:j+1], 1e-30))
    return float(t[j-1] + (t[j]-t[j-1]) * (np.log(fraction)-lo)/(hi-lo))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--melodiosa', type=Path, help='La Melodiosa LAMelodiosa directory')
    parser.add_argument('--hohner', type=Path, help='Marek Kintosh Hohner Samples directory')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if not args.melodiosa and not args.hohner:
        parser.error('specify at least one sample library')
    for directory in [args.melodiosa, args.hohner]:
        if directory is not None and not directory.is_dir():
            parser.error(f'not a directory: {directory}')
    files = []
    for reg, shift in [('I', 0), ('II', -24)]:
        for p in sorted(args.melodiosa.glob(reg + '-*.wav') if args.melodiosa else []):
            token = p.stem.split('-')[1]
            try:
                key = pitch(token) + shift
            except (TypeError, ValueError):
                continue
            files.append((p, key, 'La Melodiosa', 'Unknown articulation and pressure; two round-robin takes.'))
    for p in sorted(args.hohner.glob('*.wav') if args.hohner else []):
        m = re.search(r'_(\d+)_single_(?:KLV|Bass)\.wav$', p.name)
        if m:
            files.append((p, int(m[1]), 'Hohner / Marek Kintosh', 'Author reports normalization and pitch correction. Unknown pressure/articulation.'))
    if not files:
        parser.error('no matching non-loop library takes')
    out = []
    for p, key, library, limitation in files:
        with warnings.catch_warnings():
            warnings.simplefilter('ignore', wavfile.WavFileWarning)
            rate, samples = wavfile.read(p)
        x = samples.astype(float)
        if x.ndim == 2:
            x = x[:, np.argmax(np.mean(x[:2*rate]**2, axis=0))]
        x = x[:2*rate]
        peak = np.max(np.abs(x))
        if peak == 0:
            continue
        x /= peak
        # Pitch is only needed to center the short-window tracker. Refine a
        # nominal octave using a steady segment; do not alter the recording.
        hz = 440 * 2**((key-69)/12)
        if len(x) < int(1.5*rate):
            raise ValueError(f'{p}: need at least 1.5 seconds for the pitch reference')
        frame = x[int(.5*rate):int(1.5*rate)]
        spectrum = np.abs(np.fft.rfft(frame * np.hanning(len(frame)), 4*len(frame)))
        freqs = np.fft.rfftfreq(4*len(frame), 1/rate)
        band = np.flatnonzero((freqs > hz*.96) & (freqs < hz*1.04))
        hz = float(freqs[band[np.argmax(spectrum[band])]])
        for cycles in [2, 4, 8]:
            t, a = envelope(x, rate, hz, cycles)
            a /= np.max(a)
            c = {str(db): crossing(t, a, 10**(db/20)) for db in [-50, -30, -20, -5]}
            # -50 dB is not trusted if the first 10 ms already reaches it.
            floor_db = float(20*np.log10(max(np.median(a[t < .010]), 1e-15)))
            out.append(dict(file=str(p), sha256=hashlib.sha256(p.read_bytes()).hexdigest(),
                            library=library, limitation=limitation, key=key, hz=hz,
                            rate=rate, cycles=cycles, crossings_seconds=c,
                            initial_fundamental_db=floor_db,
                            minus50_censored=floor_db >= -50 or c['-50'] == 0,
                            minus30_cut_window=c['-30'] <= cycles/(2*hz),
                            rise_30_5_ms=1000*(c['-5']-c['-30']),
                            rise_20_5_ms=1000*(c['-5']-c['-20'])))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(out, indent=2)+'\n')
    for library in sorted({r['library'] for r in out}):
        rows = [r for r in out if r['library'] == library and r['cycles'] == 4]
        print(library, len(rows), 'takes; -30 to -5 ms p10/median/p90:',
              np.round(np.percentile([r['rise_30_5_ms'] for r in rows], [10,50,90]),1),
              '; censored -50 dB:', sum(r['minus50_censored'] for r in rows),
              '; -30 crossing in initial half-window:', sum(r['minus30_cut_window'] for r in rows))
        uncensored = [r['rise_30_5_ms'] for r in rows if not r['minus30_cut_window']]
        if uncensored:
            print('  Outside the initial half-window:', len(uncensored), 'takes;',
                  np.round(np.percentile(uncensored, [10,50,90]),1))
    for r in out:
        if r['cycles']==4 and r['key'] in [45,57,69,81]:
            print(Path(r['file']).name, r['key'], round(r['rise_30_5_ms'],1),
                  round(r['rise_20_5_ms'],1), 'censored' if r['minus50_censored'] else '')


if __name__ == '__main__':
    main()
