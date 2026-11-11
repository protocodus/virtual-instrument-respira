// Thumbnails for the fast-dance promo and its shorts: Codex-generated hero art of
// an accordion flinging its bellows open, with one short truthful hook. Landscape: 3840x2160
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
const logo = await loadImage(`${A}/logo.png`);
// hero art: Codex image generation on the owner's Mac, 2026-11-11, prompts in
// landscape.prompt.txt and portrait.prompt.txt beside the images
const heroL = await loadImage(`${A}/landscape.png`), heroP = await loadImage(`${A}/portrait.png`);
const VERSION = '0.1.27f1';

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
  // the hero at 80 %, to the lower right, so the hook clears the keys; its dark
  // left edge runs out into the films' ground
  ctx.fillStyle = 'rgb(19,0,1)'; ctx.fillRect(0, 0, 1920, 1080);   // the hero's own edge colour
  const k = 0.8, hw = 1920 * k, hh = hw * heroL.height / heroL.width, hx = 1920 - hw, hy = 1080 - hh;
  ctx.drawImage(heroL, hx, hy, hw, hh);
  for (const [x0, y0, x1, y1] of [[hx, 0, hx + 260, 0], [0, hy, 0, hy + 200]]) {
    const f = ctx.createLinearGradient(x0, y0, x1, y1); f.addColorStop(0, 'rgba(19,0,1,1)'); f.addColorStop(1, 'rgba(19,0,1,0)');
    ctx.fillStyle = f; ctx.fillRect(x0 === x1 ? 0 : hx, y0 === y1 ? 0 : hy, x0 === x1 ? 1920 : 260, y0 === y1 ? 1080 : 200);
  }
  identity(ctx, 120, 150, 1);
  line(ctx, kicker, 120, 250, '44px InterX', '#ebb853', 'left', null, 8);
  line(ctx, hook1, 108, 400, '132px PlayX', gold(ctx, 400, 132), 'left', ['rgba(255,186,90,0.45)', 40], 14);
  if (hook2) line(ctx, hook2, 108, 530, '132px PlayX', '#f6ead6', 'left', ['rgba(255,200,140,0.3)', 30], 14);
  fs.writeFileSync(`${out}/${file}-3840x2160.png`, cv.toBuffer('image/png'));
}
function portrait(file, kicker, hook1, hook2) {
  const W = 1080, H = 1920, cv = createCanvas(W, H), ctx = cv.getContext('2d'), MX = 475;
  ctx.drawImage(heroP, 0, 0, W, H);   // same 9:16 shape, the upper half left dark for words
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
