// esp32-amoled-sdusb - SD karta v zarizeni jako USB disk.
// Sluzebni sketch pro plneni karty, kdyz neni po ruce ctecka: po nahrani
// se karta objevi v pocitaci jako vymenny disk, nakopiruji se soubory do
// /apps a pak se do desky vrati launcher.
//
// Prekladat s USBMode=default (USB-OTG TinyUSB) - jen tak je MSC k mani.
// Sdileny kod z ../common se schvalne nepouziva: v OTG rezimu si jmeno
// USBSerial zabira jadro (USBCDC) a kolidovalo by s HWCDC v amoled_hw.h.
// Waveshare ESP32-S3-Touch-AMOLED-2.41

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include "Arduino_GFX_Library.h"
#include "USB.h"
#include "USBMSC.h"

#include "../common/pin_config.h"

static Arduino_DataBus *bus = new Arduino_ESP32QSPI(
  LCD_CS, LCD_SCLK, LCD_SDIO0, LCD_SDIO1, LCD_SDIO2, LCD_SDIO3);
static Arduino_RM690B0 *gfx = new Arduino_RM690B0(
  bus, LCD_RST, 0, LCD_WIDTH, LCD_HEIGHT, LCD_X_OFF, LCD_Y_OFF, LCD_X_OFF, LCD_Y_OFF);

static SPIClass sdSpi(HSPI);
static USBMSC msc;

static void exioWrite(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(EXIO_ADDR);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

static void screen(const char *line1, const char *line2, uint16_t color) {
  gfx->fillScreen(RGB565_BLACK);
  gfx->setTextSize(3);
  gfx->setTextColor(color);
  gfx->setCursor(24, 60);
  gfx->print(line1);
  gfx->setTextSize(2);
  gfx->setTextColor(RGB565_LIGHTGREY);
  gfx->setCursor(24, 120);
  gfx->print(line2);
}

// cteme a zapisujeme jen cele sektory karty
static int32_t onRead(uint32_t lba, uint32_t offset, void *buffer, uint32_t bufsize) {
  const uint32_t sec = SD.sectorSize();
  if (offset || (bufsize % sec)) return -1;
  for (uint32_t i = 0; i < bufsize / sec; i++)
    if (!SD.readRAW((uint8_t *)buffer + i * sec, lba + i)) return -1;
  return bufsize;
}

static int32_t onWrite(uint32_t lba, uint32_t offset, uint8_t *buffer, uint32_t bufsize) {
  const uint32_t sec = SD.sectorSize();
  if (offset || (bufsize % sec)) return -1;
  for (uint32_t i = 0; i < bufsize / sec; i++)
    if (!SD.writeRAW(buffer + i * sec, lba + i)) return -1;
  return bufsize;
}

static bool onStartStop(uint8_t, bool, bool) { return true; }

void setup() {
  pinMode(BAT_PWR, OUTPUT);
  digitalWrite(BAT_PWR, HIGH);
  Wire.begin(IIC_SDA, IIC_SCL);
  Wire.setClock(400000);
  exioWrite(0x01, 0xFF);     // vsechny EXIO vystupy v HIGH (povoleni panelu)
  exioWrite(0x03, 0x00);
  delay(50);
  gfx->begin(40000000);
  gfx->setBrightness(200);

  sdSpi.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS, sdSpi)) { screen("SD KARTA", "karta nenalezena", RGB565_RED); return; }

  msc.vendorID("ESP32");
  msc.productID("AMOLED SD");
  msc.productRevision("1.0");
  msc.onRead(onRead);
  msc.onWrite(onWrite);
  msc.onStartStop(onStartStop);
  msc.mediaPresent(true);
  msc.begin(SD.numSectors(), SD.sectorSize());
  USB.begin();

  char info[48];
  snprintf(info, sizeof(info), "%llu MB pres USB", SD.cardSize() / (1024ULL * 1024ULL));
  screen("USB DISK", info, RGB565_GREEN);
}

void loop() {
  delay(200);
}
