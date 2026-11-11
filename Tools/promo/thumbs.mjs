// Thumbnails for the fast-dance promo and its shorts: the panel's open accordion
// picture (Design/Pictures/accordion-open.png, generated, see its README) over
// the films' oxblood ground, with one short truthful hook. Landscape: 3840x2160
// master; lower right kept clear for the duration label. Portrait: a 1080x1920
// cover, words inside the project's TikTok safe box (x 130-821, y 307-1190).
// usage: node thumbs.mjs <outdir>
import fs from 'node:fs';
import path from 'node:path';
import {createRequire} from 'node:module';
const require = createRequire(import.meta.url);
const {createCanvas, loadImage, GlobalFonts} = require('@napi-rs/canvas');
const A = path.resolve('../assets'), out = process.argv[2];
GlobalFonts.registerFromPath(`${A}/Playfair-800i.ttf`, 'PlayX');
GlobalFonts.registerFromPath(`${A}/Playfair-700i.ttf`, 'PlayB');
GlobalFonts.registerFromPath('/usr/share/fonts/opentype/inter/Inter-SemiBold.otf', 'InterS');
GlobalFonts.registerFromPath('/usr/share/fonts/opentype/inter/Inter-ExtraBold.otf', 'InterX');
const logo = await loadImage(`${A}/logo.png`), acc = await loadImage(`${A}/accordion-open.png`);
const VERSION = '0.1.27f1';

function ground(ctx, W, H, cx, cy, r) {
  const g = ctx.createLinearGradient(0, 0, W, H);
  g.addColorStop(0, '#1c0508'); g.addColorStop(0.5, '#33090f'); g.addColorStop(1, '#120305');
  ctx.fillStyle = g; ctx.fillRect(0, 0, W, H);
  const glow = ctx.createRadialGradient(cx, cy, 0, cx, cy, r);
  glow.addColorStop(0, 'rgba(214,72,60,0.55)'); glow.addColorStop(0.45, 'rgba(140,28,34,0.25)'); glow.addColorStop(1, 'rgba(0,0,0,0)');
  ctx.fillStyle = glow; ctx.fillRect(0, 0, W, H);
  for (let i = 0; i < 60; i++) {   // dust, as in the films
    const x = (i * 397.31) % W, y = (i * 211.7 * 1.7) % H, rr = (2 + (i % 5) * 1.6) * W / 1920;
    const d = ctx.createRadialGradient(x, y, 0, x, y, rr * 4);
    d.addColorStop(0, `rgba(255,214,170,${0.05 + (i % 3) * 0.03})`); d.addColorStop(1, 'rgba(255,214,170,0)');
    ctx.fillStyle = d; ctx.fillRect(x - rr * 4, y - rr * 4, rr * 8, rr * 8);
  }
}
// air streaks leaving the bellows: the films' note bars, gold and teal
function streaks(ctx, x0, x1, y0, y1, s, seed) {
  for (let i = 0; i < 26; i++) {
    const r = Math.abs(Math.sin(i * 12.9898 + seed) * 43758.5453) % 1, q = Math.abs(Math.sin(i * 78.233 + seed) * 12543.13) % 1;
    const y = y0 + (y1 - y0) * r, x = x0 + (x1 - x0) * q, w = (60 + 220 * ((r + q) % 1)) * s, h = 12 * s;
    const c = i % 3 ? [232, 178, 84] : [111, 199, 181];
    ctx.save(); ctx.shadowColor = `rgba(${c},0.8)`; ctx.shadowBlur = 18 * s;
    ctx.fillStyle = `rgba(${c},${0.25 + 0.5 * q})`; ctx.beginPath(); ctx.roundRect(x, y, w, h, h / 2); ctx.fill(); ctx.restore();
  }
}
function gold(ctx, y, size) {
  const g = ctx.createLinearGradient(0, y - size * 0.8, 0, y + size * 0.15);
  g.addColorStop(0, '#fbe3a4'); g.addColorStop(0.55, '#ebb65c'); g.addColorStop(1, '#c98a33'); return g;
}
function line(ctx, str, x, y, font, fill, align = 'left', glow = null, stroke = 0) {
  ctx.save(); ctx.font = font; ctx.textAlign = align; ctx.fillStyle = fill;
  if (stroke) { ctx.lineJoin = 'round'; ctx.lineWidth = stroke; ctx.strokeStyle = 'rgba(20,3,6,0.85)'; ctx.strokeText(str, x, y); }
  if (glow) { ctx.shadowColor = glow[0]; ctx.shadowBlur = glow[1]; }
  ctx.fillText(str, x, y); ctx.restore();
}
function identity(ctx, x, y, s, align = 'left') {
  ctx.save(); ctx.font = `${30 * s}px InterS`;
  const label = `PROTOCODUS  ·  RESPIRA ${VERSION}`, tw = ctx.measureText(label).width, lw = 64 * s;
  const lx = align === 'center' ? x - (lw + 16 * s + tw) / 2 : x;
  ctx.drawImage(logo, lx, y - 48 * s, lw, lw); ctx.fillStyle = '#f2e7d7'; ctx.fillText(label, lx + lw + 16 * s, y);
  ctx.restore();
}

// Landscape, drawn at 1920x1080 units and scaled 2x for the 4K master.
function landscape(file, kicker, hook1, hook2) {
  const S = 2, W = 1920 * S, H = 1080 * S, cv = createCanvas(W, H), ctx = cv.getContext('2d');
  ctx.scale(S, S);
  ground(ctx, 1920, 1080, 1000, 760, 900);
  streaks(ctx, 1080, 1840, 150, 470, 1, hook1.length);
  const aw = 1180, ah = aw * acc.height / acc.width;
  ctx.save(); ctx.shadowColor = 'rgba(0,0,0,0.6)'; ctx.shadowBlur = 50; ctx.drawImage(acc, 690, 1080 - ah + 20, aw, ah); ctx.restore();
  identity(ctx, 120, 150, 1);
  line(ctx, kicker, 120, 250, '44px InterX', '#ebb853', 'left', null, 8);
  line(ctx, hook1, 108, 390, '150px PlayX', gold(ctx, 390, 150), 'left', ['rgba(255,186,90,0.45)', 40], 14);
  if (hook2) line(ctx, hook2, 108, 530, '150px PlayX', '#f6ead6', 'left', ['rgba(255,200,140,0.3)', 30], 14);
  fs.writeFileSync(`${out}/${file}-3840x2160.png`, cv.toBuffer('image/png'));
}
function portrait(file, kicker, hook1, hook2) {
  const W = 1080, H = 1920, cv = createCanvas(W, H), ctx = cv.getContext('2d'), MX = 475;
  ground(ctx, W, H, 540, 1300, 900);
  streaks(ctx, 60, 1000, 900, 1280, 0.8, hook1.length);
  const aw = 1000, ah = aw * acc.height / acc.width;
  ctx.save(); ctx.shadowColor = 'rgba(0,0,0,0.6)'; ctx.shadowBlur = 40; ctx.drawImage(acc, (W - aw) / 2, 1330, aw, ah); ctx.restore();
  identity(ctx, MX, 370, 0.78, 'center');
  line(ctx, kicker, MX, 470, '34px InterX', '#ebb853', 'center', null, 6);
  line(ctx, hook1, MX, 610, '112px PlayX', gold(ctx, 610, 112), 'center', ['rgba(255,186,90,0.45)', 34], 10);
  if (hook2) line(ctx, hook2, MX, 730, '112px PlayX', '#f6ead6', 'center', ['rgba(255,200,140,0.3)', 26], 10);
  fs.writeFileSync(`${out}/${file}.png`, cv.toBuffer('image/png'));
}
landscape('promo-thumb', 'THREE FAST DANCES', 'No samples.', 'Just air.');
portrait('short-tarantella-cover', 'NO SAMPLES · JUST AIR', 'Tarantella', 'flat out');
portrait('short-uszodi-cover', 'NO SAMPLES · JUST AIR', 'Csárdás', 'the friss');
portrait('short-csardas-cover', 'NO SAMPLES · JUST AIR', 'The csárdás', 'finale');
