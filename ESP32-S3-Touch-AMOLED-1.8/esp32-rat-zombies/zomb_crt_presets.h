#pragma once

#include <Arduino.h>
#include "config.h"
#include "zomb_crt.h"

// ===================================================================
// Nahodny CRT filtr (jako u rakety): swipe dolu kdekoli ve hre vylosuje
// novou kombinaci. Dva rezimy skladani (zomb_crt.h):
//   LINKY - mekke vodorovne radky s prosvitem sousednich radku a lehkym
//           vodorovnym rozmazanim (zare), bez RGB masky,
//   TECKY - tvrde scanlines, RGB maska (triady), putujici pruh, blikani.
// Vzdy navic gama palety a jiskry. Uklada se na SD kartu.
// ===================================================================

struct CrtParams { int mode, top, bot, bleed, blend, mask, flicker, hum, noise; float gain, gamma; };
// pojmenovane oblibene kombinace (swipe nahoru = navrat k nim, uklada se na kartu); prvni je vychozi
struct CrtFavorite { const char *name; CrtParams p; };
static const CrtFavorite CRT_FAVORITES[] = {
  { "crt-favorite-1", { 1, 235, 120, 120,  90,   0, 0, 0, 0, 1.00f, 1.0f } },   // mekke radky, zare, bez teck (uzivatel: super)
};
#define CRT_FAVORITE_COUNT ((int)(sizeof(CRT_FAVORITES) / sizeof(CRT_FAVORITES[0])))
static const CrtParams CRT_DEFAULT = CRT_FAVORITES[0].p;
static int crtFavorite = 0;
static CrtParams crtCur = CRT_DEFAULT;
static float crtMsgT = 0;   // jak dlouho jeste ukazovat popis
static char crtMsg[56] = "";

static void crtApply(const CrtParams &p) {
  crtCur = p;
  crtMode = p.mode;
  crtRowTop = p.top; crtRowBot = p.bot; crtRowBleed = p.bleed; crtBlend = p.blend;
  crtMask = p.mask; crtFlicker = p.flicker; crtMaskGain = p.gain; crtVignette = 0;
  crtGamma = p.gamma; crtHum = p.hum; crtNoise = p.noise;
  crtBuildTables();
  if (p.mode == 1) snprintf(crtMsg, sizeof(crtMsg), "LINKY %d/%d PROSVIT %d ROZMAZ %d MASKA %d JISKRY %d GAMA %d", p.top, p.bot, p.bleed, p.blend, p.mask, p.noise, (int)(p.gamma * 100));
  else snprintf(crtMsg, sizeof(crtMsg), "TECKY %d/%d MASKA %d PRUH %d BLIK %d GAMA %d", p.top, p.bot, p.mask, p.hum, p.flicker, (int)(p.gamma * 100));
}

// uplna nahoda: kazda slozka zvlast, nektere obcas vypnute
static void crtRandomize() {
  CrtParams p = CRT_DEFAULT;
  p.mode = random(100) < 70 ? 1 : 0;
  p.gamma = 0.8f + random(50) / 100.0f;                                       // 0.8 - 1.3
  p.noise = random(100) < 55 ? 0 : 5 + random(40);                            // jiskry
  if (p.mode == 1) {
    const int r = random(100);
    if (r < 40)      { p.top = 235 + random(21); p.bot = 150 + random(80); }   // jemne radky
    else             { p.top = 190 + random(50); p.bot = 60 + random(120); }   // vyrazne radky
    p.bleed = 60 + random(140);                                                // prosvit = zare do sousednich radku
    p.blend = random(100) < 25 ? 0 : 40 + random(140);                         // vodorovne rozmazani
    p.mask = random(100) < 45 ? 0 : 60 + random(160);                          // RGB tecky i s rozmazanim
    p.gain = 1.0f + p.mask / 256.0f * 0.7f;
    p.hum = 0; p.flicker = 0;
  } else {
    const int r = random(100);
    if (r < 20)      { p.top = 256; p.bot = 256; }                              // bez scanlines
    else if (r < 55) { p.top = 200 + random(56); p.bot = 120 + random(100); }
    else             { p.top = 150 + random(80); p.bot = 20 + random(90); }
    const int m = random(100);
    p.mask = m < 30 ? 0 : (m < 65 ? 40 + random(80) : 120 + random(110));      // 0 = bez teck / svislych car
    p.gain = 1.0f + p.mask / 256.0f * 0.7f;
    p.flicker = random(100) < 40 ? 0 : random(35);
    p.hum = random(100) < 50 ? 0 : 40 + random(160);
    p.bleed = 0; p.blend = 0;
  }
  crtApply(p);
}

// swipe nahoru: dalsi oblibena kombinace
static void crtNextFavorite() {
  crtFavorite = (crtFavorite + 1) % CRT_FAVORITE_COUNT;
  crtApply(CRT_FAVORITES[crtFavorite].p);
  snprintf(crtMsg, sizeof(crtMsg), "%s", CRT_FAVORITES[crtFavorite].name);
}
