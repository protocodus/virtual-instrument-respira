"""Respira fast-dance promo: the fast sections of three performed pieces, cut at
phrase boundaries from full-piece renders, joined and mastered as one track.

Per piece: the parts as mixed for the song films (mix.py: second voice 7 LU under
the lead, light placement, the same synthetic hall 16 dB under the dry mix), the
hall run over the whole piece so an excerpt starts with its true tail. Each
excerpt fades in over 40 ms ending on its first downbeat and out over its last
0.9 s of hall; excerpts are matched on integrated BS.1770 loudness, overlapped by
0.35 s, then the whole is mastered to -16 LUFS with true peak <= -1.15 dBTP (4x)
by mix.py's look-ahead limiter. Writes out/<set>/<set>.wav, <set>-dry.wav,
timeline.json and report.json; <set> is the film (promo) or one short."""
import os, json, numpy as np, soundfile as sf, pyloudnorm as pyln
from scipy.signal import fftconvolve, butter, sosfilt, resample_poly
from scipy.ndimage import minimum_filter1d
from dump import load
sr = 48000; meter = pyln.Meter(sr)
REG = ['Clarinet', 'Bassoon', 'Violin', 'Musette', 'Bandoneon', 'Organ', 'Oboe', 'Master']
# piece dir, parts (file stem, label, preset/register at start, gain dB), excerpt [s0, s1] in piece seconds
import sys
TAR = dict(song='tarantella', title='Tarantella', sub='Tarantella napoletana · Traditional', parts=[('solo', 'SOLO', 2, 0.0)])
USZ = dict(song='csardas-iii', title='Uszodi csárdás', sub='The friss · Hungarian traditional', parts=[('cut', 'SOLO', 6, 0.0)])
CSA = dict(song='csardas', title='Csárdás', sub='The friss · Hungarian traditional',
           parts=[('lead', 'LEAD', 2, 0.0), ('second', 'SECOND', 4, -8.78)])
# name -> (excerpts, lead-in s); a short is one excerpt under 30 s, starting at once
SETS = {
 'promo': ([dict(TAR, s0=33.40, s1=57.4), dict(USZ, s0=43.97, s1=67.5), dict(CSA, s0=86.38, s1=130.6)], 0.6),
 'short-tarantella': ([dict(TAR, s0=33.40, s1=57.4)], 0.1),
 'short-uszodi': ([dict(USZ, s0=43.97, s1=67.5)], 0.1),
 'short-csardas': ([dict(CSA, s0=100.66, s1=130.3)], 0.1),   # the Master reprise to the end
}
NAME = sys.argv[1] if len(sys.argv) > 1 else 'promo'
SEGMENTS, LEAD_IN = SETS[NAME]
OVERLAP, FADE_IN, FADE_OUT, TAIL = 0.35, 0.04, 0.9, 1.2
L = int(2.6 * sr); tt = np.arange(L) / sr
def ir(seed):   # mix.py's hall
    r = np.random.default_rng(seed).standard_normal(L)
    lo = sosfilt(butter(2, 2500, 'low', fs=sr, output='sos'), r) * np.exp(-6.91 * tt / 2.1)
    hi = sosfilt(butter(2, 2500, 'high', fs=sr, output='sos'), r) * np.exp(-6.91 * tt / 1.1) * 0.5
    h = lo + hi; h *= np.minimum(1, tt / 0.03); pre = int(0.018 * sr)
    return np.concatenate([np.zeros(pre), h])[:L]
hl, hr = ir(1), ir(2)
def tp(sig): return 20 * np.log10(np.abs(resample_poly(sig, 4, 1, axis=0)).max())

clips, dries, report, at = [], [], dict(segments=[]), LEAD_IN
for seg in SEGMENTS:
    d = f"out/{seg['song']}/final"
    x = {p: sf.read(f'{d}/{p}.wav')[0] * 10 ** (g / 20) for p, _, _, g in seg['parts']}
    n = min(len(v) for v in x.values()); x = {p: v[:n] for p, v in x.items()}
    if len(x) == 2:
        (lead, acc) = list(x); x[lead] = x[lead] * np.array([0.92, 1.0]); x[acc] = x[acc] * np.array([1.0, 0.92])
    dry = sum(x.values())
    wet = np.stack([fftconvolve(dry.mean(1), hl)[:n], fftconvolve(dry.mean(1), hr)[:n]], 1)
    wet *= np.sqrt(np.mean(dry ** 2) / np.mean(wet ** 2)) * 10 ** (-16 / 20)
    a, b = int(seg['s0'] * sr), min(n, int(seg['s1'] * sr))
    clip, dclip = (dry + wet)[a:b].copy(), dry[a:b].copy()
    env = np.ones(b - a); fi, fo = int(FADE_IN * sr), int(FADE_OUT * sr)
    env[:fi] = np.linspace(0, 1, fi) ** 2; env[-fo:] *= np.linspace(1, 0, fo) ** 2
    clip *= env[:, None]; dclip *= env[:, None]
    g = 10 ** ((-16 - meter.integrated_loudness(clip)) / 20); clip *= g; dclip *= g
    clips.append((at, clip)); dries.append((at, dclip))
    report['segments'].append(dict(title=seg['title'], piece_s=[seg['s0'], round(b / sr, 3)], promo_s=[round(at, 3), round(at + (b - a) / sr, 3)], match_gain_db=round(20 * np.log10(g), 2)))
    seg['start'], seg['end'] = at, at + (b - a) / sr
    at += (b - a) / sr - OVERLAP
os.makedirs(f'out/{NAME}', exist_ok=True)
total = int((SEGMENTS[-1]['end'] + (TAIL if len(SEGMENTS) > 1 else 0.0)) * sr)
mix, drym = np.zeros((total, 2)), np.zeros((total, 2))
for (s, c), (_, dc) in zip(clips, dries):
    i = int(round(s * sr)); mix[i:i + len(c)] += c; drym[i:i + len(dc)] += dc
mix *= 10 ** ((-16 - meter.integrated_loudness(mix)) / 20)
report['true_peak_before_limit'] = round(float(tp(mix)), 2)
up = np.abs(resample_poly(mix, 4, 1, axis=0)).max(1).reshape(-1, 4).max(1)[:total]
need = np.minimum(1, 10 ** (-1.15 / 20) / np.maximum(up, 1e-9)); la = int(.005 * sr)
target = minimum_filter1d(need, size=2 * la + 1, origin=-la)
gain = np.ones(total); rel = np.exp(-1 / (0.12 * sr)); cur = 1.0
for i in range(total):
    cur = target[i] if target[i] < cur else min(target[i], cur + (1 - cur) * (1 - rel)); gain[i] = cur
mix *= gain[:, None]
report.update(limiter_max_gr_db=round(float(-20 * np.log10(gain.min())), 2), lufs=round(float(meter.integrated_loudness(mix)), 2),
              true_peak=round(float(tp(mix)), 2), duration=round(total / sr, 3))
sf.write(f'out/{NAME}/{NAME}.wav', mix, sr, subtype='PCM_24')
sf.write(f'out/{NAME}/{NAME}-dry.wav', drym / np.abs(drym).max() * 0.89, sr, subtype='PCM_24')

# timeline for promo.mjs: each segment's notes, registers and 60 Hz trace in piece time
tl = dict(duration=total / sr, fps=30, version='0.1.27f1', segments=[])
for seg in SEGMENTS:
    d = f"out/{seg['song']}/final"; parts = []
    for name, label, reg, _ in seg['parts']:
        _, tr = load(f'{d}/{name}.mid'); t = next(iter(tr.values())) if name not in tr else tr[name]
        notes = [[round(a, 4), round(a + du, 4), p, v, c] for a, du, p, v, c in t['notes'] if not (c == 0 and p in (12, 13))]
        regs = [[0.0, REG[reg]]] + [[round(tm, 3), REG[m.program]] for tm, m in t['other'] if m.type == 'program_change']
        trc = np.genfromtxt(f'{d}/{name}.csv', delimiter=',', names=True)
        air = np.convolve(np.abs(trc['air']), np.ones(15) / 15, 'same')
        parts.append(dict(label=label, registers=regs, notes=notes, trace=dict(
            pressure=[round(float(v), 1) for v in trc['pressure']], air=[round(float(v), 3) for v in air], cells=[int(v) for v in trc['cells']])))
    tl['segments'].append(dict(title=seg['title'], sub=seg['sub'], start=seg['start'], end=seg['end'], src0=seg['s0'], parts=parts))
json.dump(tl, open(f'out/{NAME}/timeline.json', 'w'))
json.dump(report, open(f'out/{NAME}/report.json', 'w'), indent=1); print(json.dumps(report, indent=1))
