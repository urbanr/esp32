#pragma once

#include <Arduino.h>
#include <Wire.h>
#include "ESP_I2S.h"
#include "pin_config.h"

// ===================================================================
// Zvuk: kodek ES8311 (I2C 0x18, jen prehravani) + I2S pres ESP_I2S,
// zesilovac na pinu PA. Aplikace dodava funkci, ktera plni bloky
// stereo vzorku (16 bit, AUDIO_RATE); vola se z vlastniho tasku na
// jadre 0, zapis do I2S blokuje a tim udava tempo. Inicializace kodeku
// podle prikladu Waveshare 15_ES8311 (ovladac Espressif es8311.c),
// MCLK = 256 fs. Wire musi byt otevreny (hwInit()). Obsahuje definice -
// includovat jen z jednoho modulu aplikace (jejiho *_audio.h).
// ===================================================================

#ifndef AUDIO_RATE
#define AUDIO_RATE   16000
#endif
#define AUDIO_BLOCK  256          // vzorku (snimku) na blok
#define ES8311_ADDR  0x18

typedef void (*AudioFillFn)(int16_t *stereo, int frames);

static I2SClass audioI2s;
static TaskHandle_t audioTask = nullptr;
static volatile bool audioRun = false;
static AudioFillFn audioFill = nullptr;

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
static bool es8311Init(int volume) {
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
  if (volume < 1) volume = 1;
  if (volume > 100) volume = 100;
  return esWrite(0x32, (uint8_t)(volume * 256 / 100 - 1));   // hlasitost DAC
}

static void audioTaskFn(void *) {
  static int16_t buf[AUDIO_BLOCK * 2];
  while (audioRun) {
    audioFill(buf, AUDIO_BLOCK);
    audioI2s.write((uint8_t *)buf, sizeof(buf));     // blokuje = tempo tasku
  }
  audioTask = nullptr;
  vTaskDelete(nullptr);
}

// zesilovac, I2S, kodek, task; false = zvuk nedostupny (aplikace bezi dal)
static bool audioBegin(AudioFillFn fill, int volume) {
  audioFill = fill;
  pinMode(PA, OUTPUT);
  digitalWrite(PA, HIGH);
  audioI2s.setPins(I2S_BCK_IO, I2S_WS_IO, I2S_DO_IO, I2S_DI_IO, I2S_MCK_IO);
  if (!audioI2s.begin(I2S_MODE_STD, AUDIO_RATE, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO)) return false;
  if (!es8311Init(volume)) return false;
  audioRun = true;
  return xTaskCreatePinnedToCore(audioTaskFn, "audio", 8192, nullptr, 1, &audioTask, 0) == pdPASS;
}

static void audioEnd() {
  audioRun = false;
  while (audioTask) delay(5);   // task dokonci blok a skonci
  audioI2s.end();
  digitalWrite(PA, LOW);
}
