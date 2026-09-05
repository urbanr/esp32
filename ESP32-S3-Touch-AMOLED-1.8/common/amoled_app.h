#pragma once

#include "Arduino_GFX_Library.h"
#include "HWCDC.h"
#include "pin_config.h"

// ===================================================================
// Rozhrani mezi sketchem (launcher nebo samostatna aplikace) a modulem
// aplikace (<app>_app.h). Sketch vlastni hardware a dotyk, modul je
// jen pouziva. Definice objektu jsou v amoled_hw.h a amoled_touch.h -
// ty includuje POUZE hlavni .ino; modul aplikace includuje jen tento
// soubor.
// ===================================================================

#define AMOLED_BRIGHTNESS        200                   // jas displeje 0-255, spolecny pro vsechny aplikace
#define AMOLED_SPI_MAX_TRANSFER  (LCD_WIDTH * 32 * 2)  // nejvetsi SPI transakce (pruh hvezd 368x32 RGB565)

extern HWCDC USBSerial;
extern Arduino_SH8601 *gfx;

// dotyk FT3168 - poll kazdou otocku smycky (touchRead() v amoled_touch.h)
extern bool touchDown;        // prvni prst drzen na displeji
extern int  touchX, touchY;   // jeho pozice (display px)
