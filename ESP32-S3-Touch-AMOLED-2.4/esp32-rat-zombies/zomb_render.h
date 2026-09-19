#pragma once

#include <Arduino.h>
#include <stdio.h>
#include "config.h"
#include "zomb_gfx.h"
#include "zomb_world.h"
#include "zomb_crt.h"
#include "zomb_crt_presets.h"

// ===================================================================
// Kresleni sceny do 8bitoveho bufferu `fb` (LW x LH): nebe s prechodem,
// mraky a holubi, vzdalene mesto, domy (paralaxa), tmavy pas pod domy,
// teren po sloupcich z pasu bahna / betonu, rekvizity, lide, prekazky,
// mince, znamenka, zombici, dily rozpadlych zombiku, krysa (natocena
// podle terenu) s bourakem, HUD a obrazovky (uvod, priklad, garaz,
// dosel benzin, level hotov).
// ===================================================================

static char txt[40];
static float camX = 0;

static inline int sx(float wx) { return (int)lroundf(wx - camX); }

// bitmapa s kotvou na (x, y) obrazovky; 0 = pruhledna
static void blit(const Bmp &b, int x, int y) {
  x -= b.ax; y -= b.ay;
  int i0 = 0, i1 = b.w, j0 = 0, j1 = b.h;
  if (x < 0) i0 = -x;
  if (x + b.w > LW) i1 = LW - x;
  if (y < 0) j0 = -y;
  if (y + b.h > LH) j1 = LH - y;
  if (i0 >= i1 || j0 >= j1) return;
  for (int j = j0; j < j1; j++) {
    const uint8_t *src = b.px + j * b.w;
    uint8_t *dst = fb + (y + j) * LW + x;
    for (int i = i0; i < i1; i++) if (src[i]) dst[i] = src[i];
  }
}

// levy horni roh na (x, y) bez kotvy (dily)
static inline void blitTL(const Bmp &b, int x, int y) { blit(b, x + b.ax, y + b.ay); }

// bitmapa otocena o uhel (rad) kolem kotvy; nearest neighbor
static void blitRot(const Bmp &b, int x, int y, float ang) {
  if (fabsf(ang) < 0.03f) { blit(b, x, y); return; }
  const float c = cosf(ang), s = sinf(ang);
  const int R = (int)(sqrtf((float)(b.w * b.w + b.h * b.h))) + 1;
  for (int dy = -R; dy <= R; dy++) {
    const int py = y + dy;
    if (py < 0 || py >= LH) continue;
    for (int dx = -R; dx <= R; dx++) {
      const int px = x + dx;
      if (px < 0 || px >= LW) continue;
      const int u = (int)lroundf(dx * c + dy * s) + b.ax;
      const int v = (int)lroundf(-dx * s + dy * c) + b.ay;
      if (u < 0 || u >= b.w || v < 0 || v >= b.h) continue;
      const uint8_t k = b.px[v * b.w + u];
      if (k) fb[py * LW + px] = k;
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

// ---------------- vrstvy ----------------

static void drawSky() {
  static const uint8_t bands[4] = { C_SKY_TOP, C_SKY_MID, C_SKY_LOW, C_SKY_HAZE };
  static const uint8_t ends[4] = { 14, 34, 58, BAND_Y };
  int y = 0;
  for (int b = 0; b < 4; b++) {
    for (; y < ends[b]; y++) memset(fb + y * LW, bands[b], LW);
  }
  for (; y < LH; y++) memset(fb + y * LW, C_BAND, LW);
  for (int i = 0; i < MAX_CLOUD; i++) blit(*CLOUDS[clouds[i].kind], (int)(clouds[i].x - camX * 0.1f), (int)clouds[i].y);
  for (int i = 0; i < MAX_PIGEON; i++) {
    const Pigeon &p = pigeons[i];
    blit(*PIGEON_FLY[(int)(p.animT * 9) & 3], sx(p.x), (int)p.y);
  }
}

static void drawBackdrop() {
  // vzdalene mesto, paralaxa 0.2, opakuje se
  const int off = (int)(camX * 0.2f);
  for (int x = -(off % 120) - 60; x < LW + 60; x += 120) blit(G_CITY_FAR_BG, x, BAND_Y - 1);   // dosedá na pas
  // domy, paralaxa 0.5 (svetove x domu je v polovicnim prostoru)
  const float cam2 = camX * 0.5f;
  for (int i = 0; i < nBld; i++) {
    const int x = (int)(blds[i].x - cam2);
    if (x < -30 || x > LW + 30) continue;
    blit(*BUILDINGS_BG[blds[i].kind], x, BAND_Y + 1);
  }
  // tmavsi pas pod domy (ma prirozeny stin nahore)
  memset(fb + BAND_Y * LW, C_BAND_LIGHT, LW);
}

// teren po sloupcich: pas bahna / betonu 14 bodu, pod nim tma
static void drawTerrain() {
  for (int x = 0; x < LW; x++) {
    const float wx = camX + x;
    const int gy = (int)lroundf(ground(wx));
    const Bmp &strip = isMud(wx) ? G_MUD_REPEAT : G_CONCRETE_REPEAT;
    const int col = ((int)wx % strip.w + strip.w) % strip.w;
    for (int j = 0; j < strip.h; j++) {
      const int y = gy + j;
      if (y < 0 || y >= LH) continue;
      const uint8_t k = strip.px[j * strip.w + col];
      fb[y * LW + x] = k ? k : C_EARTH_DARK;
    }
    for (int y = gy + strip.h; y < LH; y++) fb[y * LW + x] = C_EARTH_DARK;
  }
}

static void drawProps() {
  for (int i = 0; i < nByst; i++) {
    const int x = sx(bysts[i].x);
    if (x > -20 && x < LW + 20) blit(*BYSTANDERS[bysts[i].kind], x, (int)ground(bysts[i].x) + 1);
  }
  for (int i = 0; i < nProp; i++) {
    const int x = sx(props[i].x);
    if (x > -20 && x < LW + 20) blit(*NEAR_PROPS[props[i].kind], x, (int)ground(props[i].x) + 1);
  }
  // garaz na konci useku (a startovni vlevo)
  const Bmp &g = *GARAGES[garage % 3];
  const int gx = sx(segEnd);
  if (gx > -30 && gx < LW + 30) {
    blit(g, gx, (int)ground(segEnd) + 1);
    const char *nm = GARAGE_NAMES[level][garage];
    text(gx - (int)strlen(nm) * 3, (int)ground(segEnd) - g.h - 8, nm, C_YELLOW_LIGHT);
  }
  const int g0 = sx(segStart - 30);
  if (g0 > -30 && g0 < LW + 30) blit(*GARAGES[(garage + 2) % 3], g0, (int)ground(segStart) + 1);   // startovni garaz vlevo
}

static void drawObstacles() {
  for (int i = 0; i < nObst; i++) {
    const Obst &o = obsts[i];
    const int x = sx(o.x);
    if (x < -30 || x > LW + 30) continue;
    const Bmp *b = o.type == OB_LOG ? &G_LOG : (o.type == OB_PIPE ? &G_PIPE : &G_RAMP);
    blit(*b, x, (int)ground(o.x) + 1);
  }
}

static void drawItems() {
  for (int i = 0; i < nCoins; i++) {
    const Coin &c = coins[i];
    if (!c.alive) continue;
    const int x = sx(c.x);
    if (x < -10 || x > LW + 10) continue;
    const int bob = (int)(sinf(anim * 4 + c.x * 0.3f) * 1.5f);
    blit(G_COIN, x, (int)ground(c.x) + c.dy + 4 + bob);
  }
  for (int i = 0; i < nPick; i++) {
    const Pick &p = picks[i];
    if (!p.alive) continue;
    const int x = sx(p.x);
    if (x < -10 || x > LW + 10) continue;
    const Bmp *b = p.type == PK_PLUS ? &G_PICKUP_PLUS : (p.type == PK_MUL ? &G_PICKUP_MULTIPLY : (p.type == PK_FUEL ? &G_FUEL_CAN : &G_WRECKING_BALL));
    const int bob = (int)(sinf(anim * 3 + p.x * 0.2f) * 2);
    blit(*b, x, (int)ground(p.x) - 6 + bob);
  }
}

static void drawZombies() {
  for (int i = 0; i < nZomb; i++) {
    const Zomb &z = zombs[i];
    if (!z.alive) continue;
    const int x = sx(z.x);
    if (x < -25 || x > LW + 25) continue;
    const int f = (int)(z.animT * ZOMBIE_FPS[z.kind]) % ZOMBIE_FRAMES;
    blit(*ZOMBIE_WALK[z.kind][f], x, (int)ground(z.x) + 1);
  }
  for (int i = 0; i < MAX_PARTS; i++) {
    const Part &p = parts[i];
    if (!p.alive) continue;
    const int x = sx(p.x);
    if (x < -30 || x > LW + 30) continue;
    blitTL(*p.bmp, x, (int)p.y);
  }
}

static void drawRat() {
  const int x = sx(ratX), y = (int)lroundf(ratY);
  const int f = ((int)(wheelT / 4)) % RAT_FRAMES;
  const Bmp &b = ratV > 0.5f || airborne ? *RAT_DRIVE[rat][ballT > 0 ? 1 : 0][f] : *RAT_DRIVE[rat][ballT > 0 ? 1 : 0][0];
  blitRot(b, x, y, tilt);
  if (ballT > 0) {
    const int f2 = swingT >= 0 ? (int)(swingT * 15) : 0;
    const Bmp &sw = *SWING[f2 > 7 ? 7 : f2];
    // koren bouraku vuci krysi kotve (nenatoceny), viz grafika-v1/PRO_CLAUDA.md
    blitRot(sw, x - 13, y - 8, tilt);
  }
  for (int i = 0; i < MAX_BLOOD; i++) {
    const Blood &b = blood[i];
    if (b.t <= 0) continue;
    const int bx = sx(b.x), by = (int)b.y;
    if (bx < 0 || bx >= LW - 1 || by < 0 || by >= LH - 1) continue;
    const uint8_t c = (i & 1) ? C_RED : C_RED_DARK;
    fb[by * LW + bx] = c;
    fb[by * LW + bx + 1] = c;
    if (b.t > 0.3f) fb[(by + 1) * LW + bx] = c;
  }
  if (hitT > 0) blit(*IMPACT[(int)((0.3f - hitT) / 0.3f * 4) & 3], sx(hitX), hitY);
}

static void drawHud() {
  int fl = (int)lroundf(fuel * 10);
  if (fl < 0) fl = 0;
  if (fl > 10) fl = 10;
  blit(*FUEL_GAUGE[fl], 26, 8);
  if (fuel > 1.0f) text(41, 2, "+", C_GREEN_BRIGHT);
  snprintf(txt, sizeof(txt), "L%d", level + 1);
  text(46, 2, txt, C_WHITE);
  snprintf(txt, sizeof(txt), "%d", points);
  text(LW - 16 - (int)strlen(txt) * 6, 2, txt, C_YELLOW_LIGHT);
  blit(G_COIN, LW - 21 - (int)strlen(txt) * 6, 9);
  if (ballT > 0) {
    blit(G_WRECKING_BALL, 70, 13);
    snprintf(txt, sizeof(txt), "%d", (int)ballT + 1);
    text(76, 2, txt, C_GRAY_LIGHT);
  }
  if (msgT > 0 && state == ST_DRIVE) {
    snprintf(txt, sizeof(txt), "DO GARAZE %s", GARAGE_NAMES[level][garage]);
    textCenter(24, txt, C_WHITE);
  }
  if (crtMsgT > 0) textCenter(34, crtMsg, C_YELLOW_LIGHT);
}

static void drawBox(int x, int y, int w, int h) {
  lr->fillRoundRect(x, y, w, h, CORNER_R, C_DEEP);
  lr->drawRoundRect(x, y, w, h, CORNER_R, C_OUTLINE);
  lr->drawRoundRect(x + 1, y + 1, w - 2, h - 2, CORNER_R - 1, C_YELLOW);
}

// palec nahoru / krizek po odpovedi (13x13)
static const char *const ICON_THUMB[13] = {
  ".....kkk.....", "....kyyyk....", "....kyyyk....", "....kyyyk....", "....kyyykkkkk",
  "kkkkkyyyyyyyk", "kyyyyyyyyyyyk", "kyyyyyyyyyykk", "kyyyyyyyyyyyk", "kyyyyyyyyyykk",
  "kyyyyyyyyyyyk", ".kyyyyyyyyykk", "..kkkkkkkkkk." };
static void drawIcon(const char *const *rows, int x, int y, uint8_t fill) {
  for (int j = 0; j < 13; j++)
    for (int i = 0; i < 13; i++) {
      const char ch = rows[j][i];
      if (ch == '.') continue;
      const int px = x + i, py = y + j;
      if (px >= 0 && px < LW && py >= 0 && py < LH) fb[py * LW + px] = ch == 'k' ? C_OUTLINE : fill;
    }
}
static void drawCross(int x, int y) {
  for (int i = 0; i < 13; i++) {
    lr->fillRect(x + i, y + i, 2, 2, C_RED_BAD);
    lr->fillRect(x + 12 - i, y + i, 2, 2, C_RED_BAD);
  }
}

static void drawMath() {
  drawBox(2, 2, LW - 4, LH - 4);
  if (mChosen >= 0) {   // vysledek: palec nahoru / krizek, pak hra jede dal
    if (mChosen == mRight) drawIcon(ICON_THUMB, LW / 2 - 6, 30, C_GREEN_OK);
    else drawCross(LW / 2 - 6, 30);
    snprintf(txt, sizeof(txt), "%d %c %d = %d", mA, mOp ? 'x' : '+', mB, mAns[mRight]);
    textCenter(56, txt, C_WHITE, 2);
    textCenter(84, mChosen == mRight ? "+10% BENZINU" : "PRISTE!", mChosen == mRight ? C_GREEN_OK : C_RED_BAD);
    return;
  }
  snprintf(txt, sizeof(txt), "%d %c %d = ?", mA, mOp ? 'x' : '+', mB);
  textCenter(22, txt, C_WHITE, 2);
  static const Bmp *const ab[3] = { &G_ANSWER_A, &G_ANSWER_B, &G_ANSWER_C };
  for (int i = 0; i < 3; i++) {
    const int cx = i == 0 ? 27 : (i == 1 ? 75 : 123);   // krajni o 2 body ke stredu, aby nezasahovaly do ramecku
    lr->drawRoundRect(cx - 22, 46, 44, 52, 5, C_GRAY);
    blit(*ab[i], cx, 62);
    snprintf(txt, sizeof(txt), "%d", mAns[i]);
    text(cx - (int)strlen(txt) * 6, 72, txt, C_WHITE, 2);
  }
  textCenter(108, "TUKNI NA A, B NEBO C", C_YELLOW_LIGHT);
}

static void drawGarage() {
  drawBox(1, 1, LW - 2, LH - 2);
  if (tempGarage) {
    textCenter(6, "DOCASNA GARAZ", C_ORANGE_LIGHT);
    snprintf(txt, sizeof(txt), "BODY %d   ZPET K: %s", points, checkpointName());
  } else {
    snprintf(txt, sizeof(txt), "GARAZ %s", checkpointName());
    textCenter(6, txt, C_YELLOW_LIGHT);
    snprintf(txt, sizeof(txt), "BODY %d   DALSI: %s", points, GARAGE_NAMES[level][garage]);
  }
  textCenter(16, txt, C_WHITE);
  static const Bmp *const ic[3] = { &G_GARAGE_ENGINE, &G_GARAGE_FUEL_CAN, &G_GARAGE_WHEEL };
  static const char *const nm[3] = { "MOTOR", "NADRZ", "KOLA" };
  for (int i = 0; i < 3; i++) {
    const int cx = 27 + i * 48;
    blit(*ic[i], cx, 46);
    text(cx - (int)strlen(nm[i]) * 3, 63, nm[i], C_GRAY_LIGHT);
    snprintf(txt, sizeof(txt), "%d", lvl[rat][i]);
    text(cx - (int)strlen(txt) * 3, 72, txt, C_WHITE);
    const bool can = lvl[rat][i] < 10 && points >= upgradeCost(i);
    blit(G_BUTTON_UPGRADE, cx, 96);
    if (!can) lr->fillRect(cx - 5, 85, 11, 10, C_GRAY_DARK);   // sede = nelze
    if (lvl[rat][i] < 10) {
      snprintf(txt, sizeof(txt), "%d", upgradeCost(i));
      text(cx - (int)strlen(txt) * 3, 100, txt, can ? C_YELLOW_LIGHT : C_GRAY);
    } else text(cx - 9, 100, "MAX", C_GREEN_OK);
  }
  if (((int)(anim * 2) & 1)) textCenter(110, "TUKNI DOLE = JET", C_YELLOW_LIGHT);
  else textCenter(110, "PLUS = KOUPIT", C_GRAY_LIGHT);
  if (garageNoMoney) {
    drawBox(18, 36, LW - 36, 50);
    textCenter(44, "UZ NEMAS BODY", C_RED_BAD);
    snprintf(txt, sizeof(txt), "MAS %d", points);
    textCenter(56, txt, C_WHITE);
    textCenter(72, "TUKNI = JEDEME!", C_YELLOW_LIGHT);
  }
}

static void drawTitle() {
  drawBox(1, 1, LW - 2, LH - 2);
  textCenter(6, "KRYSY A ZOMBICI", C_YELLOW_LIGHT, 1);
  textCenter(16, "VYBER KRYSU", C_WHITE);
  for (int i = 0; i < RAT_KINDS; i++) {
    const int cx = 27 + i * 48;
    if (i < unlocked) {
      blit(*RAT_STILL[i], cx, 56);
      text(cx - (int)strlen(RAT_NAMES[i]) * 3, 62, RAT_NAMES[i], i == rat ? C_YELLOW_LIGHT : C_GRAY_LIGHT);
      snprintf(txt, sizeof(txt), "%d/%d/%d", lvl[i][0], lvl[i][1], lvl[i][2]);
      text(cx - (int)strlen(txt) * 3, 71, txt, C_GRAY);
    } else {
      lr->fillRect(cx - 18, 40, 36, 14, C_GRAY_DARK);
      text(cx - 15, 62, "ZAMCENO", C_GRAY);
      snprintf(txt, sizeof(txt), "LEVEL %d", i + 1);
      text(cx - 21, 71, txt, C_GRAY);
    }
    if (i == rat) lr->drawRect(cx - 23, 34, 46, 46, C_YELLOW);
  }
  snprintf(txt, sizeof(txt), "LEVEL %d  GARAZ %s", level + 1, GARAGE_NAMES[level][garage]);
  textCenter(88, txt, C_WHITE);
  snprintf(txt, sizeof(txt), "BODY %d", points);
  textCenter(98, txt, C_YELLOW_LIGHT);
  if (((int)(anim * 2) & 1)) textCenter(110, "TUKNI NA KRYSU = JET", C_YELLOW_LIGHT);
}

static void drawEmpty() {
  drawBox(20, 40, LW - 40, 34);
  textCenter(46, "DOSEL BENZIN", C_RED_BAD, 1);
  textCenter(58, "DOCASNA GARAZ", C_WHITE);
  snprintf(txt, sizeof(txt), "BODY %d", points);
  textCenter(66, txt, C_YELLOW_LIGHT);
}

static void drawLevelDone() {
  drawBox(10, 20, LW - 20, 80);
  textCenter(26, "LEVEL HOTOV!", C_GREEN_OK, 1);
  snprintf(txt, sizeof(txt), "BODY %d", points);
  textCenter(40, txt, C_YELLOW_LIGHT);
  if (unlocked > 1 && unlocked - 1 <= level) {
    snprintf(txt, sizeof(txt), "NOVA KRYSA: %s", RAT_NAMES[unlocked - 1]);
    textCenter(54, txt, C_WHITE);
  }
  if (level < LEVELS - 1 || garage == 0) {
    snprintf(txt, sizeof(txt), "DALSI: LEVEL %d", level + 1);
    textCenter(68, txt, C_GRAY_LIGHT);
  }
  if (((int)(anim * 2) & 1)) textCenter(86, "TUKNI = POKRACOVAT", C_YELLOW_LIGHT);
}

static void drawScene() {
  camX = ratX - RAT_SCREEN_X;
  drawSky();
  drawBackdrop();
  drawTerrain();
  drawProps();
  drawObstacles();
  drawItems();
  drawZombies();
  if (state != ST_TITLE) drawRat();
  drawHud();
  switch (state) {
    case ST_TITLE:      drawTitle(); break;
    case ST_MATH:       drawMath(); break;
    case ST_GARAGE:     drawGarage(); break;
    case ST_EMPTY:      drawEmpty(); break;
    case ST_LEVEL_DONE: drawLevelDone(); break;
    default: break;
  }
}
