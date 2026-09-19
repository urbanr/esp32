#pragma once

#include <Wire.h>
#include "SensorQMI8658.hpp"
#include "pin_config.h"
#include "config.h"

// ===================================================================
// Vstupy: QMI8658 (vektor gravitace v rovine displeje + trepani).
// Dotyk FT3168 cte sketch pres ../common/amoled_touch.h, modul ho
// dostava jako touchDown/touchX/touchY z amoled_app.h.
// ===================================================================

static SensorQMI8658 qmi;
static bool imuOk = false;

// vystupy pro simulaci
static float inGX = 0, inGY = 0;   // slozky gravitace v rovine displeje (g)
static float inShake = 0;          // 0.. odchylka |a| od 1 g (EMA)

static bool inputBeginIMU() {
  imuOk = false;
  if (!qmi.begin(Wire, QMI8658_L_SLAVE_ADDRESS, IIC_SDA, IIC_SCL)) return false;
  qmi.configAccelerometer(SensorQMI8658::ACC_RANGE_4G,
                          SensorQMI8658::ACC_ODR_250Hz);
  qmi.enableAccelerometer();
  imuOk = true;
  return true;
}

static void inputRead() {
  // --- IMU ---
  if (imuOk && qmi.getDataReady()) {
    float ax = 0, ay = 0, az = 0;
    if (qmi.getAccelerometer(ax, ay, az)) {
      const float px = ACCEL_MAP_GX(ax, ay, az);
      const float py = ACCEL_MAP_GY(ax, ay, az);
      // lehke vyhlazeni proti jitteru
      inGX += 0.25f * (px - inGX);
      inGY += 0.25f * (py - inGY);
      // trepani = EMA odchylky celkove akcelerace od 1 g
      const float dev = fabsf(sqrtf(ax * ax + ay * ay + az * az) - 1.0f);
      inShake += 0.05f * (dev - inShake);
    }
  }
}
