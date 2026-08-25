// esp32-amoled-sand - interaktivni falling-sand simulace rizena IMU
// Waveshare ESP32-S3-Touch-AMOLED-1.8, specifikace viz sand.md

#include <Arduino.h>
#include <Wire.h>
#include "Arduino_GFX_Library.h"
#include <Adafruit_XCA9554.h>
#include "pin_config.h"
#include "HWCDC.h"
#include <esp_random.h>

#include "config.h"
#include "sand_palette.h"
#include "sand_sim.h"
#include "sand_input.h"
#include "sand_render.h"

HWCDC USBSerial;

Adafruit_XCA9554 expander;

Arduino_DataBus *bus = new Arduino_ESP32QSPI(
  LCD_CS, LCD_SCLK, LCD_SDIO0, LCD_SDIO1, LCD_SDIO2, LCD_SDIO3);

Arduino_SH8601 *gfx = new Arduino_SH8601(
  bus, GFX_NOT_DEFINED, 0, LCD_WIDTH, LCD_HEIGHT);

static uint32_t lastUs = 0;
static float timeAcc = 0;

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

void setup() {
  USBSerial.begin(115200);

  i2cBusRecover();
  Wire.begin(IIC_SDA, IIC_SCL);
  Wire.setClock(400000);

  bool expanderOk = false;
  for (int t = 0; t < 5 && !expanderOk; t++) {
    expanderOk = expander.begin(0x20);
    if (!expanderOk) delay(200);
  }
  if (!expanderOk) {
    USBSerial.println("XCA9554 init fail");
    while (1) delay(1000);
  }
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

  if (!inputBeginTouch()) USBSerial.println("FT3168 init fail");
  else USBSerial.println("FT3168 OK");

  if (!inputBeginIMU()) USBSerial.println("QMI8658 init fail");
  else USBSerial.println("QMI8658 OK");

  gfx->begin();
  buildPalette();
  gfx->fillScreen(bgColor565);
  gfx->setBrightness(DISPLAY_BRIGHTNESS);

  pinMode(POUR_BUTTON_PIN, INPUT_PULLUP);

  simInit(esp_random());
  lastUs = micros();
}

void loop() {
  const uint32_t now = micros();
  float dt = (now - lastUs) * 1e-6f;
  lastUs = now;
  if (dt > 0.1f) dt = 0.1f;
  timeAcc += dt;

  inputRead();
  simSetGravity(inGX, inGY);

  const bool pouring = (digitalRead(POUR_BUTTON_PIN) == LOW);

  bool ticked = false;
  while (timeAcc >= TICK_DT) {
    simTick(inTouchDown, inTouchX, inTouchY, inShake, pouring);
    timeAcc -= TICK_DT;
    ticked = true;
  }
  if (ticked) renderDirty(gfx);
}
