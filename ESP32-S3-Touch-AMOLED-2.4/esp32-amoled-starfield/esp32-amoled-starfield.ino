// esp32-amoled-starfield - prulet hvezdnym polem rizeny IMU
// Waveshare ESP32-S3-Touch-AMOLED-1.8, specifikace viz starfield.md

#include "../common/amoled_hw.h"
#include "star_app.h"

void setup() {
  hwInit();
  if (!starBegin()) hwHalt("starfield begin fail");
}

void loop() {
  starLoop();
}
