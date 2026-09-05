#pragma once

#include <Arduino.h>
#include <esp_random.h>
#include "../common/amoled_app.h"
#include "config.h"
#include "ship_crt.h"
#include "ship_sim.h"
#include "ship_input.h"
#include "ship_screens.h"
#include "ship_audio.h"

// ===================================================================
// Modul aplikace raketa: shipBegin() / shipLoop() / shipEnd(). Pouziva
// ho samostatny sketch i launcher (tam jako vlastni prekladova
// jednotka, proto jsou tyto tri funkce jedine ne-static symboly).
// Hardware a dotyk vlastni sketch (../common), displej je pri vstupu
// cerny s nastavenym jasem.
// ===================================================================

static uint32_t lastUs = 0;
static int screen = 0;
static uint8_t prevPhase = PH_WAIT;
static int prevSeg = 0;
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
  USBSerial.printf("fps %.1f  faze %d  t %.1f  obrazovka %d\n", frames / timeSum, phase, flightT, screen);
  frames = 0;
  timeSum = 0;
#endif
}

// zvukove udalosti z prechodu stavu letu
static void soundEvents() {
  if (phase != prevPhase) {
    if (phase == PH_RUN && (prevPhase == PH_WAIT || prevPhase == PH_DONE)) audioEvent(EV_LAUNCH);
    else if (phase == PH_DONE) audioEvent(EV_LANDING);
    else audioEvent(EV_BEEP);            // pauza / pokracovani
    prevPhase = phase;
  }
  const int seg = currentSegment();
  if (seg > prevSeg && phase == PH_RUN) audioEvent(EV_FLYBY);   // minuti planety
  prevSeg = seg;
  audioEngine(phase == PH_RUN ? 0.25f + 0.75f * tmSpeed / SPEED_MAX_KMS : 0);
}

bool shipBegin() {
  randomSeed(esp_random());
  inputBeginButton();
  if (!crtInit(gfx)) {
    USBSerial.println("CRT init fail (canvas/SPI)");
    return false;
  }
  lr->setTextWrap(false);
  audioOk = audioBegin();
  USBSerial.println(audioOk ? "ES8311 OK" : "ES8311 init fail - bez zvuku");
  simNewFlight();
  prevPhase = phase;
  prevSeg = 0;
  screen = 0;
  lastUs = micros();
  return true;
}

void shipLoop() {
  const uint32_t now = micros();
  float dt = (now - lastUs) * 1e-6f;
  lastUs = now;
  if (dt > 0.1f) dt = 0.1f;

  if (inputButtonPressed()) simButton();
  const int sw = inputSwipe();
  if (sw) screen = (screen + sw + SCREEN_COUNT) % SCREEN_COUNT;

  simUpdate(dt);
  if (audioOk) soundEvents();
  drawScreen(screen);
  crtPresent();
  debugPrint(dt);
}

void shipEnd() {
  // dokonci DMA, uvolni buffer, zastavi zvuk; pak smi kreslit launcher pres gfx
  crtEnd();
  if (audioOk) audioEnd();
}
