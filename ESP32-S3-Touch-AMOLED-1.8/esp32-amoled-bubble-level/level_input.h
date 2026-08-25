#pragma once

#include <Wire.h>
#include "SensorQMI8658.hpp"
#include "pin_config.h"
#include "config.h"

// ===================================================================
// Vstup: QMI8658 - vektor gravitace v rovine displeje (g).
// ===================================================================

static SensorQMI8658 qmi;
static bool imuOk = false;

// vyhlazene slozky gravitace v rovine displeje (g)
static float inGX = 0, inGY = 0;

static bool inputBeginIMU() {
  if (!qmi.begin(Wire, QMI8658_L_SLAVE_ADDRESS, IIC_SDA, IIC_SCL)) return false;
  qmi.configAccelerometer(SensorQMI8658::ACC_RANGE_4G,
                          SensorQMI8658::ACC_ODR_250Hz);
  qmi.enableAccelerometer();
  imuOk = true;
  return true;
}

static void inputRead() {
  if (!imuOk || !qmi.getDataReady()) return;
  float ax = 0, ay = 0, az = 0;
  if (!qmi.getAccelerometer(ax, ay, az)) return;
  const float px = ACCEL_MAP_GX(ax, ay, az);
  const float py = ACCEL_MAP_GY(ax, ay, az);
  // lehke vyhlazeni proti jitteru
  inGX += 0.25f * (px - inGX);
  inGY += 0.25f * (py - inGY);
}

// naklony pro cislny udaj (stupne)
static float tiltXDeg() {
  float v = inGX < -1 ? -1 : (inGX > 1 ? 1 : inGX);
  return TILT_X_SIGN * asinf(v) * RAD_TO_DEG;
}
static float tiltYDeg() {
  float v = inGY < -1 ? -1 : (inGY > 1 ? 1 : inGY);
  return TILT_Y_SIGN * asinf(v) * RAD_TO_DEG;
}
