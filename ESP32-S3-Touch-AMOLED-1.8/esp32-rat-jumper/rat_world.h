#pragma once

#include <Arduino.h>
#include <math.h>
#include "config.h"
#include "rat_sprites.h"

// ===================================================================
// Svet hry: krysa bezi zleva doprava, svet se posouva (scrollX,
// svetove souradnice v bodech). Generator pred pravym okrajem pridava
// vzory: prekazka na chodniku, dira ve chodniku (voda), policka
// (trubky) v 1. a 2. patre s odpadky, pavouk na vlakne ze stropu.
// Fyzika: gravitace, skok / vysoky skok, pristani na chodniku a
// polickach (jen shora), pad do vody = zasah. Sebrany odpadek = bod.
// ===================================================================

enum : uint8_t { ST_TITLE = 0, ST_PLAY, ST_OVER };
enum : uint8_t { OB_PIPE = 0, OB_CRATE, OB_SLIME, OB_COUNT };
enum : uint8_t { IT_CAN = 0, IT_PEAR, IT_PAPER, IT_ROLL, IT_COUNT };
enum : uint8_t { SND_NONE = 0, SND_JUMP, SND_JUMP_HIGH, SND_LAND, SND_COLLECT, SND_HIT, SND_SPLASH, SND_OVER, SND_START };

#define MAX_OBST   8
#define MAX_PLAT   8
#define MAX_SPIDER 6
#define MAX_ITEM   16
#define MAX_GAP    6

struct Obstacle { float x; uint8_t type; bool alive; };
struct Platform { float x; int16_t w; uint8_t y; bool alive; };
struct SpiderE  { float x; float len0; float phase; bool alive; };   // len0 = zakladni delka vlakna
struct Item     { float x; uint8_t y; uint8_t kind; bool alive; };
struct Gap      { float x; int16_t w; bool alive; };

static Obstacle obst[MAX_OBST];
static Platform plats[MAX_PLAT];
static SpiderE  spiders[MAX_SPIDER];
static Item     items[MAX_ITEM];
static Gap      gaps[MAX_GAP];

static uint8_t state = ST_TITLE;
static float scrollX = 0, speed = SPEED_START, runTime = 0, nextSpawnX = 0;
static float ratY = FLOOR_Y, ratVy = 0;      // nohy krysy (logicke y)
static bool  onGround = true;
static float coyote = 0, invuln = 0, anim = 0;
static int   lives = LIVES, score = 0, best = 0;
static uint8_t pendingSound = SND_NONE;
// kratke vizualni efekty (kresli rat_render.h, jen sada s dlazdicemi)
static float sparkT = 0, splashT = 0;      // zbyvajici cas efektu (s)
static int   sparkX = 0, sparkY = 0, splashX = 0;
#define SPARK_S  0.35f
#define SPLASH_S 0.5f

static inline void sound(uint8_t s) { pendingSound = s; }

static const Sprite *obstSprite(uint8_t type) {
  switch (type) {
    case OB_PIPE:  return &SPR_PIPE_STUB;
    case OB_CRATE: return &SPR_CRATE;
    default:       return &SPR_SLIME;
  }
}

static const Sprite *itemSprite(uint8_t kind) {
  switch (kind) {
    case IT_CAN:   return &SPR_CAN;
    case IT_PEAR:  return &SPR_PEAR;
    case IT_PAPER: return &SPR_PAPER;
    default:       return &SPR_ROLL;
  }
}

// aktualni spodek pavouka (houpe se na vlakne)
static inline float spiderY(const SpiderE &s) {
  return CEIL_H + s.len0 + sinf(runTime * 1.6f + s.phase) * 6.0f;
}

// ---------------- generator ----------------

static void addObst(float x, uint8_t type) {
  for (int i = 0; i < MAX_OBST; i++) if (!obst[i].alive) { obst[i] = { x, type, true }; return; }
}
static void addPlat(float x, int w, uint8_t y) {
  for (int i = 0; i < MAX_PLAT; i++) if (!plats[i].alive) { plats[i] = { x, (int16_t)w, y, true }; return; }
}
static void addSpider(float x, float len) {
  for (int i = 0; i < MAX_SPIDER; i++) if (!spiders[i].alive) { spiders[i] = { x, len, (float)random(628) / 100.0f, true }; return; }
}
static void addItem(float x, uint8_t y, uint8_t kind) {
  for (int i = 0; i < MAX_ITEM; i++) if (!items[i].alive) { items[i] = { x, y, kind, true }; return; }
}
static void addGap(float x, int w) {
  for (int i = 0; i < MAX_GAP; i++) if (!gaps[i].alive) { gaps[i] = { x, (int16_t)w, true }; return; }
}

static inline uint8_t randItem() { return (uint8_t)random(IT_COUNT); }

// jeden vzor od svetove souradnice x; vraci jeho sirku
static int spawnPattern(float x) {
  switch (random(6)) {
    case 0: case 1: {   // prekazka na chodniku + odpadek nad ni
      const uint8_t t = (uint8_t)random(OB_COUNT);
      addObst(x, t);
      const Sprite *s = obstSprite(t);
      addItem(x + s->w / 2 - 4, FLOOR_Y - s->h - 16, randItem());
      return s->w;
    }
    case 2: {           // dira ve chodniku, odpadek vysoko nad ni
      const int w = random(16, 27);
      addGap(x, w);
      addItem(x + w / 2 - 4, FLOOR_Y - 34, randItem());
      return w;
    }
    case 3: case 4: {   // policka v 1. patre s odpadky, obcas i 2. patro a prekazka pod ni
      const int w = random(36, 62);
      addPlat(x, w, LANE1_Y);
      for (int k = 0; k < 2 + random(2); k++) addItem(x + 6 + k * 14, LANE1_Y - 12, randItem());
      if (random(100) < 45) addObst(x + w / 2 - 6, (uint8_t)random(OB_COUNT));
      if (random(100) < 40) {
        const int w2 = random(28, 46);
        addPlat(x + w / 2, w2, LANE2_Y);
        addItem(x + w / 2 + w2 / 2 - 4, LANE2_Y - 12, IT_ROLL);
        return w / 2 + w2;
      }
      return w;
    }
    default: {          // pavouk ze stropu, odpadek na chodniku za nim
      addSpider(x + 6, (float)random(22, 62));
      addItem(x + 26, FLOOR_Y - 10, randItem());
      return 40;
    }
  }
}

static void spawnAhead() {
  while (nextSpawnX < scrollX + LW + 30) {
    const int w = spawnPattern(nextSpawnX);
    const float gap = random(SPAWN_GAP_MIN, SPAWN_GAP_MAX + 1) * (speed / SPEED_START);
    nextSpawnX += w + gap;
  }
}

static void cullBehind() {
  const float lim = scrollX - 40;
  for (int i = 0; i < MAX_OBST; i++)   if (obst[i].alive && obst[i].x + 20 < lim) obst[i].alive = false;
  for (int i = 0; i < MAX_PLAT; i++)   if (plats[i].alive && plats[i].x + plats[i].w < lim) plats[i].alive = false;
  for (int i = 0; i < MAX_SPIDER; i++) if (spiders[i].alive && spiders[i].x + 12 < lim) spiders[i].alive = false;
  for (int i = 0; i < MAX_ITEM; i++)   if (items[i].alive && items[i].x + 10 < lim) items[i].alive = false;
  for (int i = 0; i < MAX_GAP; i++)    if (gaps[i].alive && gaps[i].x + gaps[i].w < lim) gaps[i].alive = false;
}

// ---------------- hra ----------------

static void worldReset() {
  memset(obst, 0, sizeof(obst));
  memset(plats, 0, sizeof(plats));
  memset(spiders, 0, sizeof(spiders));
  memset(items, 0, sizeof(items));
  memset(gaps, 0, sizeof(gaps));
  scrollX = 0;
  speed = SPEED_START;
  runTime = 0;
  nextSpawnX = LW + 40;   // prvni prekazka az za chvili
  ratY = FLOOR_Y;
  ratVy = 0;
  onGround = true;
  coyote = invuln = anim = 0;
  lives = LIVES;
  score = 0;
}

static void gameStart() {
  worldReset();
  state = ST_PLAY;
  sound(SND_START);
}

// je pod krysou chodnik (ne dira)?
static bool floorUnderRat() {
  const float xl = scrollX + RAT_X + 5, xr = scrollX + RAT_X + RAT_W - 5;
  for (int i = 0; i < MAX_GAP; i++) {
    const Gap &g = gaps[i];
    if (g.alive && xr > g.x && xl < g.x + g.w) return false;
  }
  return true;
}

static void ratJump(bool high) {
  if (state != ST_PLAY) return;
  if (!onGround && coyote <= 0) return;
  ratVy = high ? -JUMP_HIGH_V : -JUMP_V;
  onGround = false;
  coyote = 0;
  sound(high ? SND_JUMP_HIGH : SND_JUMP);
}

static void ratHit(bool splash) {
  if (invuln > 0) return;
  lives--;
  invuln = INVULN_S;
  sound(splash ? SND_SPLASH : SND_HIT);
  if (splash) { splashT = SPLASH_S; splashX = RAT_X + RAT_W / 2; }
  if (lives <= 0) {
    state = ST_OVER;
    if (score > best) best = score;
    sound(SND_OVER);
  }
}

static inline bool overlap(float ax0, float ay0, float ax1, float ay1, float bx0, float by0, float bx1, float by1) {
  return ax0 < bx1 && ax1 > bx0 && ay0 < by1 && ay1 > by0;
}

static void worldUpdate(float dt) {
  anim += dt;
  if (sparkT > 0) sparkT -= dt;
  if (splashT > 0) splashT -= dt;
  if (state != ST_PLAY) return;

  runTime += dt;
  speed = fminf(SPEED_MAX, SPEED_START + SPEED_GAIN * runTime);
  scrollX += speed * dt;
  if (invuln > 0) invuln -= dt;
  if (coyote > 0) coyote -= dt;

  // svisly pohyb
  const float prevY = ratY;
  ratVy += GRAVITY * dt;
  ratY += ratVy * dt;
  const bool wasOnGround = onGround;
  onGround = false;
  if (ratVy >= 0) {
    // chodnik (mimo diry; behem nezranitelnosti krysa "plave" po vode)
    if ((floorUnderRat() || invuln > 0) && prevY <= FLOOR_Y + 0.5f && ratY >= FLOOR_Y) {
      ratY = FLOOR_Y;
      ratVy = 0;
      onGround = true;
    }
    // policky jen shora
    const float xl = scrollX + RAT_X + 4, xr = scrollX + RAT_X + RAT_W - 4;
    for (int i = 0; i < MAX_PLAT && !onGround; i++) {
      const Platform &p = plats[i];
      if (!p.alive || xr <= p.x || xl >= p.x + p.w) continue;
      if (prevY <= p.y + 0.5f && ratY >= p.y) {
        ratY = p.y;
        ratVy = 0;
        onGround = true;
      }
    }
  }
  if (wasOnGround && !onGround && ratVy >= 0) coyote = COYOTE_S;   // sesla z hrany
  if (onGround && !wasOnGround && ratVy == 0) sound(SND_LAND);

  // pad do vody
  if (ratY > WATER_Y + 8) {
    ratHit(true);
    ratY = FLOOR_Y;
    ratVy = 0;
    onGround = true;
  }

  // kolize (svetove souradnice, krysa trochu zmensena)
  const float rx0 = scrollX + RAT_X + 4, rx1 = scrollX + RAT_X + RAT_W - 3;
  const float ry0 = ratY - RAT_H + 2, ry1 = ratY - 1;
  for (int i = 0; i < MAX_OBST; i++) {
    const Obstacle &o = obst[i];
    if (!o.alive) continue;
    const Sprite *s = obstSprite(o.type);
    if (overlap(rx0, ry0, rx1, ry1, o.x + 1, FLOOR_Y - s->h + 1, o.x + s->w - 1, FLOOR_Y)) ratHit(false);
  }
  for (int i = 0; i < MAX_SPIDER; i++) {
    const SpiderE &s = spiders[i];
    if (!s.alive) continue;
    const float sy = spiderY(s);
    if (overlap(rx0, ry0, rx1, ry1, s.x - 4, sy - 7, s.x + 4, sy)) ratHit(false);
  }
  for (int i = 0; i < MAX_ITEM; i++) {
    Item &it = items[i];
    if (!it.alive) continue;
    const Sprite *s = itemSprite(it.kind);
    if (overlap(rx0, ry0, rx1, ry1, it.x, it.y, it.x + s->w, it.y + s->h)) {
      it.alive = false;
      score++;
      sound(SND_COLLECT);
      sparkT = SPARK_S;
      sparkX = (int)(it.x - scrollX) + s->w / 2;
      sparkY = it.y + s->h / 2;
    }
  }

  spawnAhead();
  cullBehind();
}
