#pragma once

#include <Arduino.h>
#include <esp_random.h>
#include "../common/amoled_app.h"
#include "config.h"
#include "sand_palette.h"
#include "sand_sim.h"
#include "sand_input.h"
#include "sand_render.h"

// ===================================================================
// Modul aplikace pisek: sandBegin() / sandLoop() / sandEnd(). Pouziva
// ho samostatny sketch (esp32-amoled-sand.ino) i launcher. Hardware
// (gfx, USBSerial) a dotyk (touchDown/touchX/touchY) vlastni sketch,
// modul je jen pouziva pres amoled_app.h. Jedine ne-static symboly
// jsou tyto tri funkce - v launcheru je modul vlastni prekladova
// jednotka a cokoli dalsiho by kolidovalo s ostatnimi aplikacemi.
// ===================================================================

static uint32_t lastUs = 0;
static float timeAcc = 0;

// plna inicializace a prekresleni; displej uz je cerny s nastavenym jasem
bool sandBegin() {
  if (!inputBeginIMU()) USBSerial.println("QMI8658 init fail");
  else USBSerial.println("QMI8658 OK");

  buildPalette();
  gfx->fillScreen(bgColor565);

  pinMode(POUR_BUTTON_PIN, INPUT_PULLUP);

  simInit(esp_random());
  timeAcc = 0;
  lastUs = micros();
  return true;
}

void sandLoop() {
  const uint32_t now = micros();
  float dt = (now - lastUs) * 1e-6f;
  lastUs = now;
  if (dt > 0.1f) dt = 0.1f;
  timeAcc += dt;

  inputRead();
  simSetGravity(inGX, inGY);

  const bool pouring = (digitalRead(POUR_BUTTON_PIN) == LOW);

  bool ticked = false;
  while (timeAcc >= TICK_DT) {
    simTick(touchDown, touchX, touchY, inShake, pouring);
    timeAcc -= TICK_DT;
    ticked = true;
  }
  if (ticked) renderDirty(gfx);
}

void sandEnd() {
  // kresli jen pres gfx, nic k uvolneni
}
