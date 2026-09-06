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
#include "rat_hiscore.h"

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

static void debugPrint(uint32_t usFrame, uint32_t usDraw, uint32_t usPresent) {
#if DEBUG_PERIOD_MS > 0
  static uint32_t lastMs = 0, fSum = 0, dSum = 0, pSum = 0;
  static int frames = 0;
  frames++;
  fSum += usFrame; dSum += usDraw; pSum += usPresent;
  const uint32_t now = millis();
  if (now - lastMs < DEBUG_PERIOD_MS) return;
  lastMs = now;
  USBSerial.printf("cpu %lu MHz  fps %.1f  us snimek %lu kresleni %lu pruhy %lu (cekani %lu vypocet %lu z toho blend %lu)  stav %d body %d\n",
                   (unsigned long)getCpuFrequencyMhz(), 1e6f * frames / fSum, fSum / frames, dSum / frames, pSum / frames,
                   usWait / frames, usCompose / frames, usBlend / frames, state, score);
  frames = 0; fSum = dSum = pSum = 0; usWait = usCompose = usBlend = 0;
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
  for (int i = 0; i < EXTRA_LEGEND_COUNT; i++) legend[(int)EXTRA_LEGEND[i].ch] = EXTRA_LEGEND[i].idx;
  pinMode(0, INPUT_PULLUP);
  btnPrev = (digitalRead(0) == LOW);
  touchPrev = touchDown;
  if (!crtInit(gfx)) {
    USBSerial.println("CRT init fail (canvas/SPI)");
    return false;
  }
  audioOk = ratAudioBegin();
  USBSerial.println(audioOk ? "ES8311 OK" : "ES8311 init fail - bez zvuku");
  if (hiscoreBegin()) {
    best = hiscoreLoad();
    USBSerial.printf("SD karta OK, nejlepsi %d\n", best);
  } else USBSerial.println("SD karta neni - nejlepsi skore se neuklada");
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
  if (bestDirty) { bestDirty = false; hiscoreSave(best); }
  if (audioOk) ratAudioPump();
  else pendingSound = SND_NONE;
  const uint32_t t1 = micros();
  drawScene();
  const uint32_t t2 = micros();
  crtPresent();
  debugPrint(micros() - now, t2 - t1, micros() - t2);
}

void ratEnd() {
  crtEnd();
  if (audioOk) ratAudioEnd();
  hiscoreEnd();
}
