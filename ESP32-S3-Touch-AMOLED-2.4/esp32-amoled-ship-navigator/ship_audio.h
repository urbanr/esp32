#pragma once

#include <Arduino.h>
#include <math.h>
#include "../common/amoled_audio.h"
#include "config.h"

// ===================================================================
// Zvuky rakety: synteza v tasku ze sdileneho modulu ../common/
// amoled_audio.h (ES8311 + I2S). Motor = filtrovany sum podle rychlosti,
// efekty = start, prulet kolem planety (syceni, manevrovaci pipnuti),
// pristani, pipnuti pauzy, fanfara v cili.
// ===================================================================

enum : uint8_t { EV_NONE = 0, EV_LAUNCH, EV_FLYBY, EV_LANDING, EV_BEEP, EV_DONE };

static volatile float engineTarget = 0;     // 0..1, sila motoru
static volatile uint8_t pendingEvent = EV_NONE;

// ---------------- synteza ----------------

static uint32_t sndRng = 0x9E3779B9;
static inline float white() {   // xorshift, -1..1
  sndRng ^= sndRng << 13;
  sndRng ^= sndRng >> 17;
  sndRng ^= sndRng << 5;
  return (int32_t)sndRng * (1.0f / 2147483648.0f);
}

struct Fx {
  uint8_t type;
  uint32_t n, len;   // vzorky
  float ph;          // faze tonu
};
static Fx fx[3];

static void fxStart(uint8_t type) {
  float secs;
  switch (type) {
    case EV_LAUNCH:  secs = 3.0f; break;
    case EV_FLYBY:   secs = 1.4f; break;
    case EV_LANDING: secs = 2.4f; break;
    case EV_BEEP:    secs = 0.12f; break;
    default:         secs = 0.9f; break;
  }
  for (int i = 0; i < 3; i++) {
    if (fx[i].type == EV_NONE) {
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

static inline float beepAt(Fx &f, float t, float from, float to, float hz) {
  return (t >= from && t < to) ? tone(f, hz) * 0.3f : 0;
}

// jeden vzorek efektu (-1..1); vraci 0 po skonceni a efekt uvolni
static float fxSample(Fx &f, float lp) {
  if (f.type == EV_NONE) return 0;
  const float t = f.n / (float)AUDIO_RATE;
  const float T = f.len / (float)AUDIO_RATE;
  float s = 0;
  switch (f.type) {
    case EV_LAUNCH: {   // hukot narusta, ton stoupa od 70 do 350 Hz
      const float env = t < 0.5f ? t / 0.5f : 1.0f - (t - 0.5f) / (T - 0.5f);
      s = tone(f, 70 + 280 * t / T) * 0.35f * env + lp * 1.2f * env;
      break;
    }
    case EV_FLYBY: {    // syceni manevrovacich trysek + dve pipnuti
      const float env = sinf(PI * t / T);
      s = white() * 0.28f * env + tone(f, 1400 - 900 * t / T) * 0.08f * env;
      s += beepAt(f, t, 0.05f, 0.13f, 880) + beepAt(f, t, 0.20f, 0.28f, 880);
      break;
    }
    case EV_LANDING: {  // klesajici ton, pak syknuti a tupy dopad
      if (t < 1.0f) s += tone(f, 600 - 480 * t) * 0.3f * (1.0f - t * 0.6f);
      if (t >= 0.9f) s += white() * 0.4f * expf(-(t - 0.9f) * 3.0f);
      if (t >= 1.0f) s += sinf((t - 1.0f) * TWO_PI * 55) * 0.6f * expf(-(t - 1.0f) * 5.0f);
      break;
    }
    case EV_BEEP:
      s = tone(f, 1000) * 0.3f;
      break;
    default: {          // fanfara: tri stoupajici tony
      s = beepAt(f, t, 0.0f, 0.25f, 660) + beepAt(f, t, 0.3f, 0.55f, 880) + beepAt(f, t, 0.6f, 0.9f, 1100);
      break;
    }
  }
  if (++f.n >= f.len) f.type = EV_NONE;
  return s;
}

// naplni blok stereo vzorku (vola task ze sdileneho modulu)
static void shipAudioFill(int16_t *buf, int frames) {
  static float engine = 0, brown = 0, lp = 0, subPh = 0;
  if (pendingEvent != EV_NONE) {
    fxStart(pendingEvent);
    pendingEvent = EV_NONE;
  }
  for (int i = 0; i < frames; i++) {
    engine += (engineTarget - engine) * 0.0004f;   // pomaly nabeh/dobeh motoru
    const float w = white();
    brown += (w - brown) * 0.035f;                  // hluboky hukot
    lp += (w - lp) * 0.25f;                         // syceni
    subPh += 46.0f * (TWO_PI / AUDIO_RATE);
    if (subPh > TWO_PI) subPh -= TWO_PI;
    float s = (brown * 4.0f + lp * 0.35f + sinf(subPh) * 0.25f) * engine * AUDIO_ENGINE_GAIN;
    for (int k = 0; k < 3; k++) s += fxSample(fx[k], lp);
    s = s / (1.0f + fabsf(s));                      // mekke omezeni
    const int16_t v = (int16_t)(s * 32000.0f * AUDIO_MASTER);
    buf[2 * i] = v;
    buf[2 * i + 1] = v;
  }
}

static bool shipAudioBegin() {
  for (int i = 0; i < 3; i++) fx[i].type = EV_NONE;
  engineTarget = 0;
  pendingEvent = EV_NONE;
  return audioBegin(shipAudioFill, AUDIO_VOLUME);
}

static inline void audioEvent(uint8_t ev) { pendingEvent = ev; }
static inline void audioEngine(float level) { engineTarget = level < 0 ? 0 : (level > 1 ? 1 : level); }

static void shipAudioEnd() { audioEnd(); }
