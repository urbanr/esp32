#pragma once

#include <Arduino.h>
#include <SD_MMC.h>
#include "../common/amoled_app.h"   // USBSerial, pin_config.h (SDMMC_CLK/CMD/DATA)

// ===================================================================
// Nastaveni na SD karte: /rat-zombies/settings.txt (parametry CRT).
// Postup hry se zamerne neuklada - hra jede vzdy od zacatku.
// SDMMC 1 bit; bez karty se nastaveni jen nepamatuje.
// ===================================================================

#define SAVE_DIR  "/rat-zombies"
#define SETTINGS_FILE SAVE_DIR "/settings.txt"

static bool sdOk = false;

static bool saveBegin() {
  SD_MMC.setPins(SDMMC_CLK, SDMMC_CMD, SDMMC_DATA);
  sdOk = SD_MMC.begin("/sd", true);
  return sdOk;
}

// nastaveni CRT: verze 5, rezim, radky horni/dolni, prosvit, rozmazani, maska, blikani, pruh, jiskry, zesileni*100, gama*100
static bool settingsLoad(int *v, int n, float &gain, float &gamma) {
  if (!sdOk) return false;
  File f = SD_MMC.open(SETTINGS_FILE, FILE_READ);
  if (!f) return false;
  const bool ok = f.parseInt() == 5;
  if (ok) {
    for (int i = 0; i < n; i++) v[i] = f.parseInt();
    gain = f.parseInt() / 100.0f; gamma = f.parseInt() / 100.0f;
  }
  f.close();
  if (!ok) return false;
  for (int i = 0; i < n; i++) if (v[i] < 0 || v[i] > 256) return false;
  return gain > 0.5f && gain < 3.0f && gamma > 0.4f && gamma < 2.5f;
}

static void settingsStore(const int *v, int n, float gain, float gamma) {
  if (!sdOk) return;
  SD_MMC.mkdir(SAVE_DIR);
  File f = SD_MMC.open(SETTINGS_FILE, FILE_WRITE);
  if (!f) return;
  f.print("5");
  for (int i = 0; i < n; i++) f.printf(" %d", v[i]);
  f.printf(" %d %d\n", (int)(gain * 100), (int)(gamma * 100));
  f.close();
}

static void saveEnd() {
  if (sdOk) SD_MMC.end();
  sdOk = false;
}
