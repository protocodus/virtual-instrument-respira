"""Mix and master one song: parts -> stereo mix with a light synthetic hall.
Accompaniment sits 7 LU under the lead over the windows where the lead plays
(K-weighted, 400 ms blocks); a stereo hall (RT60 ~2 s) 16 dB under the dry mix;
master to -16 LUFS with true peak at or under -1 dBTP (4x oversampled)."""
import sys, json, numpy as np, soundfile as sf, pyloudnorm as pyln
from scipy.signal import fftconvolve, butter, sosfilt, resample_poly
song, parts = sys.argv[1], sys.argv[2:]
d = f'out/{song}/final'; sr = 48000; meter = pyln.Meter(sr)
x = {p: sf.read(f'{d}/{p}.wav')[0] for p in parts}
n = min(len(v) for v in x.values()); x = {p: v[:n] for p, v in x.items()}
def blocks(sig):   # momentary loudness per 400 ms
    h = int(.4 * sr); return np.array([meter.integrated_loudness(sig[i:i+h]) if np.abs(sig[i:i+h]).max() > 1e-5 else -np.inf for i in range(0, n - h, h)])
report = {}
if len(parts) == 2:
    lead, acc = parts
    bl, ba = blocks(x[lead]), blocks(x[acc])
    on = np.isfinite(bl) & np.isfinite(ba) & (bl > np.max(bl[np.isfinite(bl)]) - 25)
    diff = np.median(bl[on] - ba[on]); trim = (diff - 7.0)
    x[acc] = x[acc] * 10 ** (trim / 20); report['accompaniment_trim_db'] = round(float(trim), 2)
    report['lead_over_accompaniment_lu'] = 7.0
    # a little placement: the lead a touch right, the second a touch left
    x[lead] = x[lead] * np.array([0.92, 1.0]); x[acc] = x[acc] * np.array([1.0, 0.92])
dry = sum(x.values())
rng = np.random.default_rng(7); L = int(2.6 * sr); t = np.arange(L) / sr
def ir(seed):
    r = np.random.default_rng(seed).standard_normal(L)
    lo = sosfilt(butter(2, 2500, 'low', fs=sr, output='sos'), r) * np.exp(-6.91 * t / 2.1)
    hi = sosfilt(butter(2, 2500, 'high', fs=sr, output='sos'), r) * np.exp(-6.91 * t / 1.1) * 0.5
    h = lo + hi; h *= np.minimum(1, t / 0.03); pre = int(0.018 * sr)
    return np.concatenate([np.zeros(pre), h])[:L]
hl, hr = ir(1), ir(2)
wet = np.stack([fftconvolve(dry.mean(1), hl)[:n], fftconvolve(dry.mean(1), hr)[:n]], 1)
wet *= np.sqrt(np.mean(dry ** 2) / np.mean(wet ** 2)) * 10 ** (-16 / 20)
mix = dry + wet
# fade the last 1.5 s of tail
f = int(1.5 * sr); mix[-f:] *= np.linspace(1, 0, f)[:, None] ** 2
lufs = meter.integrated_loudness(mix); mix *= 10 ** ((-16 - lufs) / 20)
def tp(sig): return 20 * np.log10(np.abs(resample_poly(sig, 4, 1, axis=0)).max())
peak = tp(mix); report['true_peak_before_limit'] = round(float(peak), 2)
if peak > -1.0:
    # gentle look-ahead limiter: 5 ms attack, 120 ms release on the 4x peak envelope
    up = np.abs(resample_poly(mix, 4, 1, axis=0)).max(1).reshape(-1, 4).max(1)[:n]
    ceiling = 10 ** (-1.15 / 20); need = np.minimum(1, ceiling / np.maximum(up, 1e-9))
    a = int(.005 * sr); need = -np.convolve(-np.pad(need, (0, a), constant_values=1), np.ones(1), 'same')[:n]
    g = np.ones(n); rel = np.exp(-1 / (0.12 * sr)); cur = 1.0
    # minimum over the look-ahead window, then release
    from scipy.ndimage import minimum_filter1d
    target = minimum_filter1d(need, size=2 * a + 1, origin=-a)
    for i in range(n):
        cur = target[i] if target[i] < cur else min(target[i], cur + (1 - cur) * (1 - rel))
        g[i] = cur
    mix *= g[:, None]; report['limiter_max_gr_db'] = round(float(-20 * np.log10(g.min())), 2)
report['lufs'] = round(float(meter.integrated_loudness(mix)), 2); report['true_peak'] = round(float(tp(mix)), 2)
report['lra_note'] = 'see loudness range'
sf.write(f'{d}/{song}-mix.wav', mix, sr, subtype='PCM_24')
sf.write(f'{d}/{song}-dry.wav', dry / np.abs(dry).max() * 0.89, sr, subtype='PCM_24')
json.dump(report, open(f'{d}/mix.json', 'w'), indent=1); print(song, report)
