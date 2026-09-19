// esp32-amoled-sdlauncher - seznam aplikaci z SD karty.
// Binarky lezi na karte v /apps/*.bin. Tuknuti na polozku ji zkopiruje
// do druheho app oddilu (ota_1) a restartuje se do ni; aplikace si pak
// v hwInit() prepne bootovaci oddil zpet sem, takze jakykoli dalsi
// restart vrati tento seznam.
// Waveshare ESP32-S3-Touch-AMOLED-2.41

#include <SPI.h>
#include <SD.h>
#include <esp_ota_ops.h>

#include "../common/amoled_hw.h"
#include "../common/amoled_touch.h"

#define MAX_APPS   12
#define ROW_H      56
#define LIST_Y     110
#define MARGIN     24
#define COPY_CHUNK 4096

static SPIClass sdSpi(HSPI);
static bool sdOk = false;

static char names[MAX_APPS][32];      // nazev bez pripony (na displeji)
static char paths[MAX_APPS][64];      // cela cesta na karte
static uint32_t sizes[MAX_APPS];
static int appCount = 0;

static void msg(const char *text, uint16_t color) {
  gfx->fillRect(0, LCD_HEIGHT - 60, LCD_WIDTH, 60, RGB565_BLACK);
  gfx->setTextSize(2);
  gfx->setTextColor(color);
  gfx->setCursor(MARGIN, LCD_HEIGHT - 44);
  gfx->print(text);
}

// nacte /apps/*.bin do tabulky (razeni neresime, staci poradi z FAT)
static void scanApps() {
  appCount = 0;
  File dir = SD.open("/apps");
  if (!dir || !dir.isDirectory()) return;
  for (File f = dir.openNextFile(); f && appCount < MAX_APPS; f = dir.openNextFile()) {
    const char *n = strrchr(f.name(), '/');
    n = n ? n + 1 : f.name();
    const size_t len = strlen(n);
    if (f.isDirectory() || len < 5 || strcasecmp(n + len - 4, ".bin") != 0) { f.close(); continue; }
    snprintf(paths[appCount], sizeof(paths[0]), "/apps/%s", n);
    snprintf(names[appCount], sizeof(names[0]), "%.*s", (int)(len - 4), n);
    sizes[appCount] = f.size();
    appCount++;
    f.close();
  }
  dir.close();
}

static void drawList(int highlight) {
  gfx->fillScreen(RGB565_BLACK);
  gfx->setTextSize(3);
  gfx->setTextColor(RGB565_WHITE);
  gfx->setCursor(MARGIN, 40);
  gfx->print("APLIKACE");
  gfx->drawFastHLine(MARGIN, 78, LCD_WIDTH - 2 * MARGIN, RGB565_DARKGREY);

  if (!sdOk)        { msg("SD karta nenalezena", RGB565_RED); return; }
  if (!appCount)    { msg("na karte neni /apps/*.bin", RGB565_RED); return; }

  for (int i = 0; i < appCount; i++) {
    const int y = LIST_Y + i * ROW_H;
    const uint16_t bg = (i == highlight) ? RGB565_BLUE : 0x2104;
    gfx->fillRoundRect(MARGIN, y, LCD_WIDTH - 2 * MARGIN, ROW_H - 10, 6, bg);
    gfx->setTextSize(2);
    gfx->setTextColor(RGB565_WHITE);
    gfx->setCursor(MARGIN + 14, y + 10);
    gfx->print(names[i]);
    gfx->setTextSize(1);
    gfx->setTextColor(RGB565_LIGHTGREY);
    gfx->setCursor(MARGIN + 14, y + 30);
    gfx->printf("%u kB", (unsigned)(sizes[i] / 1024));
  }
  msg("tukni na aplikaci", RGB565_DARKGREY);
}

static void drawProgress(int pct) {
  const int y = LCD_HEIGHT - 120, w = LCD_WIDTH - 2 * MARGIN;
  gfx->drawRect(MARGIN, y, w, 24, RGB565_WHITE);
  gfx->fillRect(MARGIN + 2, y + 2, (w - 4) * pct / 100, 20, RGB565_GREEN);
}

// zkopiruje binarku z karty do oddilu ota_1 a nastavi z nej boot
static bool flashApp(int idx) {
  const esp_partition_t *target = esp_ota_get_next_update_partition(nullptr);
  if (!target) { msg("volny app oddil neni", RGB565_RED); return false; }

  File f = SD.open(paths[idx], FILE_READ);
  if (!f) { msg("soubor nejde otevrit", RGB565_RED); return false; }
  const uint32_t total = f.size();
  if (total == 0 || total > target->size) { f.close(); msg("binarka se do oddilu nevejde", RGB565_RED); return false; }

  esp_ota_handle_t h = 0;
  if (esp_ota_begin(target, total, &h) != ESP_OK) { f.close(); msg("esp_ota_begin selhal", RGB565_RED); return false; }

  static uint8_t buf[COPY_CHUNK];
  uint32_t done = 0;
  int lastPct = -1;
  while (done < total) {
    const int n = f.read(buf, sizeof(buf));
    if (n <= 0) break;
    if (esp_ota_write(h, buf, n) != ESP_OK) { esp_ota_abort(h); f.close(); msg("zapis do oddilu selhal", RGB565_RED); return false; }
    done += n;
    const int pct = (int)(100ULL * done / total);
    if (pct != lastPct) { drawProgress(pct); lastPct = pct; }
  }
  f.close();

  if (done != total)             { esp_ota_abort(h); msg("cteni z karty selhalo", RGB565_RED); return false; }
  if (esp_ota_end(h) != ESP_OK)  { msg("binarka je poskozena", RGB565_RED); return false; }
  if (esp_ota_set_boot_partition(target) != ESP_OK) { msg("nastaveni bootu selhalo", RGB565_RED); return false; }
  return true;
}

static void runApp(int idx) {
  gfx->fillScreen(RGB565_BLACK);
  gfx->setTextSize(3);
  gfx->setTextColor(RGB565_WHITE);
  gfx->setCursor(MARGIN, LCD_HEIGHT / 2 - 40);
  gfx->print(names[idx]);
  msg("kopiruji z karty...", RGB565_LIGHTGREY);
  USBSerial.printf("spoustim %s\n", paths[idx]);

  if (flashApp(idx)) {
    msg("startuji", RGB565_GREEN);
    delay(300);
    esp_restart();
  }
  delay(2500);
  drawList(-1);
}

void setup() {
  hwInit();
  if (!touchBegin()) USBSerial.println("FT6336 init fail");

  sdSpi.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
  sdOk = SD.begin(SD_CS, sdSpi);
  USBSerial.println(sdOk ? "SD karta OK" : "SD karta neni");
  if (sdOk) scanApps();
  USBSerial.printf("aplikaci na karte: %d\n", appCount);
  drawList(-1);
}

void loop() {
  static bool prev = false;
  touchRead();
  const bool tap = touchDown && !prev;
  prev = touchDown;
  if (!tap || !appCount) { delay(10); return; }

  USBSerial.printf("dotyk %d %d\n", touchX, touchY);
  const int idx = (touchY - LIST_Y) / ROW_H;
  if (idx < 0 || idx >= appCount || touchY < LIST_Y) return;
  drawList(idx);
  delay(120);
  runApp(idx);
}
