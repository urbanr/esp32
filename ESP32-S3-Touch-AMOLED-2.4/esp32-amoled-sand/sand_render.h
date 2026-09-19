#pragma once

#include "Arduino_GFX_Library.h"
#include "config.h"
#include "sand_palette.h"
#include "sand_sim.h"

// ===================================================================
// Diferencialni render: kresli se jen zmenene bunky (dirty bity),
// souvisle useky v radku jednim draw16bitRGBBitmap. Zadne plosne
// mazani -> zadne blikani, zadny fullscreen buffer.
// ===================================================================

static uint16_t runBuf[GRID_W * SAND_SCALE * SAND_SCALE];

static inline bool isDirty(int i) {
  return dirtyBits[i >> 5] & (1u << (i & 31));
}

static void renderDirty(Arduino_GFX *dst) {
  for (int y = 0; y < GRID_H; y++) {
    const int base = y * GRID_W;
    int x = 0;
    while (x < GRID_W) {
      int i = base + x;
      // preskok celych cistych 32bitovych slov
      if ((i & 31) == 0) {
        while (x + 32 <= GRID_W && dirtyBits[i >> 5] == 0) {
          x += 32;
          i += 32;
        }
        if (x >= GRID_W) break;
      }
      if (!isDirty(i)) {
        x++;
        continue;
      }

      // souvisly usek dirty bunek v radku
      const int x0 = x;
      int px = 0;
      while (x < GRID_W && isDirty(base + x)) {
        const int j = base + x;
        dirtyBits[j >> 5] &= ~(1u << (j & 31));
        const uint16_t c = (cellState[j] == CELL_EMPTY)
                             ? bgColor565
                             : sandPalette[cellColor[j]];
        for (int s = 0; s < SAND_SCALE; s++) runBuf[px++] = c;
        x++;
      }
      for (int s = 1; s < SAND_SCALE; s++)
        memcpy(&runBuf[s * px], runBuf, px * sizeof(uint16_t));
      dst->draw16bitRGBBitmap(GRID_X_OFF + x0 * SAND_SCALE,
                              GRID_Y_OFF + y * SAND_SCALE,
                              runBuf, px, SAND_SCALE);
    }
  }
}
