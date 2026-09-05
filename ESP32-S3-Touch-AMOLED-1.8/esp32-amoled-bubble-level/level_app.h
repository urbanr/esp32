#pragma once

#include <Arduino.h>
#include "../common/amoled_app.h"
#include "config.h"
#include "level_input.h"
#include "level_render.h"

// ===================================================================
// Modul vodovahy: levelBegin() / levelLoop() / levelEnd() pouziva
// samostatny sketch i launcher. Hardware (USBSerial, I2C, displej)
// inicializuje sketch pres ../common/amoled_hw.h; pri vstupu do
// levelBegin() je displej cerny a jas nastaveny.
// ===================================================================

#define TICK_DT (1.0f / TICK_HZ)

static uint32_t lastUs = 0;
static float timeAcc = 0;

// stav bubliny (stred, display px, float)
static float posX = LCD_CX, posY = LCD_CY;
static float velX = 0, velY = 0;
static int lastDrawnX = LCD_CX, lastDrawnY = LCD_CY;
static bool zeroBtnPrev = false;   // predchozi stav tlacitka aretace (stisk = LOW)

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

// plna inicializace a prekresleni (jako puvodni setup() po inicializaci HW)
bool levelBegin() {
  if (!inputBeginIMU()) USBSerial.println("QMI8658 init fail");
  else USBSerial.println("QMI8658 OK");

  if (!renderInit(gfx)) {
    USBSerial.println("canvas alloc fail");
    return false;
  }
  // aretace: cisty start bez posunu
  pinMode(ZERO_BUTTON_PIN, INPUT_PULLUP);
  zeroAX = zeroAY = 0;

  posX = LCD_CX;
  posY = LCD_CY;
  velX = velY = 0;
  lastDrawnX = LCD_CX;
  lastDrawnY = LCD_CY;
  drawStaticScene(gfx);
  renderBubbleMove(gfx, LCD_CX, LCD_CY, LCD_CX, LCD_CY);
  // stisk drzeny uz pri startu (vcetne doby kresleni sceny) se nepocita
  zeroBtnPrev = (digitalRead(ZERO_BUTTON_PIN) == LOW);

  timeAcc = 0;
  lastUs = micros();
  return true;
}

void levelLoop() {
  const uint32_t now = micros();
  float dt = (now - lastUs) * 1e-6f;
  lastUs = now;
  if (dt > 0.1f) dt = 0.1f;
  timeAcc += dt;

  // aretace tlacitkem: nabezna hrana stisku (az po prvnim vzorku IMU)
  const bool zeroBtn = (digitalRead(ZERO_BUTTON_PIN) == LOW);
  if (zeroBtn && !zeroBtnPrev && rawPrimed) inputZero();
  zeroBtnPrev = zeroBtn;

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

// canvasy se uvolni, dokud bezi jina aplikace
void levelEnd() {
  renderFree();
}
