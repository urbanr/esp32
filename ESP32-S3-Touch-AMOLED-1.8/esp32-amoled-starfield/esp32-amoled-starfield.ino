// esp32-amoled-starfield - prulet hvezdnym polem rizeny IMU
// Waveshare ESP32-S3-Touch-AMOLED-1.8, specifikace viz starfield.md

#include <Arduino.h>
#include <Wire.h>
#include "Arduino_GFX_Library.h"
#include <Adafruit_XCA9554.h>
#include "pin_config.h"
#include "HWCDC.h"

#include "config.h"
#include "star_input.h"
#include "star_field.h"
#include "star_render.h"

HWCDC USBSerial;

Adafruit_XCA9554 expander;

// is_shared_interface = true: knihovna si bus nezamkne natrvalo,
// pixely posila vlastni DMA zarizeni v star_render.h
Arduino_DataBus *bus = new Arduino_ESP32QSPI(
  LCD_CS, LCD_SCLK, LCD_SDIO0, LCD_SDIO1, LCD_SDIO2, LCD_SDIO3, true);

Arduino_SH8601 *gfx = new Arduino_SH8601(
  bus, GFX_NOT_DEFINED, 0, LCD_WIDTH, LCD_HEIGHT);

static uint32_t lastUs = 0;

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

// diagnostika: fps, casy fazi (us/snimek), stav IMU, orientace
static void debugPrint(float dt, uint32_t usSim, uint32_t usProject, uint32_t usRender) {
#if DEBUG_PERIOD_MS > 0
  static uint32_t lastMs = 0, simSum = 0, projSum = 0, rendSum = 0;
  static int frames = 0;
  static float timeSum = 0;
  frames++;
  timeSum += dt;
  simSum += usSim;
  projSum += usProject;
  rendSum += usRender;
  const uint32_t now = millis();
  if (now - lastMs < DEBUG_PERIOD_MS) return;
  lastMs = now;
  USBSerial.printf("fps %.1f  us sim %lu proj %lu render %lu  imu %d calib %d  |a| %.3f |w| %.2f az %+.2f  gerr %.1f gsign %+.3f  g(%+.2f %+.2f %+.2f)  flow(%+.2f %+.2f %+.2f)  dots %d\n",
                   frames / timeSum, simSum / frames, projSum / frames, rendSum / frames,
                   imuOk, inputCalibrated(), rawAMag, rawWMag, rawAz, gErrMaxDeg, gSignScore,
                   gDir.x, gDir.y, gDir.z, flowDir.x, flowDir.y, flowDir.z, dotCount);
  gErrMaxDeg = 0;
  gSignScore = 0;
  frames = 0;
  timeSum = 0;
  simSum = projSum = rendSum = 0;
#endif
}

void setup() {
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

  if (!inputBeginIMU()) USBSerial.println("QMI8658 init fail - hvezdy poleti primo na divaka");
  else USBSerial.println("QMI8658 OK");

  // SPI bus inicializujeme sami (vetsi DMA transakce), knihovna jen
  // inicializuje panel; po renderInit() uz pres gfx nekreslit
  if (!renderBusInit()) {
    USBSerial.println("SPI bus init fail");
    while (1) delay(1000);
  }
  gfx->begin(GFX_SKIP_DATABUS_UNDERLAYING_BEGIN);
  gfx->fillScreen(0x0000);
  gfx->setBrightness(DISPLAY_BRIGHTNESS);
  if (!renderInit()) {
    USBSerial.println("SPI device init fail");
    while (1) delay(1000);
  }
  fieldInit();

  lastUs = micros();
}

void loop() {
  const uint32_t now = micros();
  float dt = (now - lastUs) * 1e-6f;
  lastUs = now;
  if (dt > 0.1f) dt = 0.1f;

  inputRead(dt);
  fieldUpdate(dt);
  const uint32_t t1 = micros();

  Vec3 U, V, W;
  inputBasis(U, V, W);
  projectStars(U, V, W);
  const uint32_t t2 = micros();

  renderFrame();
  const uint32_t t3 = micros();

  debugPrint(dt, t1 - now, t2 - t1, t3 - t2);
}
