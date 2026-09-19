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
#include "FreeSansBold18pt7b.h"
#include "FreeSans12pt7b.h"

#define MAX_APPS   8
#define ROW_H      92        // rozestup polozek (velke pismo, palec)
#define MARGIN     30
#define COPY_CHUNK 4096

#define COL_TEXT   RGB565_WHITE
#define COL_PICK   RGB565_ORANGE    // tuknuta polozka
#define COL_ERR    RGB565_RED

static SPIClass sdSpi(HSPI);
static bool sdOk = false;

static char names[MAX_APPS][24];      // nazev bez pripony (na displeji)
static char paths[MAX_APPS][64];      // cela cesta na karte
static int appCount = 0;
static int listY = 0;                 // horni hrana prvni polozky

// nacte /apps/*.bin do tabulky (razeni neresime, staci poradi z FAT)
static void scanApps() {
  appCount = 0;
  File dir = SD.open("/apps");
  if (!dir || !dir.isDirectory()) return;
  for (File f = dir.openNextFile(); f && appCount < MAX_APPS; f = dir.openNextFile()) {
    const char *n = strrchr(f.name(), '/');
    n = n ? n + 1 : f.name();
    const size_t len = strlen(n);
    // macOS vedle souboru zaklada stinove kopie "._jmeno.bin"
    if (f.isDirectory() || len < 5 || strncmp(n, "._", 2) == 0 ||
        strcasecmp(n + len - 4, ".bin") != 0) { f.close(); continue; }
    snprintf(paths[appCount], sizeof(paths[0]), "/apps/%s", n);
    snprintf(names[appCount], sizeof(names[0]), "%.*s", (int)(len - 4), n);
    for (char *c = names[appCount]; *c; c++) if (*c == '-') *c = ' ';   // hezci nazev
    appCount++;
    f.close();
  }
  dir.close();
  listY = (LCD_HEIGHT - appCount * ROW_H) / 2 + 10;
  if (listY < 20) listY = 20;
}

// Panel RM690B0 zahazuje zapisy zacinajici na lichem x, takze kresleni
// pismen primo na displej (jeden bod = okno sirky 1) nic nenakresli.
// Vsechen text proto vznika v pameti a na displej jde jednim blokem
// pres celou sirku - ten zacina na nule a je sirky 450, tedy v poradku.
static Arduino_Canvas *rowBuf = nullptr;

static void blitRow(int y) {
  gfx->draw16bitRGBBitmap(0, y, rowBuf->getFramebuffer(), LCD_WIDTH, ROW_H);
}

// jedna polozka; text sedi na uctne
static void drawItem(int i, uint16_t color) {
  if (!rowBuf) return;
  rowBuf->fillScreen(RGB565_BLACK);
  rowBuf->setFont(&FreeSansBold18pt7b);
  rowBuf->setTextColor(color);
  rowBuf->setCursor(MARGIN, 44);
  rowBuf->print(names[i]);
  blitRow(listY + i * ROW_H);
}

static void note(const char *text, uint16_t color) {
  if (!rowBuf) return;
  rowBuf->fillScreen(RGB565_BLACK);
  rowBuf->setFont(&FreeSans12pt7b);
  rowBuf->setTextColor(color);
  rowBuf->setCursor(MARGIN, 30);
  rowBuf->print(text);
  blitRow(LCD_HEIGHT / 2 - ROW_H / 2);
}

static void drawList() {
  gfx->fillScreen(RGB565_BLACK);
  if (!sdOk)     { note("bez SD karty", COL_ERR); return; }
  if (!appCount) { note("na karte neni /apps/*.bin", COL_ERR); return; }
  for (int i = 0; i < appCount; i++) drawItem(i, COL_TEXT);
}

static void drawProgress(int pct) {
  const int y = LCD_HEIGHT - 70, w = LCD_WIDTH - 2 * MARGIN;
  gfx->fillRect(MARGIN, y, w * pct / 100, 6, COL_PICK);
}

// zkopiruje binarku z karty do oddilu ota_1 a nastavi z nej boot
static bool flashApp(int idx) {
  const esp_partition_t *target = esp_ota_get_next_update_partition(nullptr);
  if (!target) { note("volny app oddil neni", COL_ERR); return false; }

  File f = SD.open(paths[idx], FILE_READ);
  if (!f) { note("soubor nejde otevrit", COL_ERR); return false; }
  const uint32_t total = f.size();
  if (total == 0 || total > target->size) { f.close(); note("binarka se nevejde", COL_ERR); return false; }

  esp_ota_handle_t h = 0;
  if (esp_ota_begin(target, total, &h) != ESP_OK) { f.close(); note("esp_ota_begin selhal", COL_ERR); return false; }

  static uint8_t buf[COPY_CHUNK];
  uint32_t done = 0;
  int lastPct = -1;
  while (done < total) {
    const int n = f.read(buf, sizeof(buf));
    if (n <= 0) break;
    if (esp_ota_write(h, buf, n) != ESP_OK) { esp_ota_abort(h); f.close(); note("zapis selhal", COL_ERR); return false; }
    done += n;
    const int pct = (int)(100ULL * done / total);
    if (pct != lastPct) { drawProgress(pct); lastPct = pct; }
  }
  f.close();

  if (done != total)            { esp_ota_abort(h); note("cteni z karty selhalo", COL_ERR); return false; }
  if (esp_ota_end(h) != ESP_OK) { note("binarka je poskozena", COL_ERR); return false; }
  if (esp_ota_set_boot_partition(target) != ESP_OK) { note("nastaveni bootu selhalo", COL_ERR); return false; }
  return true;
}

// tuknuta polozka zustane obarvena, ostatni zhasnou; pak se kopiruje
static void runApp(int idx) {
  gfx->fillScreen(RGB565_BLACK);
  drawItem(idx, COL_PICK);
  USBSerial.printf("spoustim %s\n", paths[idx]);

  if (flashApp(idx)) {
    delay(200);
    esp_restart();
  }
  delay(2500);
  drawList();
}

void setup() {
  hwInit();
  rowBuf = new Arduino_Canvas(LCD_WIDTH, ROW_H, gfx, 0, 0);
  if (!rowBuf->begin(GFX_SKIP_OUTPUT_BEGIN)) USBSerial.println("buffer radku se nepodarilo alokovat");

  hwStep = "dotyk";
  if (!touchBegin()) USBSerial.println("FT6336 init fail");

  hwStep = "SD karta";
  sdSpi.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
  sdOk = SD.begin(SD_CS, sdSpi);
  USBSerial.println(sdOk ? "SD karta OK" : "SD karta neni");
  hwStep = "cteni /apps";
  if (sdOk) scanApps();
  USBSerial.printf("aplikaci na karte: %d\n", appCount);
  hwStep = "kresleni seznamu";
  drawList();
  hwStep = "launcher";
}

// karta zasunuta az po startu: zkousime ji otevrit dokola, seznam
// se sam objevi, jakmile je co ukazat
static void pollCard() {
  static uint32_t lastTry = 0;
  if (sdOk || millis() - lastTry < 1500) return;
  lastTry = millis();
  if (!SD.begin(SD_CS, sdSpi)) return;
  sdOk = true;
  scanApps();
  USBSerial.printf("karta zasunuta, aplikaci: %d\n", appCount);
  drawList();
}

// pravidelny vypis: podle nej se pozna, ze deska bezi, i kdyz je displej tmavy
static void heartbeat() {
  static uint32_t last = 0;
  if (millis() - last < 3000) return;
  last = millis();
  USBSerial.printf("launcher bezi, aplikaci %d, heap %u, panel %s\n",
                   appCount, (unsigned)ESP.getFreeHeap(), exioOk ? "zapnuty" : "NEZAPNUTY");
}

void loop() {
  static bool prev = false;
  heartbeat();
  pollCard();
  touchRead();
  const bool tap = touchDown && !prev;
  prev = touchDown;
  if (!tap) { delay(10); return; }

  USBSerial.printf("dotyk %d %d\n", touchX, touchY);
  if (!appCount) return;
  const int idx = (touchY - listY) / ROW_H;
  if (touchY < listY || idx < 0 || idx >= appCount) return;
  drawItem(idx, COL_PICK);     // odezva na tuknuti
  delay(150);
  runApp(idx);
}
