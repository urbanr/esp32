#pragma once

#include <stdint.h>
#include "config.h"

// piskova paleta: zakladni barva + 10 tmavsich odstinu, nikdy svetlejsi
static uint16_t sandPalette[11];
static uint16_t bgColor565;

static inline uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

static void buildPalette() {
  const float contrast = PALETTE_CONTRAST_PCT / 100.0f;
  for (int i = 0; i <= 10; i++) {
    const float f = 1.0f - contrast * (i / 10.0f) * MAX_DARKEN;
    sandPalette[i] = rgb565((uint8_t)(SAND_BASE_R * f),
                            (uint8_t)(SAND_BASE_G * f),
                            (uint8_t)(SAND_BASE_B * f));
  }
  bgColor565 = rgb565(BG_R, BG_G, BG_B);
}
