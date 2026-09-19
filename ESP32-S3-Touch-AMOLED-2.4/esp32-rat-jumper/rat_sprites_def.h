#pragma once

#include <Arduino.h>
#include <string.h>

// sprite jako ASCII art: radky stejne delky, znak -> barva podle legendy
// (rat_palette.h), '.' = pruhledna; kresli se primo z retezcu
struct Sprite {
  const char *const *rows;
  uint8_t w, h;
};

#define SPRITE(name, ...) \
  static const char *const name##_rows[] = { __VA_ARGS__ }; \
  static const Sprite name = { name##_rows, (uint8_t)strlen(name##_rows[0]), (uint8_t)(sizeof(name##_rows) / sizeof(name##_rows[0])) };

