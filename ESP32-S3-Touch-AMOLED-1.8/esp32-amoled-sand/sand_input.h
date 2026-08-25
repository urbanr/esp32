#pragma once

#include <Wire.h>
#include "Arduino_DriveBus_Library.h"
#include "SensorQMI8658.hpp"
#include "pin_config.h"
#include "config.h"

// ===================================================================
// Vstupy: QMI8658 (vektor gravitace v rovine displeje + trepani)
// a FT3168 (touch, poll kazdy tick).
// ===================================================================

static std::shared_ptr<Arduino_IIC_DriveBus> IIC_Bus =
  std::make_shared<Arduino_HWIIC>(IIC_SDA, IIC_SCL, &Wire);

static void Arduino_IIC_Touch_Interrupt(void);

static std::unique_ptr<Arduino_IIC> FT3168(new Arduino_FT3x68(
  IIC_Bus, FT3168_DEVICE_ADDRESS, DRIVEBUS_DEFAULT_VALUE, TP_INT,
  Arduino_IIC_Touch_Interrupt));

static void Arduino_IIC_Touch_Interrupt(void) {
  FT3168->IIC_Interrupt_Flag = true;
}

static SensorQMI8658 qmi;
static bool imuOk = false;

// vystupy pro simulaci
static float inGX = 0, inGY = 0;   // slozky gravitace v rovine displeje (g)
static float inShake = 0;          // 0.. odchylka |a| od 1 g (EMA)
static bool  inTouchDown = false;
static int   inTouchX = 0, inTouchY = 0;

static bool inputBeginTouch() {
  for (int t = 0; t < 5; t++) {
    if (FT3168->begin()) return true;
    delay(200);
  }
  return false;
}

static bool inputBeginIMU() {
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

  // --- touch (poll) ---
  const int32_t fingers = FT3168->IIC_Read_Device_Value(
    FT3168->Arduino_IIC_Touch::Value_Information::TOUCH_FINGER_NUMBER);
  if (fingers > 0) {
    const int32_t tx = FT3168->IIC_Read_Device_Value(
      FT3168->Arduino_IIC_Touch::Value_Information::TOUCH_COORDINATE_X);
    const int32_t ty = FT3168->IIC_Read_Device_Value(
      FT3168->Arduino_IIC_Touch::Value_Information::TOUCH_COORDINATE_Y);
    inTouchDown = true;
    inTouchX = (int)tx;
    inTouchY = (int)ty;
  } else {
    inTouchDown = false;
  }
  FT3168->IIC_Interrupt_Flag = false;
}
