#pragma once

#include <Wire.h>
#include "Arduino_DriveBus_Library.h"
#include "pin_config.h"
#include "amoled_app.h"

// ===================================================================
// Dotyk FT3168 pres Arduino_DriveBus, poll kazdou otocku smycky
// (touchRead()). Obsahuje definice - includovat POUZE z hlavniho .ino
// (launcher, samostatny pisek); moduly aplikaci ctou touchDown/touchX/
// touchY pres amoled_app.h. Wire musi byt otevreny (hwInit()).
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

bool touchDown = false;
int  touchX = 0, touchY = 0;

static bool touchBegin() {
  for (int t = 0; t < 5; t++) {
    if (FT3168->begin()) return true;
    delay(200);
  }
  return false;
}

// precte pocet prstu a pri dotyku pozici prvniho prstu. Ovladac vraci
// -1 pri chybe I2C - takovy vzorek se ignoruje a plati predchozi stav
// (jinak by jedina chyba uprostred tuknuti vypadala jako zvednuti
// prstu a z jednoho tuknuti by byl dvojklik)
static void touchRead() {
  const int32_t fingers = FT3168->IIC_Read_Device_Value(
    FT3168->Arduino_IIC_Touch::Value_Information::TOUCH_FINGER_NUMBER);
  if (fingers > 0) {
    const int32_t tx = FT3168->IIC_Read_Device_Value(
      FT3168->Arduino_IIC_Touch::Value_Information::TOUCH_COORDINATE_X);
    const int32_t ty = FT3168->IIC_Read_Device_Value(
      FT3168->Arduino_IIC_Touch::Value_Information::TOUCH_COORDINATE_Y);
    if (tx >= 0 && ty >= 0) {   // pri chybe cteni zustava posledni pozice
      touchX = (int)tx;
      touchY = (int)ty;
    }
    touchDown = true;
  } else if (fingers == 0) {
    touchDown = false;
  }
  FT3168->IIC_Interrupt_Flag = false;
}
