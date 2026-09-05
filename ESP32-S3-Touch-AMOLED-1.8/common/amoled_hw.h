#pragma once

#include <Arduino.h>
#include <Wire.h>
#include "Arduino_GFX_Library.h"
#include <Adafruit_XCA9554.h>
#include "driver/spi_master.h"
#include "pin_config.h"
#include "HWCDC.h"
#include "amoled_app.h"

// ===================================================================
// Sdilena inicializace hardwaru Waveshare ESP32-S3-Touch-AMOLED-1.8:
// USBSerial, I2C (s recovery), expander XCA9554 (napajeni displeje),
// SPI sbernice displeje a panel SH8601 pres Arduino_GFX.
// Obsahuje definice objektu - includovat POUZE z hlavniho .ino
// (launcher i samostatne aplikace); moduly aplikaci pouzivaji
// amoled_app.h.
// ===================================================================

HWCDC USBSerial;

static Adafruit_XCA9554 expander;

// is_shared_interface = true: knihovna si sbernici nezamkne natrvalo,
// drzi ji jen behem zapisu - vlastni DMA zarizeni hvezd (star_render.h)
// se k ni dostane
static Arduino_DataBus *bus = new Arduino_ESP32QSPI(
  LCD_CS, LCD_SCLK, LCD_SDIO0, LCD_SDIO1, LCD_SDIO2, LCD_SDIO3, true);

Arduino_SH8601 *gfx = new Arduino_SH8601(
  bus, GFX_NOT_DEFINED, 0, LCD_WIDTH, LCD_HEIGHT);

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

// SPI sbernice displeje s vetsi max. transakci, nez pouziva knihovna
// (pruhy hvezd); volat PRED gfx->begin(GFX_SKIP_DATABUS_UNDERLAYING_BEGIN),
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

  i2cBusRecover();
  Wire.begin(IIC_SDA, IIC_SCL);
  Wire.setClock(400000);

  bool expanderOk = false;
  for (int t = 0; t < 5 && !expanderOk; t++) {
    expanderOk = expander.begin(0x20);
    if (!expanderOk) delay(200);
  }
  if (!expanderOk) hwHalt("XCA9554 init fail");
  expander.pinMode(0, OUTPUT);
  expander.pinMode(1, OUTPUT);
  expander.pinMode(2, OUTPUT);
  expander.digitalWrite(0, LOW);
  expander.digitalWrite(1, LOW);
  expander.digitalWrite(2, LOW);
  delay(20);
  expander.digitalWrite(0, HIGH);
  expander.digitalWrite(1, HIGH);
  expander.digitalWrite(2, HIGH);

  if (!hwSpiBusInit()) hwHalt("SPI bus init fail");
  gfx->begin(GFX_SKIP_DATABUS_UNDERLAYING_BEGIN);
  gfx->fillScreen(0x0000);
  gfx->setBrightness(AMOLED_BRIGHTNESS);
}
