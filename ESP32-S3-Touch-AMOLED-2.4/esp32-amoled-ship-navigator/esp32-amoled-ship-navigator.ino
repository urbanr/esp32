// esp32-amoled-ship-navigator - detska palubni deska rakety (3 obrazovky,
// swipe, styl zeleneho monitoru 80. let)
// Waveshare ESP32-S3-Touch-AMOLED-1.8, specifikace viz ship-navigator.md

#include "../common/amoled_hw.h"
#include "../common/amoled_touch.h"
#include "ship_app.h"

void setup() {
  hwInit();
  if (!touchBegin()) USBSerial.println("FT3168 init fail");
  else USBSerial.println("FT3168 OK");
  if (!shipBegin()) hwHalt("ship-navigator begin fail");
}

void loop() {
  touchRead();
  shipLoop();
}
