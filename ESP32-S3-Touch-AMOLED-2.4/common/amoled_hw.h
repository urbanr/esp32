#pragma once

#include <Arduino.h>
#include <Wire.h>
#include "Arduino_GFX_Library.h"
#include "driver/spi_master.h"
#include "pin_config.h"
#include "HWCDC.h"
#include "amoled_app.h"
#include "amoled_boot.h"

// ===================================================================
// Sdilena inicializace hardwaru Waveshare ESP32-S3-Touch-AMOLED-2.41:
// USBSerial, napajeni baterie, I2C (s recovery), expander (povoleni
// displeje), SPI sbernice displeje a panel RM690B0 pres Arduino_GFX.
// Obsahuje definice objektu - includovat POUZE z hlavniho .ino;
// moduly aplikaci pouzivaji amoled_app.h.
// ===================================================================

HWCDC USBSerial;

// is_shared_interface = true: knihovna si sbernici nezamkne natrvalo,
// drzi ji jen behem zapisu - vlastni DMA zarizeni (rat_crt.h) se k ni dostane
static Arduino_DataBus *bus = new Arduino_ESP32QSPI(
  LCD_CS, LCD_SCLK, LCD_SDIO0, LCD_SDIO1, LCD_SDIO2, LCD_SDIO3, true);

Arduino_RM690B0 *gfx = new Arduino_RM690B0(
  bus, LCD_RST, 0, LCD_WIDTH, LCD_HEIGHT,
  LCD_X_OFF, LCD_Y_OFF, LCD_X_OFF, LCD_Y_OFF);

// uvolneni I2C sbernice zaseknute slavem drzicim SDA (po soft resetu
// uprostred transakce): 9 pulzu na SCL + STOP condition
static void i2cBusRecover() {
  pinMode(IIC_SDA, INPUT_PULLUP);
  pinMode(IIC_SCL, OUTPUT);
  for (int i = 0; i < 9 && digitalRead(IIC_SDA) == LOW; i++) {
    digitalWrite(IIC_SCL, LOW);
    delayMicroseconds(5);
    digitalWrite(IIC_SCL, HIGH);
    delayMicroseconds(5);
  }
  pinMode(IIC_SDA, OUTPUT);
  digitalWrite(IIC_SDA, LOW);
  delayMicroseconds(5);
  digitalWrite(IIC_SCL, HIGH);
  delayMicroseconds(5);
  digitalWrite(IIC_SDA, HIGH);
  delayMicroseconds(5);
  pinMode(IIC_SDA, INPUT_PULLUP);
  pinMode(IIC_SCL, INPUT_PULLUP);
}

// fatalni chyba: vypis a zastaveni
static void hwHalt(const char *msg) {
  USBSerial.println(msg);
  while (1) delay(1000);
}

// zapis registru expanderu (0x01 = vystupy, 0x03 = smer, 0 = vystup)
static bool exioWrite(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(EXIO_ADDR);
  Wire.write(reg);
  Wire.write(val);
  return Wire.endTransmission() == 0;
}

// vsechny EXIO jako vystupy v HIGH (mezi nimi AMOLED_EN). Overeno
// pokusem: s piny jako vstupy nebo se vsemi v LOW zustava panel tmavy.
static bool exioInit() {
  const bool a = exioWrite(0x01, 0xFF);
  const bool b = exioWrite(0x03, 0x00);
  return a && b;
}

// vypis adres na I2C (kontrola, co je na desce osazeno)
static void i2cScan() {
  USBSerial.print("I2C:");
  for (uint8_t a = 1; a < 127; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) USBSerial.printf(" 0x%02X", a);
  }
  USBSerial.println();
}

// SPI sbernice displeje s vetsi max. transakci, nez pouziva knihovna
// (pruhy CRT filtru); volat PRED gfx->begin(GFX_SKIP_DATABUS_UNDERLAYING_BEGIN),
// knihovna pak sbernici neinicializuje znovu, jen prida sve zarizeni
static bool hwSpiBusInit() {
  spi_bus_config_t buscfg = {};
  buscfg.mosi_io_num = LCD_SDIO0;
  buscfg.miso_io_num = LCD_SDIO1;
  buscfg.sclk_io_num = LCD_SCLK;
  buscfg.quadwp_io_num = LCD_SDIO2;
  buscfg.quadhd_io_num = LCD_SDIO3;
  buscfg.data4_io_num = -1;
  buscfg.data5_io_num = -1;
  buscfg.data6_io_num = -1;
  buscfg.data7_io_num = -1;
  buscfg.max_transfer_sz = AMOLED_SPI_MAX_TRANSFER;
  buscfg.flags = SPICOMMON_BUSFLAG_MASTER | SPICOMMON_BUSFLAG_GPIO_PINS;
  buscfg.isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO;
  return spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO) == ESP_OK;
}

// kompletni inicializace hardwaru; po navratu je displej cerny a
// pripraveny na kresleni pres gfx
static void hwInit() {
  USBSerial.begin(115200);
  // bez timeoutu: kdyz port na PC nikdo necte, printf by blokoval smycku
  // (~1 fps), takto se necteny vystup zahodi
  USBSerial.setTxTimeoutMs(0);

  pinMode(BAT_PWR, OUTPUT);
  digitalWrite(BAT_PWR, HIGH);

  i2cBusRecover();
  Wire.begin(IIC_SDA, IIC_SCL);
  Wire.setClock(400000);
  i2cScan();

  if (!exioInit()) USBSerial.println("expander 0x20 neodpovida - displej nejspis zustane tmavy");
  delay(50);

  if (!hwSpiBusInit()) hwHalt("SPI bus init fail");
  gfx->begin(GFX_SKIP_DATABUS_UNDERLAYING_BEGIN);
  gfx->fillScreen(0x0000);
  gfx->setBrightness(AMOLED_BRIGHTNESS);

  bootReturnToLauncher();
}
