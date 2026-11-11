# Fast-dance promo

Builds the Full HD promo of Respira's fast sections, its three 9:16 shorts and
their thumbnails: the Tarantella napoletana
from its fourth strain, the Uszodi csárdás's friss on Musette and the
Csárdás's friss into its Master reprise. The parts are the performed MIDI of
the 2026-11-06 song films, which live in the project files, not here
(`respira/csardas-2026-11-06/midi/`). Rendered audio and video are review
deliverables and are never committed.

| File | Does |
| --- | --- |
| `SongRender.cpp` | plays one performed part through `respira::Engine`; a program change sets Register; writes the WAV and a 60 Hz trace of pressure, air and sounding chambers |
| `truncate_midi.py` | cuts a part at a phrase end and lifts the keys still down, so the excerpt decays as played |
| `mix.py` | the song films' mix of one piece (second voice 7 LU under the lead, hall, -16 LUFS) |
| `promo_audio.py` | `promo` cuts the three excerpts, matches their loudness, joins and masters them; `short-tarantella`, `short-uszodi`, `short-csardas` make one excerpt each, under 30 s; writes `out/<set>/timeline.json` |
| `promo.mjs` | draws the film from `timeline.json` and muxes the audio (H.264 CRF 18, AAC 384 kbps); `--portrait` draws a 1080×1920 short with every word in the project's TikTok safe box |
| `thumbs.mjs` | the promo's thumbnail (3840×2160) and the shorts' 9:16 covers |
| `dump.py` | reads a performed MIDI file into notes, controllers and other events |

```sh
g++ -std=c++17 -O3 -DNDEBUG -ISource -ITools Tools/promo/SongRender.cpp Source/DSP/RespiraEngine.cpp -o SongRender
# in a work directory holding out/<song>/final/<part>.mid:
./SongRender 2 out/tarantella/final/solo.mid out/tarantella/final/solo.wav out/tarantella/final/solo.csv
./SongRender 2 out/csardas/final/lead.mid    out/csardas/final/lead.wav    out/csardas/final/lead.csv
./SongRender 4 out/csardas/final/second.mid  out/csardas/final/second.wav  out/csardas/final/second.csv
python3 truncate_midi.py out/csardas-iii/final/solo.mid out/csardas-iii/final/cut.mid 66.36
./SongRender 6 out/csardas-iii/final/cut.mid out/csardas-iii/final/cut.wav out/csardas-iii/final/cut.csv
python3 promo_audio.py promo      # out/promo/promo.wav, timeline.json, report.json
node promo.mjs out/promo/timeline.json out/promo/promo.wav out/promo/promo.mp4
python3 promo_audio.py short-csardas
node promo.mjs out/short-csardas/timeline.json out/short-csardas/short-csardas.wav out/short-csardas/short.mp4 --portrait
node thumbs.mjs out/thumbs
```

`promo.mjs` needs `@napi-rs/canvas`, Inter in `/usr/share/fonts/opentype/inter/`,
and in `../assets/` the Playfair Display 700 and 800 italics as TTF and the
Protocodus logo (`reason-rack-extensions/Examples/Respira/Design/Brand/Protocodus-logo.png`);
`thumbs.mjs` also needs the Codex hero art `landscape.png` and `portrait.png` there
(project files `respira/promo-fast-2026-11-11/thumbs/codex-art/`, with their prompts).
`promo_audio.py` needs NumPy, SciPy, soundfile, pyloudnorm and mido.
