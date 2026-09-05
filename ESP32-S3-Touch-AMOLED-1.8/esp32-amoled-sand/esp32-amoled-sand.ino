// esp32-amoled-sand - interaktivni falling-sand simulace rizena IMU
// Waveshare ESP32-S3-Touch-AMOLED-1.8, specifikace viz sand.md

#include "../common/amoled_hw.h"
#include "../common/amoled_touch.h"
#include "sand_app.h"

void setup() {
  hwInit();
  if (!touchBegin()) USBSerial.println("FT3168 init fail");
  else USBSerial.println("FT3168 OK");
  if (!sandBegin()) hwHalt("sand begin fail");
}

void loop() {
  touchRead();
  sandLoop();
}
