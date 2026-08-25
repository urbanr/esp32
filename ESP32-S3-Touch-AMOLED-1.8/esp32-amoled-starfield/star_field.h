#pragma once

#include <Arduino.h>
#include "config.h"

// ===================================================================
// Hvezdne pole: koule polomeru FIELD_RADIUS kolem oka. Hvezda ma
// souradnice (u, v, w) - u, v do strany / nahoru, w dopredu (dalka).
// Leti po ose w k divakovi; kdyz opusti kouli vzadu, objevi se vpredu
// na nahodne pozici. Rovnomerne rozlozeni v kruhovem prurezu + stejna
// rychlost = rovnomerna hustota v cele kouli.
// ===================================================================

struct Star {
  float u, v, w;
  float wMax;          // w, pri kterem hvezda vstupuje / opousti kouli
  uint8_t r, g, b;     // zakladni barva (plny jas)
};

static Star stars[STAR_COUNT];

static inline float frand() { return random(0x10000) / 65536.0f; }

static void starRespawn(Star &s, bool anywhere) {
  const float r = FIELD_RADIUS * sqrtf(frand());
  const float a = frand() * TWO_PI;
  s.u = r * cosf(a);
  s.v = r * sinf(a);
  s.wMax = sqrtf(FIELD_RADIUS * FIELD_RADIUS - r * r);
  s.w = anywhere ? (frand() * 2 - 1) * s.wMax : s.wMax;

  // barva: bila, u casti hvezd nahodne priklonena k barve z palety
  static const uint8_t tints[][3] = STAR_TINT_COLORS;
  const int nTints = sizeof(tints) / sizeof(tints[0]);
  float t = 0;
  int ti = 0;
  if (random(100) < STAR_TINT_PROB_PCT) {
    t = frand() * STAR_TINT_MAX;
    ti = random(nTints);
  }
  s.r = (uint8_t)(255 + (tints[ti][0] - 255) * t);
  s.g = (uint8_t)(255 + (tints[ti][1] - 255) * t);
  s.b = (uint8_t)(255 + (tints[ti][2] - 255) * t);
}

static void fieldInit() {
  for (int i = 0; i < STAR_COUNT; i++) starRespawn(stars[i], true);
}

static void fieldUpdate(float dt) {
  const float step = STAR_SPEED * dt;
  for (int i = 0; i < STAR_COUNT; i++) {
    Star &s = stars[i];
    s.w -= step;
    if (s.w < -s.wMax) starRespawn(s, false);
  }
}
