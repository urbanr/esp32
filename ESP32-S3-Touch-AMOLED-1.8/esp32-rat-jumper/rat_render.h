#pragma once

#include <Arduino.h>
#include <stdio.h>
#include "config.h"
#include "rat_palette.h"
#include "rat_sprites.h"
#include "rat_world.h"
#include "rat_crt.h"

// ===================================================================
// Kresleni sceny do 8bitoveho bufferu `lr` (LW x LH, indexy palety):
// cihlova stena s paralaxou, strop, chodnik s dirami, voda, policky
// (trubky s mechem), prekazky, odpadky, pavouci na vlaknech, krysa,
// HUD (srdicka, body) a obrazovky uvodu / konce.
// ===================================================================

static char txt[32];

static inline void px(int x, int y, uint8_t c) {
  if (x >= 0 && x < LW && y >= 0 && y < LH) fb[y * LW + x] = c;
}

// sprite z ASCII, levy horni roh (x, y); '.' pruhledna
static void blit(const Sprite &s, int x, int y) {
  for (int j = 0; j < s.h; j++) {
    const int yy = y + j;
    if (yy < 0 || yy >= LH) continue;
    const char *row = s.rows[j];
    for (int i = 0; i < s.w; i++) {
      const char ch = row[i];
      if (ch == '.') continue;
      const int xx = x + i;
      if (xx >= 0 && xx < LW) fb[yy * LW + xx] = legend[(int)ch];
    }
  }
}

// blit omezeny na sloupce obrazovky [cx0, cx1) - orez opakovanych dlazdic
static void blitClip(const Sprite &s, int x, int y, int cx0, int cx1) {
  if (cx0 < 0) cx0 = 0;
  if (cx1 > LW) cx1 = LW;
  for (int j = 0; j < s.h; j++) {
    const int yy = y + j;
    if (yy < 0 || yy >= LH) continue;
    const char *row = s.rows[j];
    for (int i = 0; i < s.w; i++) {
      const int xx = x + i;
      if (xx < cx0 || xx >= cx1) continue;
      const char ch = row[i];
      if (ch != '.') fb[yy * LW + xx] = legend[(int)ch];
    }
  }
}

// opakovani dlazdice vodorovne pres [x0, x1) se svetovym posunem off
static void tileRow(const Sprite &t, int y, int x0, int x1, int off) {
  int x = x0 - ((off % t.w) + t.w) % t.w;
  for (; x < x1; x += t.w) blitClip(t, x, y, x0, x1);
}

static void text(int x, int y, const char *s, uint8_t c, int size = 1) {
  lr->setTextSize(size);
  lr->setTextColor(c);
  lr->setCursor(x, y);
  lr->print(s);
}

static void textCenter(int y, const char *s, uint8_t c, int size = 1) {
  text((LW - (int)strlen(s) * 6 * size) / 2, y, s, c, size);
}

static inline uint32_t hash2(int a, int b) {
  uint32_t h = (uint32_t)a * 374761393u + (uint32_t)b * 668265263u;
  h = (h ^ (h >> 13)) * 1274126177u;
  return h ^ (h >> 16);
}

// cihlova stena mezi stropem a chodnikem, paralaxa 1/2
static void drawWall() {
  const int bw = 12, bh = 6;
  const int off = (int)(scrollX * 0.5f);
  for (int y = CEIL_H; y < FLOOR_Y; y += bh) {
    const int row = (y - CEIL_H) / bh;
    const int shift = (row & 1) ? bw / 2 : 0;
    const int first = ((off - shift) / bw) * bw - off + shift - bw;
    for (int x = first; x < LW; x += bw) {
      const uint32_t h = hash2((x + off) / bw, row);
#if SPRITES_HAVE_TILES
      blit(h % 7 == 0 ? TILE_BRICK2 : (h % 3 == 0 ? TILE_BRICK1 : TILE_BRICK0), x, y);
#else
      const uint8_t c = (h % 7 == 0) ? C_BRICK_DARK : ((h % 3 == 0) ? C_BRICK2 : C_BRICK);
      lr->fillRect(x, y, bw - 1, bh - 1, c);
      lr->drawFastHLine(x, y + bh - 1, bw, C_MORTAR);
      lr->drawFastVLine(x + bw - 1, y, bh, C_MORTAR);
#endif
    }
  }
}

// ozdoby na zdi (kulate okno, ventil, mriz), stejna paralaxa jako cihly
static void drawWallDeco() {
  const int off = (int)(scrollX * 0.5f);
  for (int k = (off - WALL_DECO_STEP) / WALL_DECO_STEP; k <= (off + LW) / WALL_DECO_STEP; k++) {
    if (k < 0) continue;
    const uint32_t h = hash2(k, 4242);
    const int x = k * WALL_DECO_STEP + (int)((h >> 4) % (WALL_DECO_STEP - 20)) - off;
    const int v = (int)((h >> 12) % 6);
    switch (h % 5) {
      case 0: case 3: blit(SPR_WINDOW, x, CEIL_H + 5 + v); break;   // mezi stropem a 2. patrem
      case 1:         blit(SPR_VALVE, x, LANE2_Y + 5 + v); break;   // mezi patry
      case 2:         blit(SPR_GRATE, x, LANE1_Y + 8 + v); break;   // nad chodnikem
      default: break;
    }
  }
}

static void drawBat() {
  if (!batOn) return;
  const Sprite &s = ((int)(batT * 10) & 1) ? SPR_BAT1 : SPR_BAT0;
  blit(s, (int)batX - s.w / 2, batY() - s.h / 2);
}

static void drawCeiling() {
#if SPRITES_HAVE_TILES
  tileRow(TILE_CEILING, 0, 0, LW, (int)scrollX);   // vcetne krapniku pres okraj stropu
  return;
#endif
  lr->fillRect(0, 0, LW, CEIL_H, C_CEIL);
  lr->drawFastHLine(0, CEIL_H - 1, LW, C_CEIL_DARK);
  const int off = (int)scrollX;
  for (int x = -off % 9 - 9; x < LW; x += 9) {
    lr->fillRect(x, 1, 6, 3, ((x + off) / 9) & 1 ? C_CEIL_LIGHT : C_CEIL_DARK);
    // krapniky
    if (hash2((x + off) / 9, 7) % 4 == 0) {
      px(x + 3, CEIL_H, C_CEIL_DARK);
      px(x + 3, CEIL_H + 1, C_CEIL_DARK);
      px(x + 3, CEIL_H + 2, C_CEIL);
    }
  }
}

// chodnik s dirami a voda pod nim
#if SPRITES_HAVE_TILES
static void drawFloorAndWater() {
  const int f = (int)(anim * 7) & 3;   // faze vody
  const Sprite *water[4] = { &TILE_WATER0, &TILE_WATER1, &TILE_WATER2, &TILE_WATER3 };
  const Sprite *hole[4] = { &TILE_HOLE_WATER0, &TILE_HOLE_WATER1, &TILE_HOLE_WATER2, &TILE_HOLE_WATER3 };
  tileRow(*water[f], WATER_Y, 0, LW, (int)(scrollX * 0.6f));
  tileRow(TILE_WALKWAY, FLOOR_Y, 0, LW, (int)scrollX);
  for (int i = 0; i < MAX_GAP; i++) {
    const Gap &g = gaps[i];
    if (!g.alive) continue;
    const int x0 = (int)(g.x - scrollX);
    tileRow(*hole[f], FLOOR_Y, x0, x0 + g.w, (int)(scrollX * 0.6f));
    blit(TILE_HOLE_EDGE_L, x0 - TILE_HOLE_EDGE_L.w, FLOOR_Y);
    blit(TILE_HOLE_EDGE_R, x0 + g.w, FLOOR_Y);
  }
}
#else
static void drawFloorAndWater() {
  lr->fillRect(0, WATER_Y, LW, LH - WATER_Y, C_WATER);
  // vlny: hladina a par odlesku
  for (int x = 0; x < LW; x++) {
    const float ph = (x + scrollX * 1.3f) * 0.35f + anim * 3.0f;
    const int w = (int)(sinf(ph) * 1.4f + 1.4f);
    px(x, WATER_Y + w, C_WATER_LIGHT);
    if (((x + (int)(anim * 20)) % 17) == 0) px(x, WATER_Y + 5 + (x % 5), C_WATER_LIGHT);
    if (((x + (int)(anim * 12)) % 23) == 0) px(x, WATER_Y + 9 + (x % 7), C_WATER_DARK);
  }
  // chodnik: kamenne dlazdice, vynechat diry
  lr->fillRect(0, FLOOR_Y, LW, FLOOR_H, C_STONE);
  lr->drawFastHLine(0, FLOOR_Y, LW, C_STONE_LIGHT);
  lr->drawFastHLine(0, FLOOR_Y + FLOOR_H - 1, LW, C_STONE_DARK);
  const int off = (int)scrollX;
  for (int x = -(off % 10); x < LW; x += 10) lr->drawFastVLine(x, FLOOR_Y + 1, FLOOR_H - 2, C_STONE_DARK);
  for (int i = 0; i < MAX_GAP; i++) {
    const Gap &g = gaps[i];
    if (!g.alive) continue;
    const int x0 = (int)(g.x - scrollX);
    lr->fillRect(x0, FLOOR_Y, g.w, FLOOR_H, C_WATER);
    lr->drawFastHLine(x0, FLOOR_Y + 1, g.w, C_WATER_LIGHT);
    lr->drawFastVLine(x0 - 1, FLOOR_Y, FLOOR_H, C_STONE_DARK);
    lr->drawFastVLine(x0 + g.w, FLOOR_Y, FLOOR_H, C_STONE_DARK);
  }
}
#endif

// policka = vodorovna trubka s mechem
static void drawPlatform(const Platform &p) {
  const int x = (int)(p.x - scrollX), y = p.y;
#if SPRITES_HAVE_TILES
  tileRow(TILE_SHELF, y, x, x + p.w, 0);   // ukotveno na zacatek police
  blit(TILE_SHELF_CAP_L, x, y);
  blit(TILE_SHELF_CAP_R, x + p.w - TILE_SHELF_CAP_R.w, y);
  for (int k = 3; k < p.w - 5; k += 5)
    if (hash2((int)p.x + k, p.y) % 3 == 0) blit(TILE_MOSS, x + k, y - TILE_MOSS.h);
  return;
#endif
  lr->fillRect(x, y, p.w, PLATFORM_H, C_PIPE);
  lr->drawFastHLine(x, y + 1, p.w, C_PIPE_LIGHT);
  lr->drawFastHLine(x, y + PLATFORM_H - 2, p.w, C_PIPE_DARK);
  lr->drawRect(x, y, p.w, PLATFORM_H, C_OUTLINE);
  // objimky
  for (int k = 4; k < p.w - 4; k += 14) lr->drawFastVLine(x + k, y, PLATFORM_H, C_PIPE_DARK);
  // mech na hornim okraji
  for (int k = 2; k < p.w - 3; k += 5) {
    if (hash2((int)p.x + k, p.y) % 3) continue;
    lr->fillRect(x + k, y - 1, 3, 2, C_MOSS);
    px(x + k + 1, y - 2, C_MOSS_LIGHT);
  }
}

static void drawSpider(const SpiderE &s) {
  const int x = (int)(s.x - scrollX);
  const int yb = (int)spiderY(s);
#if SPRITES_HAVE_TILES
  for (int ty = CEIL_H; ty < yb - 8; ty += TILE_THREAD.h) blit(TILE_THREAD, x, ty);
#else
  lr->drawFastVLine(x, CEIL_H, yb - 8 - CEIL_H, C_THREAD);
#endif
  const Sprite &sp = ((int)(anim * 4) & 1) ? SPR_SPIDER1 : SPR_SPIDER0;
  blit(sp, x - sp.w / 2, yb - sp.h);
}

static void drawRat() {
  if (invuln > 0 && ((int)(anim * 12) & 1)) return;   // blikani po zasahu
  const Sprite &s = !onGround ? SPR_RAT_JUMP : (((int)(anim * 10) & 1) ? SPR_RAT_RUN1 : SPR_RAT_RUN0);
  blit(s, RAT_X, (int)ratY - RAT_H);
  if (shield) blit(((int)(anim * 4) & 1) ? SPR_SHIELD1 : SPR_SHIELD0, RAT_X - 3, (int)ratY - RAT_H - 4);   // bublina kolem krysy
}

static void drawAddons() {
  for (int i = 0; i < MAX_DUCK; i++) {
    const Duck &d = ducks[i];
    if (d.alive) blit(((int)(anim * 4) & 1) ? SPR_DUCK1 : SPR_DUCK0, (int)(d.x - scrollX), FLOOR_Y - 8);   // horni hrana tela = chodnik
  }
  for (int i = 0; i < MAX_CHEST; i++) {
    const Chest &c = chests[i];
    if (c.alive) blit(c.open ? SPR_CHEST_OPEN : SPR_CHEST_CLOSED, (int)(c.x - scrollX), FLOOR_Y - SPR_CHEST_CLOSED.h);
  }
  const int jf = (int)(anim * 8) & 3;
  const Sprite *seg[4] = { &SPR_JET_SEG0, &SPR_JET_SEG1, &SPR_JET_SEG2, &SPR_JET_SEG3 };
  const Sprite *top[4] = { &SPR_JET_TOP0, &SPR_JET_TOP1, &SPR_JET_TOP2, &SPR_JET_TOP3 };
  for (int i = 0; i < MAX_JET; i++) {
    const Jet &j = jets[i];
    if (!j.alive) continue;
    const int x = (int)(j.x - scrollX);
    for (int y = FLOOR_Y - SPR_JET_PIPE.h; y - seg[jf]->h >= j.top; y -= seg[jf]->h) blit(*seg[jf], x + 1, y - seg[jf]->h);
    blit(*top[jf], x - 1, j.top - top[jf]->h + 2);
    blit(SPR_JET_PIPE, x, FLOOR_Y - SPR_JET_PIPE.h);
  }
  for (int i = 0; i < MAX_DRIP; i++) {
    const Drip &d = drips[i];
    if (!d.alive) continue;
    const int x = (int)(d.x - scrollX);
    blit(SPR_DRIP_PIPE, x, CEIL_H);
    if (d.st == DR_BLINK) blit(((int)(d.t * 8) & 1) ? SPR_DROP_FLASH : SPR_DROP_READY, x + 4, CEIL_H + SPR_DRIP_PIPE.h);
    else if (d.st == DR_FALL) blit(SPR_DROP_FALL, x + 4, (int)d.dropY);
  }
}

static void drawHud() {
  for (int i = 0; i < LIVES; i++) blit(i < lives ? SPR_HEART : SPR_HEART_EMPTY, 3 + i * 9, CEIL_H + 2);
  snprintf(txt, sizeof(txt), "%d", score);
  text(LW - 3 - (int)strlen(txt) * 6, CEIL_H + 2, txt, C_TEXT);
  if (hasKey) blit(SPR_KEY, LW - 6 - (int)strlen(txt) * 6 - SPR_KEY.w, CEIL_H + 1);   // klic v HUD vedle bodu
}

static void drawOverlayBox(int y, int h) {
  lr->fillRect(14, y, LW - 28, h, C_HUD_BG);
  lr->drawRect(14, y, LW - 28, h, C_TEXT_DARK);
  lr->drawRect(15, y + 1, LW - 30, h - 2, C_TEXT);
}

static void drawTitle() {
#if SPRITES_HAVE_TILES
  blit(TILE_PANEL_INTRO, 14, 22);
#else
  drawOverlayBox(22, 62);
#endif
  textCenter(27, "KRYSA", C_TEXT, 2);
  textCenter(44, "SKOKAN", C_TEXT, 2);
  textCenter(63, "TUKNI = SKOK", C_WHITE);
  textCenter(72, "BOOT = VELKY SKOK", C_WHITE);
  if (((int)(anim * 2) & 1)) text(52, 88, "TUKNI PRO START", C_YELLOW);   // vpravo od krysy
  blit(SPR_RAT_RUN0, RAT_X, FLOOR_Y - RAT_H);
}

static void drawOver() {
#if SPRITES_HAVE_TILES
  blit(TILE_PANEL_OVER, 14, 26);
#else
  drawOverlayBox(26, 56);
#endif
  textCenter(31, "KONEC", C_RED, 2);
  snprintf(txt, sizeof(txt), "BODY %d", score);
  textCenter(52, txt, C_WHITE);
  snprintf(txt, sizeof(txt), "NEJLEPSI %d", best);
  textCenter(62, txt, C_TEXT);
  if (((int)(anim * 2) & 1)) textCenter(88, "TUKNI = ZNOVU", C_YELLOW);
}

static void drawScene() {
  drawWall();
  drawWallDeco();
  drawCeiling();
  for (int i = 0; i < MAX_PLAT; i++) if (plats[i].alive) drawPlatform(plats[i]);
  drawFloorAndWater();
  drawAddons();
  for (int i = 0; i < MAX_OBST; i++) {
    const Obstacle &o = obst[i];
    if (!o.alive) continue;
    const Sprite *s = obstSprite(o.type);
    blit(*s, (int)(o.x - scrollX), FLOOR_Y - s->h);
  }
  for (int i = 0; i < MAX_ITEM; i++) {
    const Item &it = items[i];
    if (!it.alive) continue;
    const int bob = (int)(sinf(anim * 3.0f + it.x * 0.2f) * 1.5f);
    blit(*itemSprite(it.kind), (int)(it.x - scrollX), it.y + bob);
  }
  for (int i = 0; i < MAX_SPIDER; i++) if (spiders[i].alive) drawSpider(spiders[i]);
  drawBat();
  if (state != ST_TITLE) drawRat();
#if SPRITES_HAVE_TILES
  if (sparkT > 0) {
    const Sprite *sp[4] = { &TILE_SPARK0, &TILE_SPARK1, &TILE_SPARK2, &TILE_SPARK3 };
    int f = (int)((SPARK_S - sparkT) / SPARK_S * 4); if (f > 3) f = 3;
    blit(*sp[f], sparkX - sp[f]->w / 2, sparkY - sp[f]->h / 2);
  }
  if (splashT > 0) {
    const Sprite *sp[4] = { &TILE_SPLASH0, &TILE_SPLASH1, &TILE_SPLASH2, &TILE_SPLASH3 };
    int f = (int)((SPLASH_S - splashT) / SPLASH_S * 4); if (f > 3) f = 3;
    blit(*sp[f], splashX - sp[f]->w / 2, WATER_Y - sp[f]->h);
  }
#endif
  drawHud();
  if (state == ST_TITLE) drawTitle();
  else if (state == ST_OVER) drawOver();
}
