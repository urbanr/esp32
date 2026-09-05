#pragma once

#include <Wire.h>
#include "SensorQMI8658.hpp"
#include "pin_config.h"
#include "config.h"

// ===================================================================
// Vstup: QMI8658 - vektor gravitace v rovine displeje (g).
// Aretace: inputZero() vezme aktualni naklon jako novou rovinu.
// ===================================================================

static SensorQMI8658 qmi;
static bool imuOk = false;

// vyhlazene slozky gravitace v rovine displeje (g): rawGX/rawGY merene,
// inGX/inGY vztazene k aretovane rovine (to, co vidi bublina a text)
static float rawGX = 0, rawGY = 0;
static bool  rawPrimed = false;      // prvni vzorek po startu nastavi filtr primo
static float zeroAX = 0, zeroAY = 0; // aretovana rovina jako uhly os (rad)
static float inGX = 0, inGY = 0;

static bool inputBeginIMU() {
  imuOk = false;
  rawPrimed = false;
  if (!qmi.begin(Wire, QMI8658_L_SLAVE_ADDRESS, IIC_SDA, IIC_SCL)) return false;
  qmi.configAccelerometer(SensorQMI8658::ACC_RANGE_4G,
                          SensorQMI8658::ACC_ODR_250Hz);
  qmi.enableAccelerometer();
  imuOk = true;
  return true;
}

// uhel naklonu kolem jedne osy z odpovidajici slozky gravitace (rad)
static inline float axisAngle(float g) {
  if (g < -1) g = -1; else if (g > 1) g = 1;
  return asinf(g);
}

static void inputRead() {
  if (!imuOk || !qmi.getDataReady()) return;
  float ax = 0, ay = 0, az = 0;
  if (!qmi.getAccelerometer(ax, ay, az)) return;
  const float px = ACCEL_MAP_GX(ax, ay, az);
  const float py = ACCEL_MAP_GY(ax, ay, az);
  if (!rawPrimed) {
    rawGX = px;
    rawGY = py;
    rawPrimed = true;
  } else {
    // lehke vyhlazeni proti jitteru
    rawGX += 0.25f * (px - rawGX);
    rawGY += 0.25f * (py - rawGY);
  }
  // aretace v uhlech os: presne pro naklon kolem jedne osy a symetricke
  // i pri vetsim naklonu aretovane roviny (proste odecteni slozek g neni)
  inGX = sinf(axisAngle(rawGX) - zeroAX);
  inGY = sinf(axisAngle(rawGY) - zeroAY);
}

// aretace: aktualni naklon se stane novou rovinou (bublina do stredu,
// udaj naklonu 0)
static void inputZero() {
  zeroAX = axisAngle(rawGX);
  zeroAY = axisAngle(rawGY);
  inGX = 0;
  inGY = 0;
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
