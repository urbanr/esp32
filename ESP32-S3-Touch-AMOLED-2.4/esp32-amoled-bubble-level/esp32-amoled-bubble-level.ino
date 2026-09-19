// esp32-amoled-bubble-level - kruhova vodovaha rizena IMU
// Waveshare ESP32-S3-Touch-AMOLED-1.8, specifikace viz bubble-level.md

#include "../common/amoled_hw.h"
#include "level_app.h"

void setup() {
  hwInit();
  if (!levelBegin()) hwHalt("bubble-level begin fail");
}

void loop() {
  levelLoop();
}
