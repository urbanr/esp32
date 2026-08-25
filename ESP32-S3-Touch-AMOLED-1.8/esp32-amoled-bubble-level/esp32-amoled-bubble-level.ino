// esp32-amoled-bubble-level - kruhova vodovaha rizena IMU
// Waveshare ESP32-S3-Touch-AMOLED-1.8, specifikace viz bubble-level.md

#include <Arduino.h>
#include <Wire.h>
#include "Arduino_GFX_Library.h"
#include <Adafruit_XCA9554.h>
#include "pin_config.h"
#include "HWCDC.h"

#include "config.h"
#include "level_input.h"
#include "level_render.h"

HWCDC USBSerial;

Adafruit_XCA9554 expander;

Arduino_DataBus *bus = new Arduino_ESP32QSPI(
  LCD_CS, LCD_SCLK, LCD_SDIO0, LCD_SDIO1, LCD_SDIO2, LCD_SDIO3);

Arduino_SH8601 *gfx = new Arduino_SH8601(
  bus, GFX_NOT_DEFINED, 0, LCD_WIDTH, LCD_HEIGHT);

#define TICK_DT (1.0f / TICK_HZ)

static uint32_t lastUs = 0;
static float timeAcc = 0;

// stav bubliny (stred, display px, float)
static float posX = LCD_CX, posY = LCD_CY;
static float velX = 0, velY = 0;
static int lastDrawnX = LCD_CX, lastDrawnY = LCD_CY;

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

// cilova pozice stredu bubliny z aktualniho naklonu: smer proti
// gravitaci v rovine displeje, vychylka pres nelinearni krivku
static void bubbleTarget(float &tx, float &ty) {
  float m = sqrtf(inGX * inGX + inGY * inGY);
  if (m < TILT_DEADZONE_G) { tx = LCD_CX; ty = LCD_CY; return; }
  const float dirX = -inGX / m, dirY = -inGY / m;
  if (m > 1) m = 1;
  const float theta = asinf(m) * RAD_TO_DEG;
  float u = theta / TILT_FULL_SCALE_DEG;
  if (u > 1) u = 1;
  const float r = BUBBLE_R_MAX_PX * powf(u, BUBBLE_CURVE_EXP);
  tx = LCD_CX + dirX * r;
  ty = LCD_CY + dirY * r;
}

static void bubbleTick() {
  float tx, ty;
  bubbleTarget(tx, ty);
#if BUBBLE_MOTION_MODE == MOTION_SMOOTH
  posX += (tx - posX) * SMOOTH_ALPHA;
  posY += (ty - posY) * SMOOTH_ALPHA;
#else
  velX += (tx - posX) * SPRING_K * TICK_DT;
  velY += (ty - posY) * SPRING_K * TICK_DT;
  velX *= DAMPING;
  velY *= DAMPING;
  posX += velX * TICK_DT;
  posY += velY * TICK_DT;
  // bublina nesmi opustit drahu - pri prestreleni srazit zpet
  const float dx = posX - LCD_CX, dy = posY - LCD_CY;
  const float d = sqrtf(dx * dx + dy * dy);
  if (d > BUBBLE_R_MAX_PX) {
    posX = LCD_CX + dx / d * BUBBLE_R_MAX_PX;
    posY = LCD_CY + dy / d * BUBBLE_R_MAX_PX;
    velX *= 0.5f;
    velY *= 0.5f;
  }
#endif
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

  if (!inputBeginIMU()) USBSerial.println("QMI8658 init fail");
  else USBSerial.println("QMI8658 OK");

  gfx->begin();
  if (!renderInit(gfx)) {
    USBSerial.println("canvas alloc fail");
    while (1) delay(1000);
  }
  drawStaticScene(gfx);
  renderBubbleMove(gfx, LCD_CX, LCD_CY, LCD_CX, LCD_CY);
  gfx->setBrightness(DISPLAY_BRIGHTNESS);

  lastUs = micros();
}

void loop() {
  const uint32_t now = micros();
  float dt = (now - lastUs) * 1e-6f;
  lastUs = now;
  if (dt > 0.1f) dt = 0.1f;
  timeAcc += dt;

  inputRead();

  bool ticked = false;
  while (timeAcc >= TICK_DT) {
    bubbleTick();
    timeAcc -= TICK_DT;
    ticked = true;
  }

  if (ticked) {
    const int x = (int)roundf(posX), y = (int)roundf(posY);
    if (x != lastDrawnX || y != lastDrawnY) {
      renderBubbleMove(gfx, lastDrawnX, lastDrawnY, x, y);
      lastDrawnX = x;
      lastDrawnY = y;
    }
    renderTiltText(tiltXDeg(), tiltYDeg());
  }
}
