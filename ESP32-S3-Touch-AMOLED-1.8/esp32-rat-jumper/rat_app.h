#pragma once

#include <Arduino.h>
#include <esp_random.h>
#include "../common/amoled_app.h"
#include "config.h"
#include "rat_palette.h"
#include "rat_sprites.h"
#include "rat_crt.h"
#include "rat_world.h"
#include "rat_render.h"
#include "rat_audio.h"

// ===================================================================
// Modul hry Krysa skokan: ratBegin() / ratLoop() / ratEnd(). Pouziva ho
// samostatny sketch i launcher (tam jako vlastni prekladova jednotka,
// proto jsou tyto tri funkce jedine ne-static symboly). Hardware
// a dotyk vlastni sketch (../common). Vstup: tuknuti (stisk) = skok,
// BOOT = vysoky skok; v uvodu a po konci tuknuti startuje hru.
// ===================================================================

static uint32_t lastUs = 0;
static bool touchPrev = false, btnPrev = false;
static bool audioOk = false;

static void debugPrint(float dt) {
#if DEBUG_PERIOD_MS > 0
  static uint32_t lastMs = 0;
  static int frames = 0;
  static float timeSum = 0;
  frames++;
  timeSum += dt;
  const uint32_t now = millis();
  if (now - lastMs < DEBUG_PERIOD_MS) return;
  lastMs = now;
  USBSerial.printf("fps %.1f  stav %d  body %d  rychlost %.0f\n", frames / timeSum, state, score, speed);
  frames = 0;
  timeSum = 0;
#endif
}

static void handleInput() {
  const bool tap = touchDown && !touchPrev;                       // stisk prstu
  touchPrev = touchDown;
  const bool btn = (digitalRead(0) == LOW);
  const bool press = btn && !btnPrev;                             // stisk BOOT
  btnPrev = btn;
  if (!tap && !press) return;
  if (state == ST_PLAY) ratJump(press);
  else gameStart();                                               // uvod / konec -> nova hra
}

bool ratBegin() {
  randomSeed(esp_random());
  paletteInit();
#if SPRITE_SET == SPRITES_ASTRA
  for (int i = 0; i < ASTRA_LEGEND_COUNT; i++) legend[(int)ASTRA_LEGEND[i].ch] = ASTRA_LEGEND[i].idx;
#endif
  pinMode(0, INPUT_PULLUP);
  btnPrev = (digitalRead(0) == LOW);
  touchPrev = touchDown;
  if (!crtInit(gfx)) {
    USBSerial.println("CRT init fail (canvas/SPI)");
    return false;
  }
  audioOk = ratAudioBegin();
  USBSerial.println(audioOk ? "ES8311 OK" : "ES8311 init fail - bez zvuku");
  worldReset();
  spawnAhead();
  state = ST_TITLE;
  lastUs = micros();
  return true;
}

void ratLoop() {
  const uint32_t now = micros();
  float dt = (now - lastUs) * 1e-6f;
  lastUs = now;
  if (dt > 0.05f) dt = 0.05f;

  handleInput();
  worldUpdate(dt);
  if (audioOk) ratAudioPump();
  else pendingSound = SND_NONE;
  drawScene();
  crtPresent();
  debugPrint(dt);
}

void ratEnd() {
  crtEnd();
  if (audioOk) ratAudioEnd();
}
