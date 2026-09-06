#pragma once

#include <Arduino.h>
#include <SD_MMC.h>
#include "../common/amoled_app.h"   // USBSerial, pin_config.h (SDMMC_CLK/CMD/DATA)

// ===================================================================
// Nejlepsi skore na SD karte: /rat-jumper/best.txt (jedno cislo).
// Karta je na SDMMC v 1bitovem rezimu (piny z pin_config.h). Bez karty
// hra bezi dal, skore se jen nepamatuje pres vypnuti.
// ===================================================================

#define HISCORE_DIR  "/rat-jumper"
#define HISCORE_FILE HISCORE_DIR "/best.txt"

static bool sdOk = false;

static bool hiscoreBegin() {
  SD_MMC.setPins(SDMMC_CLK, SDMMC_CMD, SDMMC_DATA);
  sdOk = SD_MMC.begin("/sd", true /* 1 bit */);
  return sdOk;
}

static int hiscoreLoad() {
  if (!sdOk) return 0;
  File f = SD_MMC.open(HISCORE_FILE, FILE_READ);
  if (!f) return 0;
  const int v = f.parseInt();
  f.close();
  return v < 0 ? 0 : v;
}

static void hiscoreSave(int v) {
  if (!sdOk) return;
  SD_MMC.mkdir(HISCORE_DIR);
  File f = SD_MMC.open(HISCORE_FILE, FILE_WRITE);
  if (!f) { USBSerial.println("SD: zapis best.txt selhal"); return; }
  f.println(v);
  f.close();
}

static void hiscoreEnd() {
  if (sdOk) SD_MMC.end();
  sdOk = false;
}
