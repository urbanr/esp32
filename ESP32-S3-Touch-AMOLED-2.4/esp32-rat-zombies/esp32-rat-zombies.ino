// esp32-rat-zombies - Krysy a zombici: krysa na kolech jede po zvlnenem
// terenu, sbira mince, resi priklady za benzin a projizdi zombiky;
// displej na sirku, barevny CRT filtr. Specifikace viz rat-zombies.md.

#include "../common/amoled_hw.h"
#include "../common/amoled_touch.h"
#include "zomb_app.h"

void setup() {
  hwInit();
  if (!touchBegin()) USBSerial.println("FT3168 init fail");
  else USBSerial.println("FT3168 OK");
  if (!zombBegin()) hwHalt("rat-zombies begin fail");
}

void loop() {
  touchRead();
  zombLoop();
}
