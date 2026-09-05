#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <math.h>
#include "ESP_I2S.h"
#include "pin_config.h"
#include "config.h"

// ===================================================================
// Zvuk: kodek ES8311 (I2C 0x18, jen prehravani) + I2S pres ESP_I2S,
// zesilovac na pinu PA. Zvuky se syntetizuji v samostatnem tasku
// (jadro 0): motor = filtrovany sum podle rychlosti, efekty = start,
// prulet kolem planety (syceni, manevrovaci pipnuti), pristani,
// pipnuti pauzy, fanfara v cili. Inicializace kodeku podle prikladu
// Waveshare 15_ES8311 (ovladac Espressif es8311.c), 16 kHz, MCLK 256 fs.
// ===================================================================

enum : uint8_t { EV_NONE = 0, EV_LAUNCH, EV_FLYBY, EV_LANDING, EV_BEEP, EV_DONE };

#define ES8311_ADDR   0x18
#define AUDIO_BLOCK   256

static I2SClass audioI2s;
static TaskHandle_t audioTask = nullptr;
static volatile bool audioRun = false;
static volatile float engineTarget = 0;     // 0..1, sila motoru
static volatile uint8_t pendingEvent = EV_NONE;

static bool esWrite(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(ES8311_ADDR);
  Wire.write(reg);
  Wire.write(val);
  return Wire.endTransmission() == 0;
}

static bool esRead(uint8_t reg, uint8_t &val) {
  Wire.beginTransmission(ES8311_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((int)ES8311_ADDR, 1) != 1) return false;
  val = Wire.read();
  return true;
}

static bool esUpdate(uint8_t reg, uint8_t keepMask, uint8_t val) {
  uint8_t v;
  if (!esRead(reg, v)) return false;
  return esWrite(reg, (v & keepMask) | val);
}

// slave, I2S 16 bit, MCLK z pinu = 256 * fs (4.096 MHz pri 16 kHz):
// delice z tabulky ovladace pro {4096000, 16000}
static bool es8311Init() {
  if (!esWrite(0x00, 0x1F) || !esWrite(0x00, 0x00) || !esWrite(0x00, 0x80)) return false;
  esWrite(0x01, 0x3F);            // vsechny hodiny zapnute, MCLK z pinu MCLK
  esUpdate(0x06, 0xDF, 0x00);     // SCLK neinvertovany
  esUpdate(0x02, 0x07, 0x00);     // pre_div 1, nasobic 1x
  esWrite(0x03, 0x10);            // single speed, ADC osr
  esWrite(0x04, 0x10);            // DAC osr
  esWrite(0x05, 0x00);            // adc/dac div 1
  esUpdate(0x06, 0xE0, 0x03);     // bclk div 4
  esUpdate(0x07, 0xC0, 0x00);     // lrck div
  esWrite(0x08, 0xFF);
  esUpdate(0x00, 0xBF, 0x00);     // slave
  esWrite(0x09, 0x0C);            // SDP in 16 bit
  esWrite(0x0A, 0x0C);            // SDP out 16 bit
  esWrite(0x0D, 0x01);            // analogova cast zapnuta
  esWrite(0x0E, 0x02);
  esWrite(0x12, 0x00);            // DAC zapnuty
  esWrite(0x13, 0x10);            // vystup do HP driveru
  esWrite(0x1C, 0x6A);
  esWrite(0x37, 0x08);            // DAC bez ekvalizeru
  return esWrite(0x32, (uint8_t)(AUDIO_VOLUME * 256 / 100 - 1));   // hlasitost DAC
}

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

static void audioTaskFn(void *) {
  static int16_t buf[AUDIO_BLOCK * 2];
  float engine = 0, brown = 0, lp = 0, subPh = 0;
  while (audioRun) {
    if (pendingEvent != EV_NONE) {
      fxStart(pendingEvent);
      pendingEvent = EV_NONE;
    }
    for (int i = 0; i < AUDIO_BLOCK; i++) {
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
    audioI2s.write((uint8_t *)buf, sizeof(buf));     // blokuje = tempo tasku
  }
  audioTask = nullptr;
  vTaskDelete(nullptr);
}

// zesilovac, I2S, kodek, task; false = zvuk nedostupny (aplikace bezi dal)
static bool audioBegin() {
  pinMode(PA, OUTPUT);
  digitalWrite(PA, HIGH);
  audioI2s.setPins(I2S_BCK_IO, I2S_WS_IO, I2S_DO_IO, I2S_DI_IO, I2S_MCK_IO);
  if (!audioI2s.begin(I2S_MODE_STD, AUDIO_RATE, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO)) return false;
  if (!es8311Init()) return false;
  for (int i = 0; i < 3; i++) fx[i].type = EV_NONE;
  engineTarget = 0;
  pendingEvent = EV_NONE;
  audioRun = true;
  return xTaskCreatePinnedToCore(audioTaskFn, "audio", 8192, nullptr, 1, &audioTask, 0) == pdPASS;
}

static inline void audioEvent(uint8_t ev) { pendingEvent = ev; }
static inline void audioEngine(float level) { engineTarget = level < 0 ? 0 : (level > 1 ? 1 : level); }

static void audioEnd() {
  audioRun = false;
  while (audioTask) delay(5);   // task dokonci blok a skonci
  audioI2s.end();
  digitalWrite(PA, LOW);
}
