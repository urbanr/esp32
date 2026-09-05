#pragma once

#include <Arduino.h>
#include <math.h>
#include "../common/amoled_audio.h"
#include "config.h"
#include "rat_world.h"

// ===================================================================
// Zvuky hry (synteza v tasku sdileneho modulu ../common/amoled_audio.h):
// skok, vysoky skok, dopad, sebrani odpadku, zasah, pad do vody,
// konec hry, start. Zadna hudba.
// ===================================================================

static volatile uint8_t sndPending = SND_NONE;

static uint32_t sndRng = 0x2545F491;
static inline float white() {
  sndRng ^= sndRng << 13;
  sndRng ^= sndRng >> 17;
  sndRng ^= sndRng << 5;
  return (int32_t)sndRng * (1.0f / 2147483648.0f);
}

struct Fx { uint8_t type; uint32_t n, len; float ph; };
static Fx fx[4];

static void fxStart(uint8_t type) {
  float secs;
  switch (type) {
    case SND_JUMP:      secs = 0.14f; break;
    case SND_JUMP_HIGH: secs = 0.22f; break;
    case SND_LAND:      secs = 0.05f; break;
    case SND_COLLECT:   secs = 0.18f; break;
    case SND_HIT:       secs = 0.35f; break;
    case SND_SPLASH:    secs = 0.45f; break;
    case SND_OVER:      secs = 1.1f; break;
    default:            secs = 0.5f; break;   // SND_START
  }
  for (int i = 0; i < 4; i++) {
    if (fx[i].type == SND_NONE) {
      fx[i] = { type, 0, (uint32_t)(secs * AUDIO_RATE), 0 };
      return;
    }
  }
}

static inline float tone(Fx &f, float hz) {
  f.ph += hz * (TWO_PI / AUDIO_RATE);
  if (f.ph > TWO_PI) f.ph -= TWO_PI;
  return sinf(f.ph);
}

// "8bitovy" obdelnik misto sinu pro vesele pipnuti
static inline float square(Fx &f, float hz) { return tone(f, hz) >= 0 ? 1.0f : -1.0f; }

static float fxSample(Fx &f, float lp) {
  if (f.type == SND_NONE) return 0;
  const float t = f.n / (float)AUDIO_RATE;
  const float T = f.len / (float)AUDIO_RATE;
  const float u = t / T;
  float s = 0;
  switch (f.type) {
    case SND_JUMP:      s = square(f, 320 + 700 * u) * 0.22f * (1.0f - u); break;
    case SND_JUMP_HIGH: s = square(f, 380 + 1100 * u) * 0.22f * (1.0f - u * 0.7f); break;
    case SND_LAND:      s = lp * 1.5f * (1.0f - u); break;
    case SND_COLLECT:   s = square(f, u < 0.5f ? 880 : 1320) * 0.2f * (1.0f - u * 0.5f); break;
    case SND_HIT:       s = white() * 0.5f * (1.0f - u) + tone(f, 110 - 40 * u) * 0.4f * (1.0f - u); break;
    case SND_SPLASH:    s = lp * 2.5f * sinf(PI * u) + white() * 0.15f * (1.0f - u); break;
    case SND_OVER: {
      const float hz = u < 0.33f ? 660 : (u < 0.66f ? 520 : 390);
      s = square(f, hz) * 0.22f * (u < 0.9f ? 1.0f : (1.0f - u) * 10.0f);
      break;
    }
    default:            s = square(f, u < 0.5f ? 660 : 990) * 0.2f; break;   // start
  }
  if (++f.n >= f.len) f.type = SND_NONE;
  return s;
}

static void ratAudioFill(int16_t *buf, int frames) {
  static float lp = 0;
  if (sndPending != SND_NONE) {
    fxStart(sndPending);
    sndPending = SND_NONE;
  }
  for (int i = 0; i < frames; i++) {
    const float w = white();
    lp += (w - lp) * 0.12f;
    float s = 0;
    for (int k = 0; k < 4; k++) s += fxSample(fx[k], lp);
    s = s / (1.0f + fabsf(s));
    const int16_t v = (int16_t)(s * 32000.0f * AUDIO_MASTER);
    buf[2 * i] = v;
    buf[2 * i + 1] = v;
  }
}

static bool ratAudioBegin() {
  for (int i = 0; i < 4; i++) fx[i].type = SND_NONE;
  return audioBegin(ratAudioFill, AUDIO_VOLUME);
}

// zvuk zadany hrou (rat_world.h: pendingSound) predat tasku
static inline void ratAudioPump() {
  if (pendingSound != SND_NONE) {
    sndPending = pendingSound;
    pendingSound = SND_NONE;
  }
}

static void ratAudioEnd() { audioEnd(); }
