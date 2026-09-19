#pragma once

#include <Arduino.h>
#include <math.h>
#include "config.h"
#include "zomb_gfx.h"
#include "zomb_crt_presets.h"   // crtMsgT

// ===================================================================
// Svet hry Krysy a zombici: krysa na kolech jede zleva doprava po
// zvlnenem terenu (vyskova funkce), sbira mince a znamenka, projizdi
// zombiky (zpomaleni + rozpad na dily), dojezd omezuje benzin. Usek
// konci garazi (vylepseni), tri garaze = level. Vsechny polohy jsou
// svetove souradnice v bodech (x podel trati, y dolu).
// ===================================================================

enum : uint8_t { ST_TITLE = 0, ST_DRIVE, ST_MATH, ST_GARAGE, ST_EMPTY, ST_LEVEL_DONE };
enum : uint8_t { PK_PLUS = 0, PK_MUL, PK_FUEL, PK_BALL };
enum : uint8_t { OB_LOG = 0, OB_PIPE, OB_RAMP, OB_COUNT };   // kontejner je jen kulisa (NEAR_PROPS)
enum : uint8_t { SND_NONE = 0, SND_JUMP, SND_LAND, SND_COIN, SND_OK, SND_BAD, SND_ZOMB, SND_SWING, SND_HIT,
                 SND_FUEL, SND_EMPTY, SND_BUY, SND_LEVEL, SND_PICK, SND_BUMP };
enum : uint8_t { PR_ENGINE = 0, PR_TANK, PR_WHEELS };   // vlastnosti

#define MAX_COINS  72
#define MAX_ZOMB   28
#define MAX_PICK   12
#define MAX_OBST   8
#define MAX_PROP   40
#define MAX_BLD    20
#define MAX_BYST   12
#define MAX_CLOUD  5
#define MAX_PIGEON 2
#define LEVELS     3   // tri krysy

struct Coin   { float x; int8_t dy; bool alive; };
struct Zomb   { float x, walk, animT; uint8_t kind; bool alive; };
struct Pick   { float x; uint8_t type; bool alive; };
struct Obst   { float x; uint8_t type; };
struct Prop   { float x; uint8_t kind; };
struct Bld    { float x; uint8_t kind; };
struct Part   { const Bmp *bmp; float x, y, vx, vy; bool alive, resting; };
struct Cloud  { float x, y; uint8_t kind; };
struct Pigeon { float x, y, animT; bool alive; };
struct Blood  { float x, y, vx, vy, t; };

static Coin   coins[MAX_COINS];
static Zomb   zombs[MAX_ZOMB];
static Pick   picks[MAX_PICK];
static Obst   obsts[MAX_OBST];
static Prop   props[MAX_PROP];
static Bld    blds[MAX_BLD];
static Bld    bysts[MAX_BYST];
static Part   parts[MAX_PARTS];
static Cloud  clouds[MAX_CLOUD];
static Pigeon pigeons[MAX_PIGEON];
static Blood  blood[MAX_BLOOD];
static int nObst = 0, nProp = 0, nBld = 0, nByst = 0;

static const char *const GARAGE_NAMES[LEVELS][GARAGES_PER_LEVEL] = {
  { "U HRBACE", "REZAVA DIRA", "STARA PUMPA", "U TRI KOL", "PLECHACEK" },
  { "KOCOURKOV", "ZA KOMINEM", "PLECHOVA BOUDA", "U KRIVE LAMPY", "SMETAK" },
  { "KONEC SVETA", "SROTISTE", "STO CHLUPU", "DIRA V PLOTE", "POSLEDNI STANICE" },
};
static const float RAT_SPEED[RAT_KINDS] = RAT_SPEED_MULT;
static const float RAT_SLOW[RAT_KINDS] = RAT_SLOW_MULT;
static const char *const RAT_NAMES[RAT_KINDS] = { "CHLUPATA", "DREVENA", "OCELOVA" };
static const int RAT_BASE[RAT_KINDS] = RAT_BASE_LEVEL;
static const float ZOMB_SLOWDOWN[3] = ZOMB_SLOW;

// ---- stav hry ----
static uint8_t state = ST_TITLE;
static int level = 0, garage = 0;       // level 0-2, garaz (usek) 0..GARAGES_PER_LEVEL-1 v levelu
static int rat = 0, unlocked = 1;       // zvolena krysa, pocet odemcenych
static int points = 0;
static int lvl[RAT_KINDS][3];           // urovne vlastnosti [krysa][motor, nadrz, kola]

// ---- usek ----
static float segStart = 0, segEnd = SEGMENT_LEN;
static float ph1 = 0, ph2 = 0, ph3 = 0, amp = TERRAIN_AMP;
static float mudPhase = 0;

// ---- krysa ----
static float ratX = 0, ratY = GROUND_BASE, ratV = 0, ratVy = 0, tilt = 0;
static bool  airborne = false, throttle = false;
static float fuel = 1.0f, wheelT = 0, dist = 0;
static float ballT = 0;                 // zbyvajici cas bouraku
static float swingT = -1;               // cas svihu (>= 0 = probiha)
static bool  swingHit = false;
static float stopT = 0;                 // narazeni do kontejneru
static float anim = 0, runTime = 0;
static float emptyT = 0, msgT = 0;
static uint8_t pendingSound = SND_NONE;
static int   hitX = 0, hitY = 0; static float hitT = 0;   // jiskry

// ---- priklad ----
static int mA = 0, mB = 0, mOp = 0, mAns[3], mRight = 0, mChosen = -1;
static float mResultT = 0;
static bool  garageNoMoney = false;     // okynko "uz nemas body" v garazi
static bool  tempGarage = false;        // docasna garaz po dojiti benzinu (zachytny bod zustava posledni hlavni garaz)

static inline void sound(uint8_t s) { pendingSound = s; }

static inline int shapeOf(uint8_t kind) { return kind / 3; }   // 0 hubeny, 1 hranaty, 2 kulaty

// vlastnosti zvolene krysy
static inline float vMax()  { return (V_MAX_BASE + V_MAX_STEP * (lvl[rat][PR_ENGINE] - 1)) * RAT_SPEED[rat]; }
static inline float accel() { return ACCEL_BASE + ACCEL_STEP * (lvl[rat][PR_ENGINE] - 1); }
static inline float tankLen() {
  return (TANK_BASE + TANK_STEP * (lvl[rat][PR_TANK] - 1)) / (1.0f - WHEEL_SAVE * (lvl[rat][PR_WHEELS] - 1));
}
static inline float mudFactor() { return 1.0f - MUD_SLOW * (1.0f - (lvl[rat][PR_WHEELS] - 1) / 9.0f); }
static inline int upgradeCost(int prop) { return COST_PER_LEVEL * lvl[rat][prop]; }

// ---------------- teren ----------------

// povrch terenu (y) ve svetovem x; u garazi rovina
static float ground(float wx) {
  float flat = fminf(wx - segStart, segEnd - wx) / FLAT_ZONE;
  if (flat < 0) flat = 0;
  if (flat > 1) flat = 1;
  const float p = 0.5f * sinf(wx * 0.031f + ph1) + 0.3f * sinf(wx * 0.077f + ph2) + 0.2f * sinf(wx * 0.013f + ph3);
  return GROUND_BASE - amp * flat * p;
}

// bahno / beton se stridaji po usecich 150-300 bodu
static bool isMud(float wx) {
  const float t = wx * 0.0041f + mudPhase;
  return sinf(t) + 0.5f * sinf(t * 2.3f) > -0.2f;
}

// vyska prekazky (horni plocha) ve svetovem x, nebo 0 = zadna; profil = pojizdna plocha
static float obstTop(float wx, uint8_t *type, float *left = nullptr) {
  for (int i = 0; i < nObst; i++) {
    const Obst &o = obsts[i];
    const Bmp *b = o.type == OB_LOG ? &G_LOG : (o.type == OB_PIPE ? &G_PIPE : &G_RAMP);
    const float x0 = o.x - b->ax, x1 = x0 + b->w;
    if (wx < x0 || wx >= x1) continue;
    if (type) *type = o.type;
    if (left) *left = x0;
    if (o.type == OB_RAMP) return (wx - x0) / b->w * b->h;      // stoupa zleva doprava
    return (float)b->h;
  }
  return 0;
}

// povrch pro kolo: teren nebo horni plocha prekazky
static float surface(float wx) {
  const float g = ground(wx);
  const float t = obstTop(wx, nullptr);
  return t > 0 ? g - t : g;
}

// ---------------- generator useku ----------------

static int nCoins = 0, nZomb = 0, nPick = 0;

static void addCoinRow(float x, int n, int dy) {
  for (int k = 0; k < n && nCoins < MAX_COINS; k++) coins[nCoins++] = { x + k * 9, (int8_t)dy, true };
}

static void genSegment() {
  ph1 = random(628) / 100.0f; ph2 = random(628) / 100.0f; ph3 = random(628) / 100.0f;
  mudPhase = random(628) / 100.0f;
  amp = TERRAIN_AMP + TERRAIN_AMP_STEP * level;
  nCoins = nZomb = nPick = nObst = nProp = nBld = nByst = 0;
  memset(parts, 0, sizeof(parts));
  const float len = segEnd - segStart;
  const float a = segStart + FLAT_ZONE + 20, b = segEnd - FLAT_ZONE - 20;   // uzitecna cast

  // prekazky (rozestup >= 90)
  const int nO = OBST_PER_SEG + level;
  for (int i = 0; i < nO && nObst < MAX_OBST; i++)
    obsts[nObst++] = { a + (b - a) * (i + 0.5f) / nO + random(-25, 25), (uint8_t)random(OB_COUNT) };

  // zombici: hubeni, hranati, kulati podle levelu; druh = 2 varianty siluety podle levelu
  const int cnt[3] = { ZOMB_THIN_PER_SEG + ZOMB_STEP * level, ZOMB_BLOCK_PER_SEG + level, ZOMB_ROUND_PER_SEG + level };
  for (int s = 0; s < 3; s++)
    for (int i = 0; i < cnt[s] && nZomb < MAX_ZOMB; i++) {
      const float x = a + 40 + random((long)(b - a - 80));
      const uint8_t kind = (uint8_t)(s * 3 + (level + random(2)) % 3);
      zombs[nZomb++] = { x, ZOMB_WALK_MIN + random((long)((ZOMB_WALK_MAX - ZOMB_WALK_MIN) * 10)) / 10.0f, random(100) / 100.0f, kind, true };
    }

  // mince: rady po 3-5, nektere nad prekazkami vysoko
  const int nC = COINS_PER_SEG + COINS_STEP * level;
  float x = a;
  while (nCoins < nC && x < b) {
    const int n = 3 + random(3);
    const int dy = random(100) < 25 ? -22 : -(6 + random(4));
    addCoinRow(x, n, dy);
    x += n * 9 + 25 + random(60);
  }

  // znamenka, kanystry, bouraky
  if (random(100) < MATH_PCT_SEG && nPick < MAX_PICK) picks[nPick++] = { a + (b - a) * 0.3f + random((long)((b - a) * 0.5f)), (uint8_t)(random(2) ? PK_MUL : PK_PLUS), true };
  for (int i = 0; i < FUEL_CANS_PER_SEG && nPick < MAX_PICK; i++) picks[nPick++] = { a + (b - a) * (i + 0.7f) / (FUEL_CANS_PER_SEG + 1) + random(-30, 30), PK_FUEL, true };
  for (int i = 0; i < BALLS_PER_SEG && nPick < MAX_PICK; i++) picks[nPick++] = { a + (b - a) * 0.3f + random((long)((b - a) * 0.3f)), PK_BALL, true };

  // kulisy: domy (paralaxa 0.5, svetove x v jejich prostoru = polovina drahy), rekvizity a lide (1.0)
  const float bldSpan = len * 0.5f + 200;
  for (float bx = -60; bx < bldSpan && nBld < MAX_BLD; bx += 34 + random(30)) blds[nBld++] = { bx, (uint8_t)random(BUILDING_BG_COUNT) };
  for (float px = segStart + 30; px < segEnd - 40 && nProp < MAX_PROP; px += 35 + random(60)) props[nProp++] = { px, (uint8_t)random(NEAR_PROP_COUNT) };
  for (float px = segStart + 60; px < segEnd - 60 && nByst < MAX_BYST; px += 120 + random(160)) bysts[nByst++] = { px, (uint8_t)random(7) };
  for (int i = 0; i < MAX_CLOUD; i++) clouds[i] = { (float)random(0, LW * 3), (float)random(4, 28), (uint8_t)random(2) };
  for (int i = 0; i < MAX_PIGEON; i++) pigeons[i] = { segStart + random(200, 600), (float)random(10, 40), 0, true };
}

// ---------------- stav / prechody ----------------

static void resetProgress() {
  level = garage = 0;
  rat = 0; unlocked = 1; points = 0;
  for (int r = 0; r < RAT_KINDS; r++) for (int k = 0; k < 3; k++) lvl[r][k] = RAT_BASE[r];
}

static float segmentLen() { return SEGMENT_LEN + SEGMENT_LEN_STEP * level; }

// start useku z aktualni garaze (plna nadrz)
static void startSegment() {
  segStart = 0;
  segEnd = segmentLen();
  genSegment();
  ratX = 30;
  ratV = 0; ratVy = 0; airborne = false; tilt = 0;
  ratY = ground(ratX);
  fuel = 1.0f;
  dist = 0;
  ballT = 0; swingT = -1; stopT = 0; hitT = 0; emptyT = 0; msgT = 1.5f;
  state = ST_DRIVE;
}

static void enterGarage(bool temp) {
  state = ST_GARAGE;
  msgT = 0;
  garageNoMoney = false;
  tempGarage = temp;
}

// jmeno zachytneho bodu = posledni dosazena hlavni garaz (na zacatku levelu start)
static const char *checkpointName() { return garage == 0 ? "START" : GARAGE_NAMES[level][garage - 1]; }

static void arriveGarage() {
  points += 0;
  sound(SND_LEVEL);
  if (garage == GARAGES_PER_LEVEL - 1) {   // posledni garaz levelu = level hotov, dalsi krysa
    garage = 0;
    if (level < LEVELS - 1) level++;
    if (unlocked < RAT_KINDS && unlocked <= level) unlocked = level + 1;
    state = ST_LEVEL_DONE;
    msgT = 0;
  } else {
    garage++;
    enterGarage(false);
  }
}

static bool buyUpgrade(int prop) {
  if (lvl[rat][prop] >= 10) return false;
  const int cost = upgradeCost(prop);
  if (points < cost) { garageNoMoney = true; sound(SND_BAD); return false; }
  points -= cost;
  lvl[rat][prop]++;
  sound(SND_BUY);
  return true;
}

// ---------------- priklad ----------------

static void mathStart(uint8_t type) {
  mA = random(10); mB = random(10);
  mOp = type == PK_MUL ? 1 : 0;
  const int r = mOp ? mA * mB : mA + mB;
  int w1, w2;
  do { w1 = r + random(-3, 4); } while (w1 == r || w1 < 0);
  do { w2 = mOp && r > 0 ? r + (random(2) ? mA : -mA) : r + random(-3, 4); } while (w2 == r || w2 == w1 || w2 < 0);
  mRight = random(3);
  mAns[mRight] = r;
  mAns[(mRight + 1) % 3] = w1;
  mAns[(mRight + 2) % 3] = w2;
  mChosen = -1;
  mResultT = 0;
  state = ST_MATH;
}

static void mathAnswer(int i) {
  if (mChosen >= 0) return;
  mChosen = i;
  mResultT = 0.6f;
  if (i == mRight) {
    fuel = fminf(FUEL_MAX, fuel + MATH_PCT);
    points += MATH_PTS;
    sound(SND_OK);
  } else sound(SND_BAD);
}

// ---------------- rozpad zombika ----------------

static void spawnBlood(float x, float y, float dir) {
  for (int i = 0; i < MAX_BLOOD; i++) {
    Blood &b = blood[i];
    if (b.t > 0) continue;
    b.x = x + random(-3, 4); b.y = y + random(-6, 6);
    b.vx = dir * (20 + random(60)); b.vy = -(30 + random(80));
    b.t = 0.35f + random(30) / 100.0f;
  }
}

static void breakZombie(Zomb &z, bool byBall) {
  z.alive = false;
  spawnBlood(z.x, ground(z.x) - 18, byBall ? 1.0f : 0.7f);
  const int frame = (int)(z.animT * ZOMBIE_FPS[z.kind]) % ZOMBIE_FRAMES;
  const Bmp *fr = ZOMBIE_WALK[z.kind][frame];
  const float gy = ground(z.x);
  for (int p = 0; p < ZOMBIE_PARTS; p++) {
    int slot = -1;
    for (int i = 0; i < MAX_PARTS; i++) if (!parts[i].alive) { slot = i; break; }
    if (slot < 0) { slot = 0; for (int i = 1; i < MAX_PARTS; i++) if (parts[i].resting) { slot = i; break; } }
    const Bmp *pb = ZOMBIE_PART[z.kind][p];
    Part &pt = parts[slot];
    pt.bmp = pb;
    pt.x = z.x - fr->ax + ZOMBIE_PART_POS[z.kind][frame][p][0];
    pt.y = gy - fr->ay + ZOMBIE_PART_POS[z.kind][frame][p][1];
    const bool head = p == ZOMBIE_PARTS - 1;
    pt.vx = ratV * 0.6f + random(-15, 45) + (byBall ? 30 : 0);
    pt.vy = head ? -110.0f - random(30) : -(40.0f + random(50));
    pt.alive = true;
    pt.resting = false;
  }
  hitX = (int)z.x; hitY = (int)gy - 20; hitT = 0.3f;
  sound(byBall ? SND_HIT : SND_ZOMB);
}

// ---------------- update ----------------

static inline bool overlap(float ax0, float ay0, float ax1, float ay1, float bx0, float by0, float bx1, float by1) {
  return ax0 < bx1 && ax1 > bx0 && ay0 < by1 && ay1 > by0;
}

static void ratJump() {
  if (state != ST_DRIVE || airborne) return;
  stopT = 0;
  ratVy = -JUMP_V;
  airborne = true;
  sound(SND_JUMP);
}

static void updateParts(float dt) {
  for (int i = 0; i < MAX_BLOOD; i++) {
    Blood &b = blood[i];
    if (b.t <= 0) continue;
    b.t -= dt;
    b.vy += GRAVITY * 1.5f * dt;
    b.x += b.vx * dt; b.y += b.vy * dt;
    if (b.y > ground(b.x)) b.t = 0;
  }
  for (int i = 0; i < MAX_PARTS; i++) {
    Part &p = parts[i];
    if (!p.alive || p.resting) continue;
    p.vy += GRAVITY * dt;
    p.x += p.vx * dt;
    p.y += p.vy * dt;
    const float gy = ground(p.x + p.bmp->w / 2) - p.bmp->h;
    if (p.y >= gy) {
      p.y = gy;
      if (p.vy > 60) { p.vy = -p.vy * 0.35f; p.vx *= 0.5f; }   // jeden odraz
      else { p.resting = true; }
    }
  }
}

static void updateAmbient(float dt) {
  anim += dt;
  for (int i = 0; i < MAX_PIGEON; i++) {
    Pigeon &pg = pigeons[i];
    pg.animT += dt;
    pg.x -= 18 * dt;                       // leti proti kryse
    pg.y += sinf(pg.animT * 2.0f) * 4 * dt;
    if (pg.x < ratX - RAT_SCREEN_X - 30) { pg.x = ratX + LW + random(100, 400); pg.y = random(10, 40); }
  }
  for (int i = 0; i < MAX_CLOUD; i++) {
    Cloud &c = clouds[i];
    c.x -= 2 * dt;
    const float camC = (ratX - RAT_SCREEN_X) * 0.1f;
    if (c.x + 30 < camC) c.x = camC + LW + random(20, 80);
  }
}

static void worldUpdate(float dt) {
  updateAmbient(dt);
  if (hitT > 0) hitT -= dt;
  if (msgT > 0) msgT -= dt;
  if (crtMsgT > 0) crtMsgT -= dt;
  updateParts(dt);

  if (state == ST_MATH) {
    if (mChosen >= 0) { mResultT -= dt; if (mResultT <= 0) state = ST_DRIVE; }
    return;
  }
  if (state == ST_EMPTY) {
    emptyT -= dt;
    if (ratV > 0) ratV = fmaxf(0, ratV - 30 * dt);
    ratX += ratV * dt;
    ratY = surface(ratX);
    if (emptyT <= 0) enterGarage(true);
    return;
  }
  if (state != ST_DRIVE) return;

  runTime += dt;
  if (ballT > 0) ballT -= dt;
  if (stopT > 0) stopT -= dt;

  // rychlost
  const bool mud = isMud(ratX);
  float vmax = vMax() * (mud ? mudFactor() : 1.0f);
  if (fuel <= 0) { throttle = false; }
  if (throttle && stopT <= 0) ratV = fminf(vmax, ratV + accel() * dt);
  else ratV = fmaxf(0, ratV - DRAG * dt);
  if (ratV > vmax) ratV = fmaxf(vmax, ratV - DRAG * 2 * dt);

  // vodorovny pohyb, spotreba podle drahy a sklonu
  const float prevX = ratX;
  ratX += ratV * dt;
  const float step = ratX - prevX;
  const float slope = (surface(ratX + 6) - surface(ratX - 6)) / 12.0f;   // <0 = do kopce (y dolu)
  const float slopeF = slope < 0 ? 1.0f + SLOPE_FUEL * (-slope) : fmaxf(0.4f, 1.0f + SLOPE_FUEL * (-slope));
  fuel -= step / tankLen() * slopeF;
  dist += step;
  wheelT += step;

  // svisly pohyb: kola sleduji povrch, ve vzduchu gravitace
  const float yr = surface(ratX - WHEEL_BASE / 2), yf = surface(ratX + WHEEL_BASE / 2);
  const float gy = (yr + yf) * 0.5f;
  if (airborne) {
    ratVy += GRAVITY * dt;
    ratY += ratVy * dt;
    if (ratY >= gy && ratVy > 0) { ratY = gy; airborne = false; ratVy = 0; sound(SND_LAND); }
  } else {
    // odskok z hrany (rampa, hrana prekazky), jinak drzi povrch
    if (gy - ratY > 5 && ratV > 30) { airborne = true; ratVy = -ratV * 0.35f; }
    else ratY = gy;
  }
  tilt = airborne ? tilt * 0.9f : atan2f(yf - yr, (float)WHEEL_BASE);

  // bourak: svih na zombika pred cumakem
  if (ballT > 0) {
    if (swingT < 0) {
      for (int i = 0; i < nZomb; i++) {
        const Zomb &z = zombs[i];
        if (z.alive && z.x > ratX + 10 && z.x < ratX + 10 + SWING_RANGE) { swingT = 0; swingHit = false; sound(SND_SWING); break; }
      }
    } else {
      swingT += dt;
      const int f = (int)(swingT * 15);
      if (f >= 3 && !swingHit) {
        for (int i = 0; i < nZomb; i++) {
          Zomb &z = zombs[i];
          if (z.alive && z.x > ratX + 8 && z.x < ratX + 44) { breakZombie(z, true); points += BALL_PTS; swingHit = true; }
        }
      }
      if (f >= 8) swingT = -1;
    }
  } else swingT = -1;

  // zombici: vravoraji proti kryse, kolize
  const float rx0 = ratX - 16, rx1 = ratX + 20, ry0 = ratY - 20, ry1 = ratY - 1;
  for (int i = 0; i < nZomb; i++) {
    Zomb &z = zombs[i];
    if (!z.alive) continue;
    z.animT += dt;
    if (z.x > ratX - 60) z.x -= z.walk * dt;
    const int sh = shapeOf(z.kind);
    const float zh = sh == 0 ? 30 : 35, zw = sh == 0 ? 7 : (sh == 1 ? 9 : 11);
    const float gz = ground(z.x);
    if (overlap(rx0, ry0, rx1, ry1, z.x - zw, gz - zh, z.x + zw, gz)) {
      breakZombie(z, false);
      ratV = fmaxf(MIN_SPEED_HIT, ratV - ZOMB_SLOWDOWN[sh] * RAT_SLOW[rat]);
    }
  }

  // mince a predmety
  for (int i = 0; i < nCoins; i++) {
    Coin &c = coins[i];
    if (!c.alive) continue;
    const float cy = ground(c.x) + c.dy;
    if (overlap(rx0, ry0, rx1, ry1, c.x - 4, cy - 4, c.x + 4, cy + 4)) { c.alive = false; points++; sound(SND_COIN); }
  }
  for (int i = 0; i < nPick; i++) {
    Pick &p = picks[i];
    if (!p.alive) continue;
    const float py = ground(p.x) - 12;
    if (!overlap(rx0, ry0, rx1, ry1, p.x - 5, py - 6, p.x + 5, py + 6)) continue;
    p.alive = false;
    switch (p.type) {
      case PK_PLUS: case PK_MUL: mathStart(p.type); return;
      case PK_FUEL: fuel = fminf(FUEL_MAX, fuel + FUEL_CAN_PCT); sound(SND_FUEL); break;
      default:      ballT = BALL_S; sound(SND_PICK); break;
    }
  }

  // garaz / dosel benzin
  if (ratX >= segEnd - 12) { arriveGarage(); return; }
  if (fuel <= 0 && ratV <= 0.5f) { fuel = 0; state = ST_EMPTY; emptyT = 2.5f; sound(SND_EMPTY); }
}
