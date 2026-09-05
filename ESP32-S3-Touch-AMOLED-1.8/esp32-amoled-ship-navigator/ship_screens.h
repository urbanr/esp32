#pragma once

#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include "config.h"
#include "ship_crt.h"
#include "ship_sim.h"
#include "font_tomthumb.h"   // 3x5 font (Adafruit GFX, BSD) pro hrube mrizky (>= 4)

// ===================================================================
// Ctyri obrazovky kreslene do bufferu nizkeho rozliseni `lr` (ship_crt.h).
// Pro mrizku 2-3 vestaveny font 6x8 bodu, pro 4-6 drobny font 3x5.
// Barvy = intenzita fosforu (ink()). Spolecny zaobleny ramecek (displej
// ma kulate rohy), titulni pruh, stavova hlaska s casem letu a
// indikator obrazovek (ctverecky). Ctvrta obrazovka ukazuje parametry
// CRT filtru: tuknuti je vylosuje, tah nahoru/dolu meni mrizku.
// ===================================================================

#define SCREEN_COUNT 4
#define TELEMETRY_ROWS 15

// metriky fontu podle aktualni mrizky (nastavuje layoutUpdate())
static int CH_W = 6, CH_H = 8, FONT_BASE = 0;   // FONT_BASE: GFX font ma kurzor na ucari
#define TITLE_H      (CH_H + 3)
#define CONTENT_Y    (TITLE_H + 4)          // prvni radek obsahu
#define STATUS_Y     (LR_H - 2 * CH_H - 3)
// rozestup radku tabule podle vysky bufferu (min. vyska fontu)
#define ROW_H        ((STATUS_Y - 3 - CONTENT_Y) / TELEMETRY_ROWS < CH_H ? CH_H : (STATUS_Y - 3 - CONTENT_Y) / TELEMETRY_ROWS)
#define CORNER_R     (SCREEN_CORNER_PX / crt.scale)

static char txt[40];

static void layoutUpdate() {
  if (crt.scale >= 4) { CH_W = 4; CH_H = 6; FONT_BASE = 5; }
  else { CH_W = 6; CH_H = 8; FONT_BASE = 0; }
}

static inline bool blink() { return (millis() / BLINK_MS) & 1; }

static void textAt(int x, int y, const char *s, uint16_t c) {
  lr->setFont(crt.scale >= 4 ? &TomThumb : nullptr);
  lr->setTextSize(1);
  lr->setTextColor(c);
  lr->setCursor(x, y + FONT_BASE);
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
  // zaobleny ramecek: rohy displeje jsou kulate, obsah za nimi mizi
  lr->drawRoundRect(0, 0, LR_W, LR_H, CORNER_R, INK_DIM);
  lr->fillRect(CORNER_R - 2, 2, LR_W - 2 * (CORNER_R - 2), TITLE_H, INK_BRIGHT);
  textCenter(3, title, INK_OFF);
  statusLine();
  // ctverecky obrazovek: aktualni plny
  const int y = LR_H - CH_H, w = CH_H - 3, gap = CH_H - 3;
  int x = (LR_W - (SCREEN_COUNT * w + (SCREEN_COUNT - 1) * gap)) / 2;
  for (int i = 0; i < SCREEN_COUNT; i++, x += w + gap) {
    if (i == screen) lr->fillRect(x, y, w, w, INK_FULL);
    else lr->drawRect(x, y, w, w, INK_DIM);
  }
}

// ---------------- 1. letove udaje ----------------

static int rowY;

static void row(const char *label, const char *value) {
  if (rowY + CH_H > STATUS_Y - 3) return;   // uz se nevejde (hrube mrizky)
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
  if (ROW_H * TELEMETRY_ROWS <= STATUS_Y - 3 - CONTENT_Y) {   // vejde se i 15. radek
    snprintf(txt, sizeof(txt), "%5.1f MIL.KM", tmSunDist);    row("VZDAL.SLUNCE", txt);
  }
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
  textAt(pl.x - pl.r - CH_W - 2, pl.y - CH_H / 2, txt, INK_BRIGHT);
  // jmeno vpravo od planety, u praveho okraje vlevo
  const int nameW = (int)strlen(pl.name) * CH_W;
  int nx = pl.x + pl.r + 3;
  if (nx + nameW > LR_W - 3) nx = pl.x - pl.r - CH_W - 4 - nameW;
  if (nx < 3) nx = 3;
  textAt(nx, pl.y - CH_H / 2, pl.name, INK_DIM);
}

// raketa: trojuhelnik ve smeru letu
static void drawShip(float x, float y, float dx, float dy) {
  const float px = -dy, py = dx;   // kolmice
  const float n = LR_H / 30.0f, b = LR_H / 50.0f;   // delka spicky, pulka zakladny
  lr->fillTriangle((int)lroundf(x + dx * n), (int)lroundf(y + dy * n),
                   (int)lroundf(x - dx * b + px * b), (int)lroundf(y - dy * b + py * b),
                   (int)lroundf(x - dx * b - px * b), (int)lroundf(y - dy * b - py * b), INK_FULL);
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
  textAt(startX - 5 * CH_W / 2, startY + 4, "START", INK_FAINT);

  for (int i = 0; i < PLANET_COUNT; i++)
    drawPlanet(planets[i], i, i < seg || phase == PH_DONE);

  if (phase == PH_DONE) {
    const Planet &last = planets[PLANET_COUNT - 1];
    drawShip(last.x, last.y - last.r - 5, 0, -1);
  } else {
    drawShip(sx, sy, sdx, sdy);
  }
}

// ---------------- 3. zasoby ----------------

static void drawBar(int y, const char *label, float pct, bool warn) {
  textAt(4, y, label, INK_DIM);
  snprintf(txt, sizeof(txt), "%3.0f %%", pct);
  textRight(LR_W - 4, y, txt, INK_FULL);
  if (warn && blink()) textAt(4 + (int)strlen(label) * CH_W + 6, y, "!!!", INK_FULL);

  const int bx = 4, by = y + CH_H + 1, bw = LR_W - 8, bh = (ROW_H > 9 ? 16 : (crt.scale >= 4 ? 8 : 11));
  lr->drawRect(bx, by, bw, bh, INK_DIM);
  const int blocks = 20, gap = 1;
  const int blockW = (bw - 4 - (blocks - 1) * gap) / blocks;
  const int filled = (int)lroundf(pct / 100.0f * blocks);
  for (int i = 0; i < filled && i < blocks; i++)
    lr->fillRect(bx + 2 + i * (blockW + gap), by + 2, blockW, bh - 4, INK_BRIGHT);
}

static void drawSupplies(int screen) {
  frameCommon("ZASOBY", screen);
  const int step = (STATUS_Y - 6 - CONTENT_Y) / 4;
  drawBar(CONTENT_Y + 2,            "JIDLO",   foodPct(),  foodPct() < SUPPLY_WARN_PCT);
  drawBar(CONTENT_Y + 2 + step,     "PITI",    drinkPct(), drinkPct() < SUPPLY_WARN_PCT);
  drawBar(CONTENT_Y + 2 + 2 * step, "PALIVO",  fuelPct(),  fuelPct() < SUPPLY_WARN_PCT);
  drawBar(CONTENT_Y + 2 + 3 * step, "TOALETA", wastePct(), wastePct() > 100 - SUPPLY_WARN_PCT);
}

// ---------------- 4. parametry CRT ----------------

// nahodne parametry filtru (mrizka se nemeni - ta jde tahem nahoru/dolu)
static void crtRandomize() {
  crt.decay    = random(120, 231);
  crt.bloom    = random(0, 201);
  crt.glow     = random(0, 141);
  crt.rowTop   = random(120, 257);
  crt.rowBot   = random(20, 201);
  crt.rowBleed = random(0, 201);
  crt.grille   = random(60, 257);
  crt.vignette = random(0, 61) / 100.0f;
  crt.flicker  = random(0, 41);
  crt.hum      = random(0, 11);
  crt.noise    = random(0, 21);
  crt.gamma    = random(60, 111) / 100.0f;
  crtBuildTables();
}

static void drawCrt(int screen) {
  frameCommon("CRT", screen);
  rowY = CONTENT_Y;
  snprintf(txt, sizeof(txt), "%dX%d", crt.scale, crt.scale);       row("MRIZKA", txt);
  snprintf(txt, sizeof(txt), "%d", crt.decay);                    row("DOSVIT", txt);
  snprintf(txt, sizeof(txt), "%d", crt.bloom);                    row("BLOOM", txt);
  snprintf(txt, sizeof(txt), "%d", crt.glow);                     row("ZARE", txt);
  snprintf(txt, sizeof(txt), "%d", crt.rowTop);                   row("RADEK HORNI", txt);
  snprintf(txt, sizeof(txt), "%d", crt.rowBot);                   row("RADEK DOLNI", txt);
  snprintf(txt, sizeof(txt), "%d", crt.rowBleed);                 row("PROSVIT", txt);
  snprintf(txt, sizeof(txt), "%d", crt.grille);                   row("MASKA", txt);
  snprintf(txt, sizeof(txt), "%.2f", crt.vignette);               row("VINETA", txt);
  snprintf(txt, sizeof(txt), "%d", crt.flicker);                  row("BLIKANI", txt);
  snprintf(txt, sizeof(txt), "%d", crt.hum);                      row("PRUH", txt);
  snprintf(txt, sizeof(txt), "%d", crt.noise);                    row("JISKRY", txt);
  snprintf(txt, sizeof(txt), "%.2f", crt.gamma);                  row("GAMA", txt);
  rowY += 2;
  row("TUK = NAHODNE", "");
  row("NAHORU/DOLU = MRIZKA", "");
}

static void drawScreen(int screen) {
  layoutUpdate();
  switch (screen) {
    case 0:  drawTelemetry(screen); break;
    case 1:  drawNavigation(screen); break;
    case 2:  drawSupplies(screen); break;
    default: drawCrt(screen); break;
  }
}
