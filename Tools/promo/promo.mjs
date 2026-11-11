// Respira fast-dance promo: the fast sections of three pieces in one Full HD film,
// in the song films' style (render.mjs): a rolling piano roll and the reeds at the
// playhead. Frames are a deterministic function of time and the timeline (notes
// from the performed MIDI; pressure, air and sounding chambers from the engine's
// 60 Hz trace). The reeds are illustrative: their swing follows the measured
// bellows pressure (sign = push/pull), their length the pitch; their visible rate
// is not the pitch.
// usage: node promo.mjs <timeline.json> <audio.wav> <out.mp4> [--still t1 t2 ...]
import fs from 'node:fs';
import path from 'node:path';
import {spawn} from 'node:child_process';
import {createRequire} from 'node:module';
const require = createRequire(import.meta.url);
const {createCanvas, loadImage, GlobalFonts} = require('@napi-rs/canvas');
const A = path.resolve('../assets');
GlobalFonts.registerFromPath(`${A}/Playfair-800i.ttf`, 'PlayX');
GlobalFonts.registerFromPath(`${A}/Playfair-700i.ttf`, 'PlayB');
GlobalFonts.registerFromPath('/usr/share/fonts/opentype/inter/Inter-Regular.otf', 'InterR');
GlobalFonts.registerFromPath('/usr/share/fonts/opentype/inter/Inter-Medium.otf', 'InterM');
GlobalFonts.registerFromPath('/usr/share/fonts/opentype/inter/Inter-SemiBold.otf', 'InterS');
const logo = await loadImage(`${A}/logo.png`);

const [timelinePath, audioPath, outPath, ...rest] = process.argv.slice(2);
const W = 1920, H = 1080;
const still = rest[0] === '--still' ? rest.slice(1).map(Number) : null;
const T = JSON.parse(fs.readFileSync(timelinePath));
const fps = T.fps, dur = T.duration, frames = Math.ceil(dur * fps), SEG = T.segments;
const clamp = (x, a = 0, b = 1) => Math.min(b, Math.max(a, x));
const ease = x => { x = clamp(x); return x * x * (3 - 2 * x); };
const COL = [[232, 178, 84], [111, 199, 181]];   // lead gold, second teal
const rgba = (c, a) => `rgba(${c[0]},${c[1]},${c[2]},${clamp(a)})`;
for (const s of SEG) {   // each segment's pitch range, from the notes it shows
  const span = s.end - s.start;
  const ns = s.parts.flatMap(p => p.notes).filter(n => n[1] >= s.src0 - 1 && n[0] <= s.src0 + span + 1);
  s.pmin = Math.min(...ns.map(n => n[2])) - 1; s.pmax = Math.max(...ns.map(n => n[2])) + 1;
  s.last = Math.max(...ns.map(n => n[1])) - s.src0 + s.start;   // promo time of the last note-off
}
// Pieces change through a dip at the middle of their audio overlap, so two titles
// never sit on top of each other; the first piece fades in from its start.
const XF = 0.35;
SEG.forEach((s, i) => { s.in = i ? (s.start + SEG[i - 1].end) / 2 : s.start; s.out = i < SEG.length - 1 ? (s.end + SEG[i + 1].start) / 2 : Infinity; });
const segAlpha = (s, t) => ease((t - s.in) / XF) * ease((s.out - t) / XF);
const final = SEG[SEG.length - 1], OUT0 = final.last - 0.4;   // the end card rises with the last chord's release
const R = {x0: 990, x1: 1784, y0: 150, y1: 905, play: 1290}, PPS = 160;

function spaced(ctx, str, x, y, sp, align = 'left') {
  let w = 0; for (const ch of str) w += ctx.measureText(ch).width + sp; w -= sp;
  let cx = align === 'center' ? x - w / 2 : x;
  for (const ch of str) { ctx.fillText(ch, cx, y); cx += ctx.measureText(ch).width + sp; }
  return w;
}
function text(ctx, str, x, y, font, color, align = 'left', glow = null, sp = 0) {
  ctx.save(); ctx.font = font; ctx.fillStyle = color; ctx.textAlign = sp ? 'left' : align;
  if (glow) { ctx.shadowColor = glow[0]; ctx.shadowBlur = glow[1]; }
  if (sp) spaced(ctx, str, x, y, sp, align); else ctx.fillText(str, x, y);
  ctx.restore();
}
function round(ctx, x, y, w, h, r) {
  r = Math.min(r, h / 2, w / 2); ctx.beginPath(); ctx.moveTo(x + r, y); ctx.arcTo(x + w, y, x + w, y + h, r);
  ctx.arcTo(x + w, y + h, x, y + h, r); ctx.arcTo(x, y + h, x, y, r); ctx.arcTo(x, y, x + w, y, r); ctx.closePath();
}
const traceAt = (p, key, u) => { const a = p.trace[key]; return a[clamp(Math.round(u * 60) - 1, 0, a.length - 1)]; };
const registerAt = (p, u) => { let r = p.registers[0]; for (const x of p.registers) if (x[0] <= u) r = x; return r; };
const colourOf = (s, k) => s.parts.length === 1 ? COL[0] : COL[k];

// one piece's scene at piece time u, drawn at opacity a
function scene(ctx, s, u, a) {
  const {pmin, pmax} = s, span = s.end - s.start;
  const rowH = (R.y1 - R.y0) / (pmax - pmin + 1), barH = Math.max(7, rowH * 0.82);
  const yOf = p => R.y0 + (pmax - p + 0.5) * rowH;
  const edge = x => clamp((x - R.x0) / 140) * clamp((R.x1 - x) / 90);
  ctx.save(); ctx.globalAlpha = a;
  for (let p = pmin; p <= pmax; p++) {
    ctx.fillStyle = `rgba(255,220,200,${[1, 3, 6, 8, 10].includes(((p % 12) + 12) % 12) ? 0.025 : 0.045})`;
    ctx.fillRect(R.x0, yOf(p) - rowH / 2 + 0.5, R.x1 - R.x0, rowH - 1);
  }
  const ahead = (R.x1 - R.play) / PPS, behind = (R.play - R.x0) / PPS;
  // only the excerpt's own notes: nothing from before the cut or after it
  const inside = n => n[0] >= s.src0 - 0.02 && n[0] <= s.src0 + span;
  s.parts.forEach((part, k) => {
    const c = colourOf(s, k);
    for (const n of part.notes) {
      const [st, e, p, v, ch] = n;
      if (!inside(n) || st > u + ahead || e < u - behind) continue;
      const xs = Math.max(R.x0, R.play + (st - u) * PPS), xe = Math.min(R.x1, R.play + (e - u) * PPS);
      if (xe - xs < 1) continue;
      const on = st <= u && u < e, past = e <= u;
      const button = ch > 0 && s.parts.length === 1;   // a solo's bass and chord buttons, a shade softer
      const al = (on ? 1 : past ? 0.32 : 0.62) * (button ? 0.7 : 1) * edge((xs + xe) / 2);
      ctx.save();
      if (on) { ctx.shadowColor = rgba(c, 0.8); ctx.shadowBlur = 14; }
      ctx.fillStyle = rgba(on ? c.map(x => Math.min(255, x + 25)) : c, al);
      round(ctx, xs, yOf(p) - barH / 2, xe - xs, barH, barH / 2); ctx.fill(); ctx.restore();
    }
  });
  s.parts.forEach((part, k) => {   // reeds at the playhead
    const c = colourOf(s, k);
    const P = traceAt(part, 'pressure', u), share = clamp(Math.abs(P) / 450, 0, 1.25), dir = Math.sign(P);
    for (const n of part.notes) {
      const [st, e, p] = n;
      if (!inside(n) || st > u || u > e + 0.12) continue;
      const env = clamp((u - st) / 0.04) * (u > e ? 1 - (u - e) / 0.12 : 1);
      const y = yOf(p), len = clamp(110 + (pmax - p) * 3.0, 110, 300);
      const amp = (3 + 24 * share) * env;
      const rate = 2.6 + (((p % 12) + 12) % 12) / 12 * 2.2;
      const tipY = y - dir * 0.45 * amp - amp * 0.55 * Math.sin(2 * Math.PI * rate * u + p * 1.7);
      const tipX = R.play - len;
      for (const [lw, al] of [[3.2, 0.35], [1.3, 0.95]]) {
        ctx.beginPath(); ctx.moveTo(R.play, y); ctx.quadraticCurveTo(R.play - len * 0.55, y, tipX, tipY);
        ctx.strokeStyle = rgba(c.map(x => Math.min(255, x + 30)), al * env); ctx.lineWidth = lw; ctx.stroke();
      }
      ctx.save(); ctx.shadowColor = rgba(c, 0.9); ctx.shadowBlur = 10; ctx.fillStyle = rgba([255, 240, 215], env);
      ctx.beginPath(); ctx.arc(tipX, tipY, 2.6, 0, Math.PI * 2); ctx.fill(); ctx.restore();
    }
  });
  // the piece, its players and the register switch
  text(ctx, s.title, 136, 632, '112px PlayB', '#f4e8d2', 'left', ['rgba(255,200,140,0.25)', 26]);
  text(ctx, s.sub, 140, 694, '38px InterS', '#ebb853');
  const cols = s.parts.length === 2 ? [140, 520] : [140];
  s.parts.forEach((part, k) => {
    const c = colourOf(s, k), x = cols[k], y0 = 788;
    const reg = registerAt(part, u)[1].toUpperCase();
    const air = traceAt(part, 'air', u), cells = traceAt(part, 'cells', u), P = traceAt(part, 'pressure', u);
    const way = cells > 0 && Math.abs(P) > 20 ? (P > 0 ? ' · push' : ' · pull') : '';
    text(ctx, part.label, x, y0, '23px InterS', rgba(c, 1), 'left', null, 4);
    text(ctx, reg, x, y0 + 42, '31px InterS', '#f2e7d7', 'left', null, 2);
    text(ctx, `${air.toFixed(2)} L/s air · ${cells} reeds${way}`, x, y0 + 80, '23px InterR', '#dccdb9');
  });
  for (const part of s.parts) for (const [rt, name] of part.registers.slice(1)) {
    if (rt < s.src0 || rt > s.src0 + span) continue;
    const ca = ease((u - rt) / 0.3) * ease((rt + 3.6 - u) / 0.6);
    if (ca <= 0) continue;
    ctx.save(); ctx.globalAlpha *= ca; ctx.font = '26px InterM';
    const label = `Register switch: ${name}`, tw = ctx.measureText(label).width;
    round(ctx, 136, 458, tw + 44, 48, 24); ctx.fillStyle = 'rgba(40,10,12,0.55)'; ctx.fill();
    ctx.strokeStyle = 'rgba(232,178,84,0.75)'; ctx.lineWidth = 1.5; ctx.stroke();
    ctx.fillStyle = '#f2e7d7'; ctx.fillText(label, 158, 491); ctx.restore();
  }
  ctx.restore();
}

function draw(ctx, t) {
  const fade = ease(t / 0.5) * ease((dur - t) / 0.6);
  const live = SEG.map(s => [s, segAlpha(s, t)]).filter(([, a]) => a > 0);
  const endA = ease((t - OUT0) / 0.9);
  // ground: dark oxblood, a warm glow behind the roll that breathes with the air
  const g = ctx.createLinearGradient(0, 0, W, H);
  g.addColorStop(0, '#180407'); g.addColorStop(0.5, '#2a080d'); g.addColorStop(1, '#100305');
  ctx.fillStyle = g; ctx.fillRect(0, 0, W, H);
  const airAll = live.reduce((sum, [s, a]) => sum + a * s.parts.reduce((q, p) => q + traceAt(p, 'air', t - s.start + s.src0), 0), 0);
  const cx = (R.x0 + R.x1) / 2 + 60, cy = (R.y0 + R.y1) / 2;
  const glow = ctx.createRadialGradient(cx, cy, 0, cx, cy, 760);
  glow.addColorStop(0, `rgba(150,36,40,${(0.30 + 0.10 * clamp(airAll / 1.5)) * fade})`);
  glow.addColorStop(0.6, 'rgba(90,16,22,0.10)'); glow.addColorStop(1, 'rgba(0,0,0,0)');
  ctx.fillStyle = glow; ctx.fillRect(0, 0, W, H);
  for (let i = 0; i < 34; i++) {   // slow dust, deterministic
    const x = ((i * 397.31 + t * (6 + (i % 4) * 3)) % (W + 80)) - 40, y = (i * 211.7 + Math.sin(t * 0.21 + i) * 26 + H) % H;
    const r = 2 + (i % 5) * 1.4, a = (0.03 + (i % 3) * 0.025) * fade;
    const d = ctx.createRadialGradient(x, y, 0, x, y, r * 4);
    d.addColorStop(0, `rgba(255,214,170,${a})`); d.addColorStop(1, 'rgba(255,214,170,0)');
    ctx.fillStyle = d; ctx.fillRect(x - r * 4, y - r * 4, r * 8, r * 8);
  }
  for (const [s, a] of live) scene(ctx, s, t - s.start + s.src0, a * fade * (s === final ? 1 - endA : 1));
  ctx.fillStyle = `rgba(255,248,240,${0.85 * fade * (1 - endA)})`; ctx.fillRect(R.play - 1, R.y0 - 24, 2, R.y1 - R.y0 + 48);

  // the end card, in the roll's place
  if (endA > 0) {
    ctx.save(); ctx.globalAlpha = endA * fade; const mx = (R.x0 + R.x1) / 2;
    text(ctx, 'Every note,', mx, 470, '96px PlayB', '#f4e8d2', 'center', ['rgba(255,200,140,0.25)', 26]);
    text(ctx, 'air from the bellows.', mx, 580, '96px PlayB', '#f4e8d2', 'center', ['rgba(255,200,140,0.25)', 26]);
    text(ctx, 'Three dances · one physical model', mx, 668, '36px InterS', '#ebb853', 'center');
    ctx.restore();
  }

  // identity: on every frame, never faded
  ctx.drawImage(logo, 140, 108, 88, 88); text(ctx, 'PROTOCODUS', 248, 164, '30px InterS', '#efe4d4', 'left', null, 9);
  ctx.save(); ctx.font = '152px PlayX';
  const gg = ctx.createLinearGradient(0, 322 - 152 * 0.8, 0, 322 + 152 * 0.15);
  gg.addColorStop(0, '#f8dc96'); gg.addColorStop(0.55, '#e9b45c'); gg.addColorStop(1, '#c98a33');
  ctx.shadowColor = 'rgba(255,186,90,0.38)'; ctx.shadowBlur = 34; ctx.fillStyle = gg; ctx.fillText('Respira', 136, 322); ctx.restore();
  text(ctx, 'physically modelled accordion', 140, 382, '34px InterM', '#eadfd0', 'left', null, 1.2);
  text(ctx, `Reason Rack Extension · ${T.version}`, 140, 428, '25px InterM', 'rgba(222,186,120,0.92)', 'left', null, 0.6);

  // progress across the whole film, one track per piece
  const px0 = 140, px1 = 1784, py = 985, gap = 14;
  const bounds = SEG.map((s, i) => [i ? s.in : 0, Math.min(s.out, dur)]);
  ctx.save(); ctx.globalAlpha = fade;
  for (const [b0, b1] of bounds) {
    const x0 = px0 + (px1 - px0) * b0 / dur + (b0 > 0 ? gap / 2 : 0), x1 = px0 + (px1 - px0) * b1 / dur - (b1 < dur ? gap / 2 : 0);
    ctx.fillStyle = 'rgba(255,255,255,0.85)'; round(ctx, x0, py, x1 - x0, 5, 2.5); ctx.fill();
    const f = clamp((t - b0) / (b1 - b0));
    if (f > 0) { ctx.fillStyle = '#e8b254'; round(ctx, x0, py, Math.max(5, (x1 - x0) * f), 5, 2.5); ctx.fill(); }
  }
  ctx.restore();
}

const canvas = createCanvas(W, H), ctx = canvas.getContext('2d');
if (still) {
  for (const t of still) { ctx.clearRect(0, 0, W, H); draw(ctx, t); fs.writeFileSync(`${outPath}-${t}.png`, canvas.toBuffer('image/png')); }
} else {
  const ff = spawn('ffmpeg', ['-hide_banner', '-loglevel', 'error', '-y', '-f', 'rawvideo', '-pixel_format', 'rgba',
    '-video_size', `${W}x${H}`, '-framerate', String(fps), '-i', 'pipe:0', '-i', audioPath, '-map', '0:v', '-map', '1:a',
    '-c:v', 'libx264', '-preset', 'medium', '-crf', '18', '-profile:v', 'high', '-pix_fmt', 'yuv420p',
    '-colorspace', 'bt709', '-color_primaries', 'bt709', '-color_trc', 'bt709', '-color_range', 'tv',
    '-c:a', 'aac', '-b:a', '384k', '-ar', '48000', '-af', 'volume=-0.3dB', '-shortest', '-movflags', '+faststart', outPath],
    {stdio: ['pipe', 'inherit', 'inherit']});
  for (let i = 0; i < frames; i++) {
    draw(ctx, (i + 0.5) / fps);
    const buf = Buffer.from(ctx.getImageData(0, 0, W, H).data.buffer);
    if (!ff.stdin.write(buf)) await new Promise(r => ff.stdin.once('drain', r));
    if (i % 300 === 0) process.stderr.write(`${path.basename(outPath)} ${i}/${frames}\n`);
  }
  ff.stdin.end(); await new Promise(r => ff.on('close', r));
}
