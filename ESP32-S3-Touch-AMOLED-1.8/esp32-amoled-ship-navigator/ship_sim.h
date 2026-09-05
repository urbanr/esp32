#pragma once

#include <Arduino.h>
#include <math.h>
#include "config.h"

// ===================================================================
// Simulace letu: faze (cekani / let / pauza / cil), cas letu, nahodne
// rozmistene planety, trasa start -> planeta 1..4, zasoby s nahodnou
// rychlosti ubytku a "telemetrie" odvozena z postupu letu + sum.
// Souradnice planet jsou v pixelech bufferu nizkeho rozliseni (LR).
// ===================================================================

enum : uint8_t { PH_WAIT = 0, PH_RUN, PH_PAUSE, PH_DONE };

struct Planet {
  int x, y, r;          // stred a polomer (LR px)
  const char *name;
  int8_t dot[3][2];     // kratery relativne ke stredu
};

static uint8_t phase = PH_WAIT;
static float flightT = 0;                 // cas letu (s), 0..FLIGHT_S
static Planet planets[PLANET_COUNT];
static int startX = 0, startY = 0;        // start rakety (LR px)
static float segLen[PLANET_COUNT];        // delky useku trasy (LR px)
static float routeLen = 1;
static float foodEnd = 50, drinkEnd = 50, fuelEnd = 50, wasteEnd = 60;  // % na konci letu

// telemetrie - zobrazovane hodnoty, "tikaji" TELEMETRY_HZ za sekundu
static float tmSpeed = 0, tmDist = 0, tmHeading = 0, tmTiltX = 0, tmTiltY = 0, tmG = 1;
static float tmSunAng = 0, tmSunDist = 149.6f, tmHull = -20, tmCabinP = 101.3f, tmO2 = 21;
static float tmRad = 0.25f, tmPulse = 72;
static float tmAcc = 0;
static float prevSpeed = 0;

// jmena planet (max 9 znaku, bez diakritiky)
static const char *const PLANET_NAMES[] = {
  "MARS", "LUNA", "VENUS", "TITAN", "CERES", "PLUTO",
  "VESTA", "EUROPA", "IO", "SATURN", "MERKUR", "NEPTUN",
  "LOCHTA", "STIPANA", "PRASECI", "PSI", "OPACNA", "MLHOVA",
  "NOCNI", "BUBLANINA", "PIKOROVA", "VLKOVA",
};
#define PLANET_NAME_COUNT ((int)(sizeof(PLANET_NAMES) / sizeof(PLANET_NAMES[0])))

static inline float frand() { return random(0x10000) / 65536.0f; }
static inline float frange(float a, float b) { return a + (b - a) * frand(); }
static inline float noise(float amp) { return (frand() * 2 - 1) * amp; }

static inline float progress() { return flightT / FLIGHT_S; }

// bod na trase pro ujetou delku d (LR px); vraci index useku,
// pozici a jednotkovy smer useku
static int routePoint(float d, float &x, float &y, float &dirX, float &dirY) {
  float px = startX, py = startY;
  for (int i = 0; i < PLANET_COUNT; i++) {
    const float qx = planets[i].x, qy = planets[i].y;
    const float L = segLen[i] > 0 ? segLen[i] : 1;
    if (d <= segLen[i] || i == PLANET_COUNT - 1) {
      const float t = fminf(d / L, 1.0f);
      x = px + (qx - px) * t;
      y = py + (qy - py) * t;
      dirX = (qx - px) / L;
      dirY = (qy - py) / L;
      return i;
    }
    d -= segLen[i];
    px = qx;
    py = qy;
  }
  return PLANET_COUNT - 1;
}

// ujeta delka trasy (LR px)
static inline float routeDist() { return progress() * routeLen; }

// zacatek useku i na trase (LR px)
static float segStart(int i) {
  float s = 0;
  for (int k = 0; k < i; k++) s += segLen[k];
  return s;
}

// index aktualniho useku (planeta, ke ktere se leti)
static int currentSegment() {
  float x, y, dx, dy;
  return routePoint(routeDist(), x, y, dx, dy);
}

// zbyvajici vzdalenost k dalsi planete (km)
static float kmToNextPlanet() {
  const int seg = currentSegment();
  const float end = segStart(seg) + segLen[seg];
  const float left = fmaxf(end - routeDist(), 0.0f);
  return left / routeLen * ROUTE_TOTAL_KM;
}

// zasoby (%)
static inline float supplyPct(float endPct) { return 100.0f - (100.0f - endPct) * progress(); }
static inline float foodPct()  { return supplyPct(foodEnd); }
static inline float drinkPct() { return supplyPct(drinkEnd); }
static inline float fuelPct()  { return supplyPct(fuelEnd); }
static inline float wastePct() { return wasteEnd * progress(); }

static void telemetryTick() {
  const float p = progress();
  float x, y, dx, dy;
  const float d = routeDist();
  const int seg = routePoint(d, x, y, dx, dy);
  const float s = segLen[seg] > 0 ? fminf((d - segStart(seg)) / segLen[seg], 1.0f) : 1.0f;
  const float t = millis() * 1e-3f;

  // rychlost: v kazdem useku rozjezd a brzdeni (sinus), mimo let 0
  const bool flying = (phase == PH_RUN || phase == PH_PAUSE);
  const float speed = flying ? SPEED_MIN_KMS + (SPEED_MAX_KMS - SPEED_MIN_KMS) * sinf(PI * s) : 0;
  tmG = 1.0f + fabsf(speed - prevSpeed) * 0.06f + fabsf(noise(0.02f));
  prevSpeed = speed;
  tmSpeed = speed + (flying ? noise(0.3f) : 0);
  tmDist = p * ROUTE_TOTAL_KM;

  // kurz: nahoru = 0 deg, doprava = 90 deg
  float hd = atan2f(dx, -dy) * RAD_TO_DEG;
  if (hd < 0) hd += 360;
  tmHeading = hd;

  tmTiltX = 2.0f * sinf(t * 0.7f) + noise(0.2f);
  tmTiltY = 1.5f * cosf(t * 0.5f) + noise(0.2f);

  tmSunAng = fmodf(40.0f + 300.0f * p, 360.0f);
  tmSunDist = 149.6f + 30.0f * p;
  tmHull = -60.0f + 130.0f * fabsf(cosf(tmSunAng * DEG_TO_RAD)) + noise(1.5f);

  tmCabinP = 101.3f + noise(0.2f);
  tmO2 = 21.0f - 1.5f * p + noise(0.05f);
  tmRad = 0.25f + 0.4f * p + noise(0.03f);
  tmPulse = 72.0f + 20.0f * (speed / SPEED_MAX_KMS) + noise(2.0f);
}

// novy let: nahodne planety (v pasech odspodu nahoru), jmena, kratery,
// start, delky useku, koncove stavy zasob
static void simNewFlight() {
  const int top = 18, bottom = LR_H - 44;
  const int band = (bottom - top) / PLANET_COUNT;

  // 4 ruzna jmena
  int used[PLANET_COUNT];
  for (int i = 0; i < PLANET_COUNT; i++) {
    int n;
    bool dup;
    do {
      n = random(PLANET_NAME_COUNT);
      dup = false;
      for (int k = 0; k < i; k++) if (used[k] == n) dup = true;
    } while (dup);
    used[i] = n;
  }

  for (int i = 0; i < PLANET_COUNT; i++) {
    Planet &pl = planets[i];
    pl.r = random(5, 9);
    const int yMax = bottom - i * band - pl.r;   // planeta 1 dole, 4 nahore
    const int yMin = yMax - band + 2 * pl.r;
    pl.y = yMin < yMax ? random(yMin, yMax + 1) : yMax;
    pl.x = random(12 + pl.r, LR_W - 12 - pl.r);
    pl.name = PLANET_NAMES[used[i]];
    for (int k = 0; k < 3; k++) {
      const float a = frand() * TWO_PI, rr = frange(1, pl.r - 2);
      pl.dot[k][0] = (int8_t)lroundf(cosf(a) * rr);
      pl.dot[k][1] = (int8_t)lroundf(sinf(a) * rr);
    }
  }
  startX = random(20, LR_W - 20);
  startY = LR_H - 32;

  routeLen = 0;
  float px = startX, py = startY;
  for (int i = 0; i < PLANET_COUNT; i++) {
    const float dx = planets[i].x - px, dy = planets[i].y - py;
    segLen[i] = sqrtf(dx * dx + dy * dy);
    routeLen += segLen[i];
    px = planets[i].x;
    py = planets[i].y;
  }
  if (routeLen < 1) routeLen = 1;

  foodEnd = frange(SUPPLY_END_MIN, SUPPLY_END_MAX);
  drinkEnd = frange(SUPPLY_END_MIN, SUPPLY_END_MAX);
  fuelEnd = frange(SUPPLY_END_MIN, SUPPLY_END_MAX);
  wasteEnd = frange(WASTE_END_MIN, WASTE_END_MAX);

  flightT = 0;
  prevSpeed = 0;
  phase = PH_WAIT;
  telemetryTick();
}

// tlacitko: cekani -> let -> pauza -> let ...; v cili novy let a start
static void simButton() {
  switch (phase) {
    case PH_WAIT:  phase = PH_RUN; break;
    case PH_RUN:   phase = PH_PAUSE; break;
    case PH_PAUSE: phase = PH_RUN; break;
    case PH_DONE:  simNewFlight(); phase = PH_RUN; break;
  }
}

static void simUpdate(float dt) {
  if (phase == PH_RUN) {
    flightT += dt;
    if (flightT >= FLIGHT_S) {
      flightT = FLIGHT_S;
      phase = PH_DONE;
    }
  }
  tmAcc += dt;
  if (tmAcc >= 1.0f / TELEMETRY_HZ) {
    tmAcc = 0;
    telemetryTick();
  }
}
