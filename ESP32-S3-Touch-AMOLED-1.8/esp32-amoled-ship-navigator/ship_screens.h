#pragma once

#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include "config.h"
#include "ship_crt.h"
#include "ship_sim.h"

// ===================================================================
// Tri obrazovky kreslene do bufferu nizkeho rozliseni `lr` (ship_crt.h,
// 123 x 150 bodu) vestavenym fontem 6x8 bodu = 20 sloupcu textu.
// Barvy = intenzita fosforu (ink()). Spolecny ramecek, titulni pruh,
// stavova hlaska s casem letu a indikator obrazovek (ctverecky).
// ===================================================================

#define SCREEN_COUNT 3
#define ROW_H        8        // rozestup radku textu (LR body)
#define CH_W         6        // sirka znaku fontu (LR body)
#define CONTENT_Y    15       // prvni radek obsahu
#define STATUS_Y     (LR_H - 19)

static char txt[40];

static inline bool blink() { return (millis() / BLINK_MS) & 1; }

static void textAt(int x, int y, const char *s, uint16_t c) {
  lr->setTextSize(1);
  lr->setTextColor(c);
  lr->setCursor(x, y);
  lr->print(s);
}

static void textRight(int xr, int y, const char *s, uint16_t c) {
  textAt(xr - (int)strlen(s) * CH_W, y, s, c);
}

static void textCenter(int y, const char *s, uint16_t c) {
  textAt((LR_W - (int)strlen(s) * CH_W) / 2, y, s, c);
}

// stav + cas letu, napr. "LET 00:23"
static void statusLine() {
  const int secs = (int)flightT;
  const char *st;
  switch (phase) {
    case PH_WAIT:  st = "STISKNI TLACITKO"; break;
    case PH_RUN:   st = "LET"; break;
    case PH_PAUSE: st = "PAUZA"; break;
    default:       st = "CIL DOSAZEN"; break;
  }
  if (phase == PH_WAIT) snprintf(txt, sizeof(txt), "%s", st);
  else snprintf(txt, sizeof(txt), "%s %02d:%02d", st, secs / 60, secs % 60);
  if (phase == PH_RUN || blink()) textCenter(STATUS_Y, txt, INK_FULL);
}

// ramecek, titulni pruh, stav, indikator obrazovek
static void frameCommon(const char *title, int screen) {
  lr->fillScreen(INK_OFF);
  lr->drawRect(0, 0, LR_W, LR_H, INK_DIM);
  lr->fillRect(2, 2, LR_W - 4, 11, INK_BRIGHT);
  textCenter(4, title, INK_OFF);
  lr->drawFastHLine(2, STATUS_Y - 3, LR_W - 4, INK_FAINT);
  statusLine();
  // ctverecky obrazovek: aktualni plny
  const int y = LR_H - 8, w = 5, gap = 5;
  int x = (LR_W - (SCREEN_COUNT * w + (SCREEN_COUNT - 1) * gap)) / 2;
  for (int i = 0; i < SCREEN_COUNT; i++, x += w + gap) {
    if (i == screen) lr->fillRect(x, y, w, w, INK_FULL);
    else lr->drawRect(x, y, w, w, INK_DIM);
  }
}

// ---------------- 1. letove udaje ----------------

static int rowY;

static void row(const char *label, const char *value) {
  textAt(4, rowY, label, INK_DIM);
  textRight(LR_W - 4, rowY, value, INK_FULL);
  rowY += ROW_H;
}

static void drawTelemetry(int screen) {
  frameCommon("LETOVE UDAJE", screen);
  rowY = CONTENT_Y;
  snprintf(txt, sizeof(txt), "%5.1f KM/S", tmSpeed);          row("RYCHLOST", txt);
  snprintf(txt, sizeof(txt), "%7.0f KM", tmDist);             row("ULETENO", txt);
  snprintf(txt, sizeof(txt), "%03.0f DEG", tmHeading);        row("KURZ", txt);
  snprintf(txt, sizeof(txt), "%+4.1f %+4.1f", tmTiltX, tmTiltY); row("NAKLON", txt);
  snprintf(txt, sizeof(txt), "%4.2f G", tmG);                 row("PRETIZENI", txt);
  snprintf(txt, sizeof(txt), "%3.0f DEG", tmSunAng);          row("SLUNCE", txt);
  snprintf(txt, sizeof(txt), "%+4.0f C", tmHull);             row("PLAST", txt);
  snprintf(txt, sizeof(txt), "%5.1f KPA", tmCabinP);          row("TLAK", txt);
  snprintf(txt, sizeof(txt), "%4.1f %%", tmO2);               row("KYSLIK", txt);
  snprintf(txt, sizeof(txt), "%4.2f MSV", tmRad);             row("RADIACE", txt);
  snprintf(txt, sizeof(txt), "%3.0f %%", fuelPct());          row("PALIVO", txt);
  snprintf(txt, sizeof(txt), "%3.0f BPM", tmPulse);           row("TEP", txt);
  const int seg = currentSegment();
  row("CIL", planets[seg].name);
  snprintf(txt, sizeof(txt), "%7.0f KM", kmToNextPlanet());   row("ZBYVA", txt);
}

// ---------------- 2. navigace ----------------

// carkovana usecka: dash bodu kresli, dash bodu vynecha
static void dashedLine(int x0, int y0, int x1, int y1, int dash, uint16_t c) {
  const float dx = x1 - x0, dy = y1 - y0;
  const float len = sqrtf(dx * dx + dy * dy);
  if (len < 1) return;
  const int n = (int)len;
  for (int i = 0; i <= n; i++) {
    if (((i / dash) & 1) == 0)
      lr->drawPixel((int)lroundf(x0 + dx * i / len), (int)lroundf(y0 + dy * i / len), c);
  }
}

static void drawPlanet(const Planet &pl, int index, bool reached) {
  lr->drawCircle(pl.x, pl.y, pl.r, INK_FULL);
  if (reached) lr->drawCircle(pl.x, pl.y, pl.r + 2, INK_FAINT);
  for (int k = 0; k < 3; k++) lr->drawPixel(pl.x + pl.dot[k][0], pl.y + pl.dot[k][1], INK_DIM);
  snprintf(txt, sizeof(txt), "%d", index + 1);
  textAt(pl.x - pl.r - 8, pl.y - 4, txt, INK_BRIGHT);
  // jmeno vpravo od planety, u praveho okraje vlevo
  const int nameW = (int)strlen(pl.name) * CH_W;
  int nx = pl.x + pl.r + 3;
  if (nx + nameW > LR_W - 3) nx = pl.x - pl.r - 10 - nameW;
  if (nx < 3) nx = 3;
  textAt(nx, pl.y - 4, pl.name, INK_DIM);
}

// raketa: trojuhelnik ve smeru letu
static void drawShip(float x, float y, float dx, float dy) {
  const float px = -dy, py = dx;   // kolmice
  lr->fillTriangle((int)lroundf(x + dx * 5), (int)lroundf(y + dy * 5),
                   (int)lroundf(x - dx * 3 + px * 3), (int)lroundf(y - dy * 3 + py * 3),
                   (int)lroundf(x - dx * 3 - px * 3), (int)lroundf(y - dy * 3 - py * 3), INK_FULL);
}

static void drawNavigation(int screen) {
  frameCommon("NAVIGACE", screen);

  float sx, sy, sdx, sdy;
  const float d = routeDist();
  const int seg = routePoint(d, sx, sy, sdx, sdy);

  // trasa: uletene useky plne, aktualni usek plne az k rakete, zbytek carkovane
  float px = startX, py = startY;
  for (int i = 0; i < PLANET_COUNT; i++) {
    const Planet &pl = planets[i];
    if (i < seg || phase == PH_DONE) {
      lr->drawLine((int)px, (int)py, pl.x, pl.y, INK_BRIGHT);
    } else if (i == seg) {
      lr->drawLine((int)px, (int)py, (int)lroundf(sx), (int)lroundf(sy), INK_BRIGHT);
      dashedLine((int)lroundf(sx), (int)lroundf(sy), pl.x, pl.y, 2, INK_DIM);
    } else {
      dashedLine((int)px, (int)py, pl.x, pl.y, 2, INK_DIM);
    }
    px = pl.x;
    py = pl.y;
  }

  lr->drawCircle(startX, startY, 2, INK_DIM);
  textAt(startX - 15, startY + 4, "START", INK_FAINT);

  for (int i = 0; i < PLANET_COUNT; i++)
    drawPlanet(planets[i], i, i < seg || phase == PH_DONE);

  if (phase == PH_DONE) {
    const Planet &last = planets[PLANET_COUNT - 1];
    drawShip(last.x, last.y - last.r - 5, 0, -1);
    textAt(4, STATUS_Y - 12, "VSECHNY PLANETY", INK_BRIGHT);
  } else {
    drawShip(sx, sy, sdx, sdy);
    snprintf(txt, sizeof(txt), "CIL %d: %s", seg + 1, planets[seg].name);
    textAt(4, STATUS_Y - 12, txt, INK_BRIGHT);
  }
}

// ---------------- 3. zasoby ----------------

static void drawBar(int y, const char *label, float pct, bool warn) {
  textAt(4, y, label, INK_DIM);
  snprintf(txt, sizeof(txt), "%3.0f %%", pct);
  textRight(LR_W - 4, y, txt, INK_FULL);
  if (warn && blink()) textAt(4 + (int)strlen(label) * CH_W + 6, y, "!!!", INK_FULL);

  const int bx = 4, by = y + 9, bw = LR_W - 8, bh = 11;
  lr->drawRect(bx, by, bw, bh, INK_DIM);
  const int blocks = 20, gap = 1;
  const int blockW = (bw - 4 - (blocks - 1) * gap) / blocks;
  const int filled = (int)lroundf(pct / 100.0f * blocks);
  for (int i = 0; i < filled && i < blocks; i++)
    lr->fillRect(bx + 2 + i * (blockW + gap), by + 2, blockW, bh - 4, INK_BRIGHT);
}

static void drawSupplies(int screen) {
  frameCommon("ZASOBY", screen);
  drawBar(CONTENT_Y + 2,  "JIDLO",   foodPct(),  foodPct() < SUPPLY_WARN_PCT);
  drawBar(CONTENT_Y + 30, "PITI",    drinkPct(), drinkPct() < SUPPLY_WARN_PCT);
  drawBar(CONTENT_Y + 58, "PALIVO",  fuelPct(),  fuelPct() < SUPPLY_WARN_PCT);
  drawBar(CONTENT_Y + 86, "TOALETA", wastePct(), wastePct() > 100 - SUPPLY_WARN_PCT);
}

static void drawScreen(int screen) {
  switch (screen) {
    case 0:  drawTelemetry(screen); break;
    case 1:  drawNavigation(screen); break;
    default: drawSupplies(screen); break;
  }
}
