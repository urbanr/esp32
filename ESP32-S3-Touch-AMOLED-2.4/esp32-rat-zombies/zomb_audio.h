#pragma once

#include <Arduino.h>
#include <math.h>
#include "../common/amoled_audio.h"
#include "config.h"
#include "zomb_world.h"

// ===================================================================
// Zvuky (synteza v tasku ../common/amoled_audio.h): motor podle
// rychlosti (obdelnik s nizkym tonem), skok, dopad, mince, spravna /
// spatna odpoved, zombik (mlasknuti + sum), svih a uder bouraku,
// kanystr, dosel benzin, nakup, fanfara levelu, sebrani bouraku, naraz.
// ===================================================================

static volatile uint8_t sndPending = SND_NONE;
static volatile float engineLevel = 0;   // 0-1 podle rychlosti (nastavuje hra)
static volatile uint32_t audioBlocks = 0, audioPeak = 0;   // diagnostika: pocet bloku, max |vzorek|

static uint32_t sndRng = 0x2545F491;
static inline float white() {
  sndRng ^= sndRng << 13; sndRng ^= sndRng >> 17; sndRng ^= sndRng << 5;
  return (int32_t)sndRng * (1.0f / 2147483648.0f);
}

struct Fx { uint8_t type; uint32_t n, len; float ph; };
static Fx fx[4];

static void fxStart(uint8_t type) {
  float secs;
  switch (type) {
    case SND_JUMP:  secs = 0.22f; break;
    case SND_LAND:  secs = 0.06f; break;
    case SND_COIN:  secs = 0.12f; break;
    case SND_OK:    secs = 0.5f; break;
    case SND_BAD:   secs = 0.45f; break;
    case SND_ZOMB:  secs = 0.45f; break;
    case SND_SWING: secs = 0.25f; break;
    case SND_HIT:   secs = 0.3f; break;
    case SND_FUEL:  secs = 0.3f; break;
    case SND_EMPTY: secs = 1.2f; break;
    case SND_BUY:   secs = 0.25f; break;
    case SND_LEVEL: secs = 1.0f; break;
    case SND_PICK:  secs = 0.4f; break;
    default:        secs = 0.2f; break;   // SND_BUMP
  }
  for (int i = 0; i < 4; i++) if (fx[i].type == SND_NONE) { fx[i] = { type, 0, (uint32_t)(secs * AUDIO_RATE), 0 }; return; }
}

static inline float tone(Fx &f, float hz) {
  f.ph += hz * (TWO_PI / AUDIO_RATE);
  if (f.ph > TWO_PI) f.ph -= TWO_PI;
  return sinf(f.ph);
}
static inline float square(Fx &f, float hz) { return tone(f, hz) >= 0 ? 1.0f : -1.0f; }

static float fxSample(Fx &f, float lp) {
  if (f.type == SND_NONE) return 0;
  const float u = f.n / (float)f.len;
  float s = 0;
  switch (f.type) {
    case SND_JUMP:  s = square(f, 250 + 900 * u) * 0.32f * (1 - u * 0.8f); break;
    case SND_LAND:  s = lp * 1.5f * (1 - u); break;
    case SND_COIN:  s = square(f, u < 0.5f ? 1200 : 1600) * 0.25f * (1 - u * 0.5f); break;
    case SND_OK:    s = square(f, 500 + 1300 * u * u) * 0.25f * (u < 0.85f ? 1 : (1 - u) * 6.6f); break;   // od stredniho k vysokemu
    case SND_BAD:   s = square(f, 130) * 0.3f * ((u < 0.35f) || (u > 0.5f && u < 0.85f) ? 1.0f : 0.0f); break;   // hluboke dvoji pipnuti
    case SND_ZOMB:  s = white() * 0.6f * (1 - u) * (u < 0.15f ? 1.0f : 0.5f) + square(f, 70 - 30 * u) * 0.45f * (1 - u); break;   // mlasknuti + rana
    case SND_SWING: s = white() * 0.2f * sinf(PI * u); break;
    case SND_HIT:   s = white() * 0.45f * (1 - u) + square(f, 150) * 0.2f * (1 - u); break;
    case SND_FUEL:  s = tone(f, 500 + 300 * u) * 0.2f * (1 - u); break;
    case SND_EMPTY: s = square(f, 160 - 120 * u) * 0.18f * (1 - u) + white() * 0.1f * (u < 0.3f ? 1 : 0); break;
    case SND_BUY:   s = square(f, u < 0.5f ? 900 : 1350) * 0.18f; break;
    case SND_LEVEL: { const float hz = u < 0.25f ? 523 : (u < 0.5f ? 659 : (u < 0.75f ? 784 : 1046)); s = square(f, hz) * 0.2f * (u < 0.9f ? 1 : (1 - u) * 10); break; }
    case SND_PICK:  s = square(f, 400 + 200 * sinf(u * 12)) * 0.18f * (1 - u); break;
    default:        s = lp * 2.0f * (1 - u) + white() * 0.2f * (1 - u); break;   // naraz
  }
  if (++f.n >= f.len) f.type = SND_NONE;
  return s;
}

static void zombAudioFill(int16_t *buf, int frames) {
  static float lp = 0, eph = 0, eLvl = 0;
  if (sndPending != SND_NONE) { fxStart(sndPending); sndPending = SND_NONE; }
  for (int i = 0; i < frames; i++) {
    const float w = white();
    lp += (w - lp) * 0.12f;
    eLvl += (engineLevel - eLvl) * 0.002f;
    // motor: nizky obdelnik s "klepanim"
    const float hz = 45 + 90 * eLvl;
    eph += hz * (TWO_PI / AUDIO_RATE);
    if (eph > TWO_PI) eph -= TWO_PI;
    float s = (eph < PI * 0.6f ? 1.0f : -1.0f) * 0.12f * eLvl + lp * 0.08f * eLvl;
    for (int k = 0; k < 4; k++) s += fxSample(fx[k], lp);
    s = s / (1.0f + fabsf(s));
    const int16_t v = (int16_t)(s * 32000.0f * AUDIO_MASTER);
    buf[2 * i] = v;
    buf[2 * i + 1] = v;
    if ((uint32_t)abs(v) > audioPeak) audioPeak = abs(v);
  }
  audioBlocks++;
}

static bool zombAudioBegin() {
  for (int i = 0; i < 4; i++) fx[i].type = SND_NONE;
  return audioBegin(zombAudioFill, AUDIO_VOLUME);
}

static inline void zombAudioPump() {
  if (pendingSound != SND_NONE) { sndPending = pendingSound; pendingSound = SND_NONE; }
  engineLevel = state == ST_DRIVE || state == ST_EMPTY ? fminf(1.0f, ratV / 90.0f) * (fuel > 0 ? 1.0f : 0.3f) : 0;
}

static void zombAudioEnd() { audioEnd(); }
