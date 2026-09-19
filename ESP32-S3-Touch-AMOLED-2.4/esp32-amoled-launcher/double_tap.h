#pragma once

#include <Arduino.h>
#include "config.h"

// ===================================================================
// Detekce dvojkliku z polled stavu dotyku (volat kazdou otocku smycky).
// Tuknuti = stisk kratsi nez TAP_MAX_MS bez pohybu > TAP_MOVE_PX;
// dvojklik = druhe tuknuti do DOUBLE_TAP_GAP_MS od uvolneni prvniho,
// nejdal DOUBLE_TAP_DIST_PX od nej. Vyhodnocuje se pri uvolneni
// druheho tuknuti, takze v okamziku prepnuti uz prst na displeji neni.
// ===================================================================

static bool     tapPrevDown = false;
static bool     tapMoved = false;      // aktualni stisk se pohnul vic nez TAP_MOVE_PX
static bool     tapHaveFirst = false;  // prvni tuknuti ceka na druhe
static uint32_t tapDownMs = 0, tapUpMs = 0;
static int      tapDownX = 0, tapDownY = 0;   // pozice aktualniho stisku
static int      tapFirstX = 0, tapFirstY = 0; // pozice prvniho tuknuti

static inline bool tapFarther(int ax, int ay, int bx, int by, int d) {
  const int dx = ax - bx, dy = ay - by;
  return dx * dx + dy * dy > d * d;
}

// vraci true prave jednou, pri uvolneni druheho tuknuti dvojkliku
static bool doubleTapUpdate(bool down, int x, int y) {
  const uint32_t now = millis();
  bool fired = false;
  if (down && !tapPrevDown) {                     // stisk
    tapDownMs = now;
    tapDownX = x;
    tapDownY = y;
    tapMoved = false;
    // prvni tuknuti propada, kdyz druhy stisk prisel pozde nebo jinam
    if (tapHaveFirst && (now - tapUpMs > DOUBLE_TAP_GAP_MS ||
                         tapFarther(x, y, tapFirstX, tapFirstY, DOUBLE_TAP_DIST_PX)))
      tapHaveFirst = false;
  } else if (down) {                              // drzeni
    if (tapFarther(x, y, tapDownX, tapDownY, TAP_MOVE_PX)) tapMoved = true;
  } else if (tapPrevDown) {                       // uvolneni
    const bool tap = !tapMoved && (now - tapDownMs <= TAP_MAX_MS);
    if (tap && tapHaveFirst) {
      fired = true;
      tapHaveFirst = false;
    } else if (tap) {
      tapHaveFirst = true;
      tapFirstX = tapDownX;
      tapFirstY = tapDownY;
      tapUpMs = now;
    } else {
      tapHaveFirst = false;
    }
  }
  tapPrevDown = down;
  return fired;
}
