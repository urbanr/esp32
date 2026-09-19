#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include "../common/amoled_app.h"     // USBSerial
#include "../common/pin_config.h"     // SD_CS/SCLK/MOSI/MISO

// ===================================================================
// Nejlepsi skore na SD karte: /rat-jumper/best.txt (jedno cislo).
// Karta je na vlastni SPI sbernici (piny z pin_config.h), aby si
// nelezla do cesty s QSPI displeje na SPI2. Bez karty hra bezi dal,
// skore se jen nepamatuje pres vypnuti.
// ===================================================================

#define HISCORE_DIR  "/rat-jumper"
#define HISCORE_FILE HISCORE_DIR "/best.txt"

static bool sdOk = false;
static SPIClass sdSpi(HSPI);

static bool hiscoreBegin() {
  sdSpi.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
  sdOk = SD.begin(SD_CS, sdSpi);
  return sdOk;
}

static int hiscoreLoad() {
  if (!sdOk) return 0;
  File f = SD.open(HISCORE_FILE, FILE_READ);
  if (!f) return 0;
  const int v = f.parseInt();
  f.close();
  return v < 0 ? 0 : v;
}

static void hiscoreSave(int v) {
  if (!sdOk) return;
  SD.mkdir(HISCORE_DIR);
  File f = SD.open(HISCORE_FILE, FILE_WRITE);
  if (!f) { USBSerial.println("SD: zapis best.txt selhal"); return; }
  f.println(v);
  f.close();
}

static void hiscoreEnd() {
  if (sdOk) SD.end();
  sdOk = false;
}
