#pragma once

#include <stdint.h>
#include <string.h>
#include <math.h>
#include "config.h"
#include "sand_palette.h"

// ===================================================================
// Stav simulace - grid je jediny zdroj pravdy o tom, kde je pisek.
// Algoritmus zrnka prevzat z JS predlohy (pad -> lepivost -> skluz ->
// zklidneni/freeze), "dolu" je smer vektoru gravitace z IMU.
// ===================================================================

#define GRID_W  (368 / SAND_SCALE)
#define GRID_H  (448 / SAND_SCALE)
#define GRID_N  (GRID_W * GRID_H)

// vycentrovani hraci plochy pri rozliseni nedelitelnem SAND_SCALE
#define GRID_X_OFF  ((368 - GRID_W * SAND_SCALE) / 2)
#define GRID_Y_OFF  ((448 - GRID_H * SAND_SCALE) / 2)

#define TICK_DT (1.0f / TICK_HZ)

enum : uint8_t { CELL_EMPTY = 0, CELL_ACTIVE = 1, CELL_FROZEN = 2 };

static uint8_t  cellState[GRID_N];
static uint8_t  cellCalm[GRID_N];   // pocet klidnych ticku, strop 250
static uint8_t  cellHold[GRID_N];   // jak dlouho je zrnko "prilepene"
static uint8_t  cellColor[GRID_N];  // index do sandPalette 0-10
static uint32_t dirtyBits[(GRID_N + 31) / 32];

static uint32_t tickCount = 0;
static float    pourAcc = 0, eraseAcc = 0;

// 8 smeru serazenych po 45 stupnich (atan2 s +Y dolu)
static const int8_t DX8[8] = { 1, 1, 0, -1, -1, -1, 0, 1 };
static const int8_t DY8[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };

// gravitace pro aktualni tick
static float gravUX = 0, gravUY = 1;  // jednotkovy smer
static float gravMag = 0;             // 0..1 (0 = lezi, 1 = svisle)
static bool  simActive = false;
static int   dirK1 = 2, dirK2 = 2;    // dve nejblizsi osmismerky
static float dirFrac = 0;             // pravdepodobnost volby dirK2
static int   wakeSgx = 0, wakeSgy = 1; // dominantni smer pro wakeAround

// ---------------- PRNG (xorshift32) ----------------
static uint32_t rngState = 0x2545F491;

static inline uint32_t rnd32() {
  rngState ^= rngState << 13;
  rngState ^= rngState >> 17;
  rngState ^= rngState << 5;
  return rngState;
}
static inline float rndf() { return (rnd32() >> 8) * (1.0f / 16777216.0f); }
static inline int rndRange(int n) { return (int)(rnd32() % (uint32_t)n); }

// ---------------- pomocne ----------------
static inline void markDirty(int i) { dirtyBits[i >> 5] |= 1u << (i & 31); }

// probudi zamrzla zrnka okolo uvolnene bunky (strana proti gravitaci
// a do stran), aby nic nezustalo viset ve vzduchu
static void wakeAround(int x, int y) {
  for (int k = 0; k < 8; k++) {
    const int dx = DX8[k], dy = DY8[k];
    if (dx * wakeSgx + dy * wakeSgy > 0) continue;  // strana po smeru gravitace
    const int nx = x + dx, ny = y + dy;
    if (nx < 0 || nx >= GRID_W || ny < 0 || ny >= GRID_H) continue;
    const int j = ny * GRID_W + nx;
    if (cellState[j] == CELL_FROZEN) {
      cellState[j] = CELL_ACTIVE;
      cellCalm[j] = 0;
      cellHold[j] = 0;
    }
  }
}

static void wakeAll() {
  for (int i = 0; i < GRID_N; i++) {
    if (cellState[i] == CELL_FROZEN) {
      cellState[i] = CELL_ACTIVE;
      cellCalm[i] = 0;
      cellHold[i] = 0;
    }
  }
}

static inline void moveGrain(int i, int j, int x, int y) {
  cellState[j] = CELL_ACTIVE;
  cellColor[j] = cellColor[i];
  cellCalm[j] = 0;
  cellHold[j] = 0;
  cellState[i] = CELL_EMPTY;
  markDirty(i);
  markDirty(j);
  wakeAround(x, y);
}

static inline void calmStep(int i) {
  const uint8_t c = cellCalm[i] + 1;
  cellCalm[i] = c > 250 ? 250 : c;
  if (c >= CALM_TICKS_TO_FREEZE) cellState[i] = CELL_FROZEN;
}

// ---------------- gravitace ----------------

// gx, gy = slozky akcelerace v rovine displeje (v g), +X doprava, +Y dolu
static void simSetGravity(float gx, float gy) {
  static bool  wasActive = false;
  static float prevUX = 0, prevUY = 1;

  const float mag = sqrtf(gx * gx + gy * gy);
  gravMag = mag > 1.0f ? 1.0f : mag;

  if (mag <= TILT_DEADZONE) {
    simActive = false;
    wasActive = false;
    return;
  }

  const float ux = gx / mag, uy = gy / mag;
  const bool dirChanged = (ux * prevUX + uy * prevUY) < cosf(WAKE_ANGLE_DEG * (float)M_PI / 180.0f);
  if (!wasActive || dirChanged) wakeAll();
  prevUX = ux;
  prevUY = uy;
  wasActive = true;

  gravUX = ux;
  gravUY = uy;
  simActive = true;

  // dve nejblizsi osmismerky + pomer mixovani (prumerny pohyb sleduje vektor)
  float ang = atan2f(uy, ux);
  if (ang < 0) ang += 2.0f * (float)M_PI;
  const float a = ang / ((float)M_PI / 4.0f);
  dirK1 = ((int)a) & 7;
  dirFrac = a - (int)a;
  dirK2 = (dirK1 + 1) & 7;

  const int kPrim = (dirFrac < 0.5f) ? dirK1 : dirK2;
  wakeSgx = DX8[kPrim];
  wakeSgy = DY8[kPrim];
}

// ---------------- zrnko ----------------

static inline void stepGrain(int x, int y) {
  const int i = y * GRID_W + x;
  if (cellState[i] != CELL_ACTIVE) return;

  // loterie rychlosti: pravdepodobnost pohybu umerna naklonu
  // (preskocene zrnko nezklidnuje - pri zeslabeni gravitace nezamrzne navic)
  if (rndf() >= gravMag) return;

  // smer "dolu" pro tento pokus
  const int k = (rndf() < dirFrac) ? dirK2 : dirK1;

  // 1) pad rovne ve smeru gravitace
  {
    const int nx = x + DX8[k], ny = y + DY8[k];
    if (nx >= 0 && nx < GRID_W && ny >= 0 && ny < GRID_H) {
      const int j = ny * GRID_W + nx;
      if (cellState[j] == CELL_EMPTY) {
        moveGrain(i, j, x, y);
        return;
      }
    }
  }

  // 2) docasna lepivost: zrnko se na chvili prilepi a nesklouzava
  if (cellHold[i] < MAX_HOLD_TICKS && rndf() < STICKINESS_PROB / 100.0f) {
    cellHold[i]++;
    calmStep(i);
    return;
  }

  // 3) diagonalni skluz - padajici zrnko vs. jiz usazene zrnko kupy
  const float p = (cellCalm[i] == 0) ? SLIDE_SURFACE_PROB / 100.0f
                                     : SLIDE_SETTLED_PROB / 100.0f;
  if (rndf() < p) {
    const int s = (rnd32() & 1) ? 1 : -1;
    for (int t = 0; t < 2; t++) {
      const int kk = (k + (t == 0 ? s : -s)) & 7;
      const int nx = x + DX8[kk], ny = y + DY8[kk];
      if (nx < 0 || nx >= GRID_W || ny < 0 || ny >= GRID_H) continue;
      const int j = ny * GRID_W + nx;
      if (cellState[j] == CELL_EMPTY) {
        moveGrain(i, j, x, y);
        return;
      }
    }
  }

  // 4) zklidneni
  calmStep(i);
}

// pruchod gridem: od "nejnizsi" strany podle gravitace; poradi v kolmem
// smeru proti bocni slozce gravitace (zrnko se nikdy nezpracuje 2x za tick),
// pri nulove bocni slozce se strida po ticich
static void physicsPass() {
  const bool yMajor = fabsf(gravUY) >= fabsf(gravUX);

  if (yMajor) {
    const int y0 = (gravUY > 0) ? GRID_H - 1 : 0;
    const int ys = (gravUY > 0) ? -1 : 1;
    const bool desc = (gravUX > 0.001f) ? true
                    : (gravUX < -0.001f) ? false
                    : (tickCount & 1);
    const int x0 = desc ? GRID_W - 1 : 0;
    const int xs = desc ? -1 : 1;
    for (int y = y0, yc = 0; yc < GRID_H; y += ys, yc++)
      for (int x = x0, xc = 0; xc < GRID_W; x += xs, xc++)
        stepGrain(x, y);
  } else {
    const int x0 = (gravUX > 0) ? GRID_W - 1 : 0;
    const int xs = (gravUX > 0) ? -1 : 1;
    const bool desc = (gravUY > 0.001f) ? true
                    : (gravUY < -0.001f) ? false
                    : (tickCount & 1);
    const int y0 = desc ? GRID_H - 1 : 0;
    const int ys = desc ? -1 : 1;
    for (int x = x0, xc = 0; xc < GRID_W; x += xs, xc++)
      for (int y = y0, yc = 0; yc < GRID_H; y += ys, yc++)
        stepGrain(x, y);
  }
}

// ---------------- sypani ----------------

static void emitGrains(float shake) {
  pourAcc += EMIT_MAX_PX_PER_S * gravMag * TICK_DT;
  int n = (int)pourAcc;
  pourAcc -= n;
  if (n <= 0) return;

  // stred sypaci zony: bod okraje gridu nejbliz smeru "vzhuru"
  // (paprsek ze stredu proti gravitaci az na hranici)
  const float cx = GRID_W * 0.5f, cy = GRID_H * 0.5f;
  const float ux = -gravUX, uy = -gravUY;
  float t = 1e9f;
  if (ux > 1e-6f)       t = fminf(t, ((GRID_W - 1) - cx) / ux);
  else if (ux < -1e-6f) t = fminf(t, (0 - cx) / ux);
  if (uy > 1e-6f)       t = fminf(t, ((GRID_H - 1) - cy) / uy);
  else if (uy < -1e-6f) t = fminf(t, (0 - cy) / uy);

  // trepani zonu umerne rozsiruje (prepocet na bunky se zaokrouhlenim nahoru)
  int zone = ((int)(EMIT_ZONE_BASE_PX * (1.0f + SHAKE_ZONE_GAIN * shake))
              + SAND_SCALE - 1) / SAND_SCALE;
  if (zone < 1) zone = 1;
  if (zone > GRID_W) zone = GRID_W;
  const int half = zone / 2;

  int ex = (int)(cx + ux * t);
  int ey = (int)(cy + uy * t);
  // pritahnout stred dovnitr, aby cela zona lezela v gridu
  if (ex < half) ex = half;
  if (ex > GRID_W - 1 - half) ex = GRID_W - 1 - half;
  if (ey < half) ey = half;
  if (ey > GRID_H - 1 - half) ey = GRID_H - 1 - half;

  while (n-- > 0) {
    const int x = ex - half + rndRange(zone);
    const int y = ey - half + rndRange(zone);
    if (x < 0 || x >= GRID_W || y < 0 || y >= GRID_H) continue;
    const int i = y * GRID_W + x;
    if (cellState[i] != CELL_EMPTY) continue;  // obsazeno = pokus propada
    cellState[i] = CELL_ACTIVE;
    cellCalm[i] = 0;
    cellHold[i] = 0;
    cellColor[i] = (uint8_t)rndRange(11);
    markDirty(i);
  }
}

// ---------------- mazani dotykem ----------------

// touchX/touchY v display pixelech; funguje vzdy, i kdyz simulace stoji
static void eraseAtTouch(int touchX, int touchY) {
  eraseAcc += TOUCH_ERASE_PX_PER_S * TICK_DT;
  int n = (int)eraseAcc;
  eraseAcc -= n;

  const int cx = (touchX - GRID_X_OFF) / SAND_SCALE;
  const int cy = (touchY - GRID_Y_OFF) / SAND_SCALE;
  int zone = (ERASE_ZONE_PX + SAND_SCALE - 1) / SAND_SCALE;
  if (zone < 1) zone = 1;
  const int half = zone / 2;

  while (n-- > 0) {
    const int x = cx - half + rndRange(zone);
    const int y = cy - half + rndRange(zone);
    if (x < 0 || x >= GRID_W || y < 0 || y >= GRID_H) continue;
    const int i = y * GRID_W + x;
    if (cellState[i] == CELL_EMPTY) continue;
    cellState[i] = CELL_EMPTY;
    markDirty(i);
    wakeAround(x, y);
  }
}

// ---------------- verejne ----------------

static void simInit(uint32_t seed) {
  memset(cellState, 0, sizeof(cellState));
  memset(cellCalm, 0, sizeof(cellCalm));
  memset(cellHold, 0, sizeof(cellHold));
  memset(cellColor, 0, sizeof(cellColor));
  memset(dirtyBits, 0, sizeof(dirtyBits));
  if (seed) rngState = seed;
}

static void simTick(bool touching, int touchX, int touchY, float shake,
                    bool pouring) {
  tickCount++;

  if (touching) eraseAtTouch(touchX, touchY);
  else eraseAcc = 0;

  if (simActive) {
    if (pouring) emitGrains(shake);
    else pourAcc = 0;
    physicsPass();
  } else {
    pourAcc = 0;
  }
}
