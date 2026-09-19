#pragma once

#include <Arduino.h>
#include "../common/amoled_app.h"
#include "config.h"
#include "star_input.h"
#include "star_field.h"
#include "star_render.h"

// ===================================================================
// Modul aplikace hvezdy: starBegin() / starLoop() / starEnd() - to, co
// byvalo v setup()/loop() samostatneho sketche. Pouziva ho samostatny
// sketch i launcher (tam jako vlastni prekladova jednotka, proto jsou
// tyto tri funkce jedine ne-static symboly modulu). Hardware
// (USBSerial, I2C, SPI sbernice, panel) uz inicializoval sketch
// pres hwInit(), displej je cerny s nastavenym jasem.
// ===================================================================

static uint32_t lastUs = 0;

// diagnostika: fps, casy fazi (us/snimek), stav IMU, orientace
static void debugPrint(float dt, uint32_t usSim, uint32_t usProject, uint32_t usRender) {
#if DEBUG_PERIOD_MS > 0
  static uint32_t lastMs = 0, simSum = 0, projSum = 0, rendSum = 0;
  static int frames = 0;
  static float timeSum = 0;
  frames++;
  timeSum += dt;
  simSum += usSim;
  projSum += usProject;
  rendSum += usRender;
  const uint32_t now = millis();
  if (now - lastMs < DEBUG_PERIOD_MS) return;
  lastMs = now;
  USBSerial.printf("fps %.1f  us sim %lu proj %lu render %lu  imu %d calib %d  |a| %.3f |w| %.2f az %+.2f  gerr %.1f gsign %+.3f  g(%+.2f %+.2f %+.2f)  flow(%+.2f %+.2f %+.2f)  dots %d\n",
                   frames / timeSum, simSum / frames, projSum / frames, rendSum / frames,
                   imuOk, inputCalibrated(), rawAMag, rawWMag, rawAz, gErrMaxDeg, gSignScore,
                   gDir.x, gDir.y, gDir.z, flowDir.x, flowDir.y, flowDir.z, dotCount);
  gErrMaxDeg = 0;
  gSignScore = 0;
  frames = 0;
  timeSum = 0;
  simSum = projSum = rendSum = 0;
#endif
}

// plna inicializace a prekresleni (displej uz je cerny); false jen pri
// fatalni chybe SPI zarizeni
bool starBegin() {
  if (!inputBeginIMU()) USBSerial.println("QMI8658 init fail - hvezdy poleti primo na divaka");
  else USBSerial.println("QMI8658 OK");
  // prvni vzorek znovu nastavi gDir a flowDir, jinak by hvezdy chvili
  // letely starym smerem; bias gyra a calibrated se zamerne nechavaji
  haveGravity = false;

  if (!renderInit()) {
    USBSerial.println("SPI device init fail");
    return false;
  }
  fieldInit();

  lastUs = micros();
  return true;
}

void starLoop() {
  const uint32_t now = micros();
  float dt = (now - lastUs) * 1e-6f;
  lastUs = now;
  if (dt > 0.1f) dt = 0.1f;

  inputRead(dt);
  fieldUpdate(dt);
  const uint32_t t1 = micros();

  Vec3 U, V, W;
  inputBasis(U, V, W);
  projectStars(U, V, W);
  const uint32_t t2 = micros();

  renderFrame();
  const uint32_t t3 = micros();

  debugPrint(dt, t1 - now, t2 - t1, t3 - t2);
}

void starEnd() {
  // dokonci posledni pruh - fronta prazdna, CS HIGH, od ted smi kreslit
  // launcher pres gfx
  waitPending(0);
}
