// esp32-rat-jumper - detska skakacka: krysa bezi kanalem, skace pres
// prekazky a sbira odpadky; displej na sirku, barevny CRT filtr
// Waveshare ESP32-S3-Touch-AMOLED-1.8, specifikace viz rat-jumper.md

#include "../common/amoled_hw.h"
#include "../common/amoled_touch.h"
#include "rat_app.h"

void setup() {
  hwInit();
  if (!touchBegin()) USBSerial.println("FT3168 init fail");
  else USBSerial.println("FT3168 OK");
  if (!ratBegin()) hwHalt("rat-jumper begin fail");
}

void loop() {
  touchRead();
  ratLoop();
}
