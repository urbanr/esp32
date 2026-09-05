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
      const uint8_t c = (h % 7 == 0) ? C_BRICK_DARK : ((h % 3 == 0) ? C_BRICK2 : C_BRICK);
      lr->fillRect(x, y, bw - 1, bh - 1, c);
      lr->drawFastHLine(x, y + bh - 1, bw, C_MORTAR);
      lr->drawFastVLine(x + bw - 1, y, bh, C_MORTAR);
    }
  }
}

static void drawCeiling() {
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

// policka = vodorovna trubka s mechem
static void drawPlatform(const Platform &p) {
  const int x = (int)(p.x - scrollX), y = p.y;
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
  lr->drawFastVLine(x, CEIL_H, yb - 8 - CEIL_H, C_THREAD);
  const Sprite &sp = ((int)(anim * 4) & 1) ? SPR_SPIDER1 : SPR_SPIDER0;
  blit(sp, x - sp.w / 2, yb - sp.h);
}

static void drawRat() {
  if (invuln > 0 && ((int)(anim * 12) & 1)) return;   // blikani po zasahu
  const Sprite &s = !onGround ? SPR_RAT_JUMP : (((int)(anim * 10) & 1) ? SPR_RAT_RUN1 : SPR_RAT_RUN0);
  blit(s, RAT_X, (int)ratY - RAT_H);
}

static void drawHud() {
  for (int i = 0; i < LIVES; i++) blit(i < lives ? SPR_HEART : SPR_HEART_EMPTY, 3 + i * 9, CEIL_H + 2);
  snprintf(txt, sizeof(txt), "%d", score);
  text(LW - 3 - (int)strlen(txt) * 6, CEIL_H + 2, txt, C_TEXT);
}

static void drawOverlayBox(int y, int h) {
  lr->fillRect(14, y, LW - 28, h, C_HUD_BG);
  lr->drawRect(14, y, LW - 28, h, C_TEXT_DARK);
  lr->drawRect(15, y + 1, LW - 30, h - 2, C_TEXT);
}

static void drawTitle() {
  drawOverlayBox(22, 62);
  textCenter(27, "KRYSA", C_TEXT, 2);
  textCenter(44, "SKOKAN", C_TEXT, 2);
  textCenter(63, "TUKNI = SKOK", C_WHITE);
  textCenter(72, "BOOT = VELKY SKOK", C_WHITE);
  if (((int)(anim * 2) & 1)) text(52, 88, "TUKNI PRO START", C_YELLOW);   // vpravo od krysy
  blit(SPR_RAT_RUN0, RAT_X, FLOOR_Y - RAT_H);
}

static void drawOver() {
  drawOverlayBox(26, 56);
  textCenter(31, "KONEC", C_RED, 2);
  snprintf(txt, sizeof(txt), "BODY %d", score);
  textCenter(52, txt, C_WHITE);
  snprintf(txt, sizeof(txt), "NEJLEPSI %d", best);
  textCenter(62, txt, C_TEXT);
  if (((int)(anim * 2) & 1)) textCenter(88, "TUKNI = ZNOVU", C_YELLOW);
}

static void drawScene() {
  drawWall();
  drawCeiling();
  for (int i = 0; i < MAX_PLAT; i++) if (plats[i].alive) drawPlatform(plats[i]);
  drawFloorAndWater();
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
  if (state != ST_TITLE) drawRat();
  drawHud();
  if (state == ST_TITLE) drawTitle();
  else if (state == ST_OVER) drawOver();
}
