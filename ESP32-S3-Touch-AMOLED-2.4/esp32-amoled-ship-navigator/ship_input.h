#pragma once

#include <Arduino.h>
#include "../common/amoled_app.h"
#include "config.h"

// ===================================================================
// Vstupy: swipe z dotyku (touchDown/touchX/touchY plni sketch pres
// ../common/amoled_touch.h) a nabezna hrana tlacitka BOOT.
// ===================================================================

enum : uint8_t { G_NONE = 0, G_LEFT, G_RIGHT, G_UP, G_DOWN, G_TAP };

static bool     swPrev = false;
static int      swX0 = 0, swY0 = 0;
static uint32_t swT0 = 0;

// gesto vyhodnocene pri zvednuti prstu: tah doleva/doprava/nahoru/dolu
// (posun >= SWIPE_MIN_PX v prevazujicim smeru) nebo tuknuti (kratke, bez posunu)
static uint8_t inputGesture() {
  uint8_t g = G_NONE;
  if (touchDown && !swPrev) {
    swX0 = touchX;
    swY0 = touchY;
    swT0 = millis();
  } else if (!touchDown && swPrev) {
    const int dx = touchX - swX0, dy = touchY - swY0;   // touchX/Y drzi posledni pozici
    const uint32_t dur = millis() - swT0;
    if (dur <= SWIPE_MAX_MS) {
      if (abs(dx) >= SWIPE_MIN_PX && abs(dx) > abs(dy)) g = dx < 0 ? G_LEFT : G_RIGHT;
      else if (abs(dy) >= SWIPE_MIN_PX) g = dy < 0 ? G_UP : G_DOWN;
      else if (abs(dx) < TAP_MOVE_PX && abs(dy) < TAP_MOVE_PX && dur <= TAP_MAX_MS) g = G_TAP;
    }
  }
  swPrev = touchDown;
  return g;
}

static bool btnPrev = false;

static void inputBeginButton() {
  pinMode(FLIGHT_BUTTON_PIN, INPUT_PULLUP);
  btnPrev = (digitalRead(FLIGHT_BUTTON_PIN) == LOW);   // stisk drzeny pri startu se nepocita
}

// true prave jednou pri stisku
static bool inputButtonPressed() {
  const bool b = (digitalRead(FLIGHT_BUTTON_PIN) == LOW);
  const bool edge = b && !btnPrev;
  btnPrev = b;
  return edge;
}
