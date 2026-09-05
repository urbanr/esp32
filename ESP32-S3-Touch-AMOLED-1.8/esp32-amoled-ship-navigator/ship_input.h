#pragma once

#include <Arduino.h>
#include "../common/amoled_app.h"
#include "config.h"

// ===================================================================
// Vstupy: swipe z dotyku (touchDown/touchX/touchY plni sketch pres
// ../common/amoled_touch.h) a nabezna hrana tlacitka BOOT.
// ===================================================================

static bool     swPrev = false;
static int      swX0 = 0, swY0 = 0;
static uint32_t swT0 = 0;

// vyhodnoceni pri zvednuti prstu: +1 = tah doleva (dalsi obrazovka),
// -1 = tah doprava (predchozi), 0 = nic
static int inputSwipe() {
  int r = 0;
  if (touchDown && !swPrev) {
    swX0 = touchX;
    swY0 = touchY;
    swT0 = millis();
  } else if (!touchDown && swPrev) {
    const int dx = touchX - swX0, dy = touchY - swY0;   // touchX/Y drzi posledni pozici
    if (millis() - swT0 <= SWIPE_MAX_MS && abs(dx) >= SWIPE_MIN_PX && abs(dx) > abs(dy))
      r = dx < 0 ? 1 : -1;
  }
  swPrev = touchDown;
  return r;
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
