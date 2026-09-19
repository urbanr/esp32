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
enum : uint8_t { OB_PIPE = 0, OB_CRATE, OB_SLIME, OB_COUNT, OB_SNAIL = OB_COUNT };   // snek leze, ma vlastni vzor
enum : uint8_t { IT_CAN = 0, IT_PEAR, IT_PAPER, IT_ROLL, IT_COUNT, IT_CUP_SILVER = IT_COUNT, IT_CUP_GOLD, IT_KEY, IT_BUBBLE };   // od IT_COUNT bonusy
enum : uint8_t { SND_NONE = 0, SND_JUMP, SND_JUMP_HIGH, SND_LAND, SND_COLLECT, SND_HIT, SND_SPLASH, SND_OVER, SND_START, SND_CUP, SND_POP, SND_JET };
enum : uint8_t { DR_WAIT = 0, DR_BLINK, DR_FALL };   // stav kapajici trubky

#define MAX_OBST   8
#define MAX_PLAT   8
#define MAX_SPIDER 6
#define MAX_ITEM   16
#define MAX_GAP    6
#define MAX_CHEST  3
#define MAX_DRIP   4
#define MAX_JET    3
#define MAX_DUCK   3

struct Obstacle { float x; uint8_t type; bool alive; };
struct Platform { float x; int16_t w; uint8_t y; bool alive; };
struct SpiderE  { float x; float len0; float phase; bool alive; };   // len0 = zakladni delka vlakna
struct Item     { float x; uint8_t y; uint8_t kind; bool alive; };
struct Gap      { float x; int16_t w; bool alive; };
struct Chest    { float x; bool open; bool alive; };
struct Drip     { float x; float t; float dropY; uint8_t st; bool alive; };   // trubka u stropu, kapka pada na chodnik
struct Jet      { float x; int16_t top; bool alive; };                        // tryska na chodniku, proud az do vysky top
struct Duck     { float x; bool alive; };                                     // kachnicka v dire (chodi se po ni)

static Obstacle obst[MAX_OBST];
static Platform plats[MAX_PLAT];
static SpiderE  spiders[MAX_SPIDER];
static Item     items[MAX_ITEM];
static Gap      gaps[MAX_GAP];
static Chest    chests[MAX_CHEST];
static Drip     drips[MAX_DRIP];
static Jet      jets[MAX_JET];
static Duck     ducks[MAX_DUCK];

static uint8_t state = ST_TITLE;
static float scrollX = 0, speed = SPEED_START, runTime = 0, nextSpawnX = 0;
static float ratY = FLOOR_Y, ratVy = 0;      // nohy krysy (logicke y)
static bool  onGround = true;
static float coyote = 0, invuln = 0, anim = 0;
static bool  hasKey = false, shield = false, inJet = false;
static int   lives = LIVES, score = 0, best = 0;
static bool  bestDirty = false;             // nove nejlepsi skore, ulozit (rat_app.h -> SD karta)
static int   itemCount = 0;                 // pocitadlo odpadku pro pohary
static uint8_t pendingSound = SND_NONE;
// netopyr - jen parada, prolita zprava doleva a vlni se nahoru a dolu
static bool  batOn = false;
static float batX = 0, batBase = 0, batT = 0, batTimer = 4;
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
    case OB_SNAIL: return ((int)(anim * 6) & 1) ? &SPR_SNAIL1 : &SPR_SNAIL0;
    default:       return &SPR_SLIME;
  }
}

static const Sprite *itemSprite(uint8_t kind) {
  switch (kind) {
    case IT_CAN:        return &SPR_CAN;
    case IT_PEAR:       return &SPR_PEAR;
    case IT_PAPER:      return &SPR_PAPER;
    case IT_CUP_SILVER: return &SPR_CUP_SILVER;
    case IT_CUP_GOLD:   return &SPR_CUP_GOLD;
    case IT_KEY:        return &SPR_KEY;
    case IT_BUBBLE:     return ((int)(anim * 4) & 1) ? &SPR_BUBBLE1 : &SPR_BUBBLE0;
    default:            return &SPR_ROLL;
  }
}

static inline int itemPoints(uint8_t kind) {
  return kind == IT_CUP_GOLD ? CUP_GOLD_PTS : (kind == IT_CUP_SILVER ? CUP_SILVER_PTS : 1);
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
static void addChest(float x) {
  for (int i = 0; i < MAX_CHEST; i++) if (!chests[i].alive) { chests[i] = { x, false, true }; return; }
}
static void addDrip(float x) {
  for (int i = 0; i < MAX_DRIP; i++) if (!drips[i].alive) { drips[i] = { x, (float)random(10) / 10.0f, 0, DR_WAIT, true }; return; }
}
static void addJet(float x, int top) {
  for (int i = 0; i < MAX_JET; i++) if (!jets[i].alive) { jets[i] = { x, (int16_t)top, true }; return; }
}
static void addDuck(float x) {
  for (int i = 0; i < MAX_DUCK; i++) if (!ducks[i].alive) { ducks[i] = { x, true }; addPlat(x + 3, 11, FLOOR_Y); return; }
}

// nahodny odpadek; kazdy CUP_EVERY-ty je pohar (stribrny, kazdy druhy zlaty),
// obcas klic nebo bublina
static uint8_t randItem() {
  if (++itemCount % CUP_EVERY == 0) return (itemCount / CUP_EVERY) & 1 ? IT_CUP_SILVER : IT_CUP_GOLD;
  const int r = random(100);
  if (r < KEY_CHANCE) return IT_KEY;
  if (r < KEY_CHANCE + BUBBLE_CHANCE) return IT_BUBBLE;
  return (uint8_t)random(IT_COUNT);
}

// jeden vzor od svetove souradnice x; vraci jeho sirku
static int spawnPattern(float x) {
  switch (random(10)) {
    case 0: case 1: {   // prekazka na chodniku + odpadek nad ni, obcas bedynka za ni
      const uint8_t t = (uint8_t)random(OB_COUNT);
      addObst(x, t);
      const Sprite *s = obstSprite(t);
      addItem(x + s->w / 2 - 4, FLOOR_Y - s->h - 16, randItem());
      if (random(100) < CHEST_CHANCE) { addChest(x + s->w + 12); return s->w + 12 + SPR_CHEST_CLOSED.w; }
      return s->w;
    }
    case 6: {           // snek leze po chodniku proti kryse, odpadek nad nim
      addObst(x + 20, OB_SNAIL);
      addItem(x + 20, FLOOR_Y - 26, randItem());
      return 34;
    }
    case 7: {           // kapajici trubka u stropu, odpadek na chodniku pod ni
      addDrip(x + 6);
      addItem(x + 4, FLOOR_Y - 10, randItem());
      return 30;
    }
    case 8: {           // proud vody vynese krysu na policku ve 2. patre s odpadky
      addJet(x, LANE2_Y + 2);
      const int w2 = random(30, 50);
      addPlat(x + 14, w2, LANE2_Y);
      for (int k = 0; k < 2 + random(2); k++) addItem(x + 18 + k * 12, LANE2_Y - 12, randItem());
      return 14 + w2;
    }
    case 9: {           // siroka dira s kachnickou uprostred, odpadek nad ni
      const int w = random(38, 46);
      addGap(x, w);
      addDuck(x + (w - SPR_DUCK0.w) / 2);
      addItem(x + w / 2 - 4, FLOOR_Y - 30, randItem());
      return w;
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
  for (int i = 0; i < MAX_CHEST; i++)  if (chests[i].alive && chests[i].x + 14 < lim) chests[i].alive = false;
  for (int i = 0; i < MAX_DRIP; i++)   if (drips[i].alive && drips[i].x + 12 < lim) drips[i].alive = false;
  for (int i = 0; i < MAX_JET; i++)    if (jets[i].alive && jets[i].x + 16 < lim) jets[i].alive = false;
  for (int i = 0; i < MAX_DUCK; i++)   if (ducks[i].alive && ducks[i].x + 24 < lim) ducks[i].alive = false;
}

// ---------------- hra ----------------

static void worldReset() {
  memset(obst, 0, sizeof(obst));
  memset(plats, 0, sizeof(plats));
  memset(spiders, 0, sizeof(spiders));
  memset(items, 0, sizeof(items));
  memset(gaps, 0, sizeof(gaps));
  memset(chests, 0, sizeof(chests));
  memset(drips, 0, sizeof(drips));
  memset(jets, 0, sizeof(jets));
  memset(ducks, 0, sizeof(ducks));
  hasKey = shield = inJet = false;
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
  itemCount = 0;
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
  if (shield) {                 // bublina chrani pred jednim narazem
    shield = false;
    invuln = INVULN_S * 0.5f;
    sound(SND_POP);
    return;
  }
  lives--;
  invuln = INVULN_S;
  sound(splash ? SND_SPLASH : SND_HIT);
  if (splash) { splashT = SPLASH_S; splashX = RAT_X + RAT_W / 2; }
  if (lives <= 0) {
    state = ST_OVER;
    if (score > best) { best = score; bestDirty = true; }
    sound(SND_OVER);
  }
}

static inline bool overlap(float ax0, float ay0, float ax1, float ay1, float bx0, float by0, float bx1, float by1) {
  return ax0 < bx1 && ax1 > bx0 && ay0 < by1 && ay1 > by0;
}

// netopyr: obcas prolitne zprava doleva (proti smeru hry) a vlni se
static void batUpdate(float dt) {
  if (!batOn) {
    batTimer -= dt;
    if (batTimer > 0) return;
    batOn = true;
    batX = LW + 16;
    batT = 0;
    batBase = random(CEIL_H + 14, LANE2_Y - 6);
    batTimer = BAT_GAP_MIN_S + random((long)((BAT_GAP_MAX_S - BAT_GAP_MIN_S) * 10)) / 10.0f;
    return;
  }
  batT += dt;
  batX -= ((state == ST_PLAY ? speed : 0) + BAT_SPEED) * dt;
  if (batX < -20) batOn = false;
}
static inline int batY() { return (int)(batBase + sinf(batT * 4.0f) * 9.0f); }

static void worldUpdate(float dt) {
  anim += dt;
  if (sparkT > 0) sparkT -= dt;
  if (splashT > 0) splashT -= dt;
  batUpdate(dt);
  if (state != ST_PLAY) return;

  runTime += dt;
  speed = fminf(SPEED_MAX, SPEED_START + SPEED_GAIN * runTime);
  scrollX += speed * dt;
  if (invuln > 0) invuln -= dt;
  if (coyote > 0) coyote -= dt;

  // snek leze proti kryse
  for (int i = 0; i < MAX_OBST; i++) if (obst[i].alive && obst[i].type == OB_SNAIL) obst[i].x -= SNAIL_SPEED * dt;

  // kapajici trubky: pauza -> blikani kapky -> pad na chodnik
  for (int i = 0; i < MAX_DRIP; i++) {
    Drip &d = drips[i];
    if (!d.alive) continue;
    d.t += dt;
    if (d.st == DR_WAIT && d.t > DRIP_WAIT_S) { d.st = DR_BLINK; d.t = 0; }
    else if (d.st == DR_BLINK && d.t > DRIP_BLINK_S) { d.st = DR_FALL; d.t = 0; d.dropY = CEIL_H + SPR_DRIP_PIPE.h; }
    else if (d.st == DR_FALL) {
      d.dropY += DROP_SPEED * dt;
      if (d.dropY > FLOOR_Y) { d.st = DR_WAIT; d.t = 0; }
    }
  }

  // proud vody: krysu v nem vynasi nahoru
  {
    const float xl = scrollX + RAT_X + 6, xr = scrollX + RAT_X + RAT_W - 6;
    bool lift = false;
    for (int i = 0; i < MAX_JET; i++) {
      const Jet &j = jets[i];
      if (!j.alive || xr <= j.x + 2 || xl >= j.x + 12) continue;
      if (ratY > j.top && ratY <= FLOOR_Y + 0.5f) lift = true;
    }
    if (lift) { ratVy = -JET_LIFT; onGround = false; if (!inJet) sound(SND_JET); }
    inJet = lift;
  }

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
  for (int i = 0; i < MAX_DRIP; i++) {
    const Drip &d = drips[i];
    if (!d.alive || d.st != DR_FALL) continue;
    if (overlap(rx0, ry0, rx1, ry1, d.x + 4, d.dropY, d.x + 8, d.dropY + 7)) ratHit(false);
  }
  for (int i = 0; i < MAX_CHEST; i++) {
    Chest &c = chests[i];
    if (!c.alive || c.open || !hasKey) continue;
    if (overlap(rx0, ry0, rx1, ry1, c.x, FLOOR_Y - SPR_CHEST_CLOSED.h, c.x + SPR_CHEST_CLOSED.w, FLOOR_Y)) {
      c.open = true;
      hasKey = false;
      score += CHEST_PTS;
      sound(SND_CUP);
      sparkT = SPARK_S;
      sparkX = (int)(c.x - scrollX) + SPR_CHEST_CLOSED.w / 2;
      sparkY = FLOOR_Y - SPR_CHEST_CLOSED.h;
    }
  }
  for (int i = 0; i < MAX_ITEM; i++) {
    Item &it = items[i];
    if (!it.alive) continue;
    const Sprite *s = itemSprite(it.kind);
    if (overlap(rx0, ry0, rx1, ry1, it.x, it.y, it.x + s->w, it.y + s->h)) {
      it.alive = false;
      score += itemPoints(it.kind);
      if (it.kind == IT_KEY) hasKey = true;
      else if (it.kind == IT_BUBBLE) shield = true;
      sound(it.kind == IT_CUP_SILVER || it.kind == IT_CUP_GOLD ? SND_CUP : SND_COLLECT);
      sparkT = SPARK_S;
      sparkX = (int)(it.x - scrollX) + s->w / 2;
      sparkY = it.y + s->h / 2;
    }
  }

  spawnAhead();
  cullBehind();
}
