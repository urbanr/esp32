#pragma once

#include <string.h>
#include "driver/spi_master.h"
#include "soc/gpio_reg.h"
#include "Arduino_GFX_Library.h"
#include "pin_config.h"
#include "../common/amoled_app.h"
#include "config.h"
#include "rat_palette.h"

// ===================================================================
// Barevna "stara obrazovka" na displeji otocenem na sirku. Hra se
// kresli do 8bitoveho indexovaneho canvasu LW x LH (bod = 3x3 px
// displeje); na displej jde po fyzickych pruzich 368 x STRIPE_H pres
// vlastni SPI/DMA zarizeni (jako esp32-amoled-starfield). Fyzicky
// radek displeje odpovida logickemu sloupci hry (a naopak), takze se
// pruh sklada "napric": pro kazdy fyzicky radek se vezme jeden logicky
// sloupec pres vsechny logicke radky. Filtr:
//   - stopa paprsku: krajni tretiny bodu prolnute se sousednim bodem
//     ve smeru logickeho x,
//   - scanlines: z trojice fyzickych sloupcu (= logicky radek) je
//     stredni plny, horni a dolni tmavsi a prosvicene sousednim radkem,
//   - RGB maska (aperture grille): trojice fyzickych radku v bodu nese
//     po rade cervenou, zelenou a modrou, cizi kanaly utlumene o CRT_MASK,
//   - vinetace a blikani jasu snimku.
// ===================================================================

static_assert(CRT_SCALE == 3, "filtr pocita s 3 px na bod");
static_assert(LCD_HEIGHT % STRIPE_H == 0, "STRIPE_H musi delit LCD_HEIGHT");
#define STRIPE_BYTES (LCD_WIDTH * STRIPE_H * 2)
static_assert(STRIPE_BYTES <= AMOLED_SPI_MAX_TRANSFER, "pruh musi byt <= AMOLED_SPI_MAX_TRANSFER (common/amoled_app.h)");

static Arduino_Canvas_Indexed *lr = nullptr;   // buffer hry (indexy palety)
static uint8_t *fb = nullptr;                  // jeho framebuffer LW * LH

static uint16_t stripes[2][LCD_WIDTH * STRIPE_H] __attribute__((aligned(16)));
static spi_device_handle_t lcdDev = nullptr;
static spi_transaction_t colorTrans[2];
static spi_transaction_t cmdTrans[3];
static int pending = 0;

static uint8_t palR[256], palG[256], palB[256];
static uint8_t colLx[LCD_HEIGHT], colP[LCD_HEIGHT];    // fyzicky radek -> logicky sloupec, faze 0-2
static uint8_t rowLy[LCD_WIDTH], rowK[LCD_WIDTH];      // fyzicky sloupec -> logicky radek, faze 0-2
static uint16_t vigF[LCD_WIDTH > LCD_HEIGHT ? LCD_WIDTH : LCD_HEIGHT];   // vinetace pres fyzicky sloupec (0-256)
static uint16_t vigRow[LCD_HEIGHT];                    // vinetace pres fyzicky radek
static int maskW[3][3];                                // [faze radku][kanal] 0-512
static int rowW[3];                                    // vaha vlastniho radku (se zapocitanym blikanim)
static uint8_t blend[LH][3];                           // barvy logickeho sloupce po stope paprsku

static void crtBuildTables() {
  for (int i = 0; i < 256; i++) {
    const int c = i < C_COUNT ? i : 0;
    palR[i] = PALETTE[c][0];
    palG[i] = PALETTE[c][1];
    palB[i] = PALETTE[c][2];
  }
  for (int py = 0; py < LCD_HEIGHT; py++) {
    const int q = ROTATE_CW ? py : (LCD_HEIGHT - 1 - py);
    colLx[py] = (uint8_t)(q / CRT_SCALE);
    colP[py] = (uint8_t)(q % CRT_SCALE);
    const float d = (py - LCD_HEIGHT / 2.0f) / (LCD_HEIGHT / 2.0f);
    vigRow[py] = (uint16_t)(256 * (1.0f - CRT_VIGNETTE * d * d));
  }
  for (int px = 0; px < LCD_WIDTH; px++) {
    const int q = ROTATE_CW ? (LCD_WIDTH - 1 - px) : px;
    rowLy[px] = (uint8_t)(q / CRT_SCALE);
    rowK[px] = (uint8_t)(q % CRT_SCALE);
    const float d = (px - LCD_WIDTH / 2.0f) / (LCD_WIDTH / 2.0f);
    vigF[px] = (uint16_t)(256 * (1.0f - CRT_VIGNETTE * d * d));
  }
  for (int p = 0; p < 3; p++)
    for (int ch = 0; ch < 3; ch++)
      maskW[p][ch] = (int)((p == ch ? 256 : 256 - CRT_MASK) * CRT_MASK_GAIN);
}

static void IRAM_ATTR csLow(spi_transaction_t *) { REG_WRITE(GPIO_OUT_W1TC_REG, 1u << LCD_CS); }
static void IRAM_ATTR csHigh(spi_transaction_t *) { REG_WRITE(GPIO_OUT_W1TS_REG, 1u << LCD_CS); }

static bool crtDeviceInit() {
  if (lcdDev) return true;
  spi_device_interface_config_t devcfg = {};
  devcfg.command_bits = 8;
  devcfg.address_bits = 24;
  devcfg.mode = 0;
  devcfg.clock_source = SPI_CLK_SRC_DEFAULT;
  devcfg.clock_speed_hz = LCD_QSPI_HZ;
  devcfg.spics_io_num = -1;
  devcfg.pre_cb = csLow;
  devcfg.post_cb = csHigh;
  devcfg.flags = SPI_DEVICE_HALFDUPLEX;
  devcfg.queue_size = 8;
  return spi_bus_add_device(SPI2_HOST, &devcfg, &lcdDev) == ESP_OK;
}

// canvas (indexy palety primo jako "barvy") + tabulky + SPI zarizeni
static bool crtInit(Arduino_GFX *gfx) {
  crtBuildTables();
  if (!lr) {
    lr = new Arduino_Canvas_Indexed(LW, LH, gfx, 0, 0);
    if (!lr->begin(GFX_SKIP_OUTPUT_BEGIN)) return false;
    lr->setDirectUseColorIndex(true);
    lr->setTextWrap(false);
    fb = lr->getFramebuffer();
  }
  return crtDeviceInit();
}

static void queueTrans(spi_transaction_t *t) {
  ESP_ERROR_CHECK(spi_device_queue_trans(lcdDev, t, portMAX_DELAY));
  pending++;
}

static void waitPending(int maxLeft) {
  spi_transaction_t *r;
  while (pending > maxLeft) {
    spi_device_get_trans_result(lcdDev, &r, portMAX_DELAY);
    pending--;
  }
}

static void queueCmd(int slot, uint8_t reg, uint16_t d1, uint16_t d2, bool withData) {
  spi_transaction_t &t = cmdTrans[slot];
  memset(&t, 0, sizeof(t));
  t.flags = SPI_TRANS_MULTILINE_CMD | SPI_TRANS_MULTILINE_ADDR;
  t.cmd = 0x02;
  t.addr = (uint32_t)reg << 8;
  if (withData) {
    t.flags |= SPI_TRANS_USE_TXDATA;
    t.tx_data[0] = d1 >> 8;
    t.tx_data[1] = d1;
    t.tx_data[2] = d2 >> 8;
    t.tx_data[3] = d2;
    t.length = 32;
  }
  queueTrans(&t);
}

static void queueStripe(int buf) {
  spi_transaction_t &t = colorTrans[buf];
  memset(&t, 0, sizeof(t));
  t.flags = SPI_TRANS_MODE_QIO;
  t.cmd = 0x32;
  t.addr = 0x003C00;
  t.tx_buffer = stripes[buf];
  t.length = STRIPE_BYTES * 8;
  queueTrans(&t);
}

// barvy logickeho sloupce c pro fazi p (stopa paprsku: krajni tretina
// bodu = 3 dily vlastni + 1 dil soused ve smeru logickeho x)
static void blendColumn(int c, int p) {
  int nb = c;
  if (p == 0 && c > 0) nb = c - 1;
  else if (p == 2 && c < LW - 1) nb = c + 1;
  const uint8_t *col = fb + c, *ncol = fb + nb;
  for (int r = 0; r < LH; r++, col += LW, ncol += LW) {
    const int i = *col, j = *ncol;
    if (p == 1 || i == j) {
      blend[r][0] = palR[i]; blend[r][1] = palG[i]; blend[r][2] = palB[i];
    } else {
      blend[r][0] = (3 * palR[i] + palR[j]) >> 2;
      blend[r][1] = (3 * palG[i] + palG[j]) >> 2;
      blend[r][2] = (3 * palB[i] + palB[j]) >> 2;
    }
  }
}

static inline uint16_t pack565swapped(int r, int g, int b) {
  if (r > 255) r = 255;
  if (g > 255) g = 255;
  if (b > 255) b = 255;
  const uint16_t c = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
  return (c >> 8) | (c << 8);
}

// jeden fyzicky pruh: pro kazdy fyzicky radek jeden logicky sloupec
static void composeStripe(uint16_t *buf, int y0) {
  for (int py = y0; py < y0 + STRIPE_H; py++) {
    const int c = colLx[py], p = colP[py];
    blendColumn(c, p);
    const int *mw = maskW[p];
    const int vr = vigRow[py];
    uint16_t *out = buf + (py - y0) * LCD_WIDTH;
    for (int px = 0; px < LCD_WIDTH; px++) {
      const int r = rowLy[px], k = rowK[px];
      const uint8_t *b0 = blend[r];
      int cr = b0[0] * rowW[k], cg = b0[1] * rowW[k], cb = b0[2] * rowW[k];
      if (k == 0 && r > 0) {
        const uint8_t *b1 = blend[r - 1];
        cr += b1[0] * CRT_ROW_BLEED; cg += b1[1] * CRT_ROW_BLEED; cb += b1[2] * CRT_ROW_BLEED;
      } else if (k == 2 && r < LH - 1) {
        const uint8_t *b1 = blend[r + 1];
        cr += b1[0] * CRT_ROW_BLEED; cg += b1[1] * CRT_ROW_BLEED; cb += b1[2] * CRT_ROW_BLEED;
      }
      const int v = (vigF[px] * vr) >> 8;   // vinetace 0-256
      cr = (((cr >> 8) * mw[0]) >> 8) * v >> 8;
      cg = (((cg >> 8) * mw[1]) >> 8) * v >> 8;
      cb = (((cb >> 8) * mw[2]) >> 8) * v >> 8;
      out[px] = pack565swapped(cr, cg, cb);
    }
  }
}

static void crtPresent() {
  const int flick = 256 - random(CRT_FLICKER + 1);
  rowW[0] = (CRT_ROW_TOP * flick) >> 8;
  rowW[1] = flick;
  rowW[2] = (CRT_ROW_BOT * flick) >> 8;

  int stripeIdx = 0;
  for (int y0 = 0; y0 < LCD_HEIGHT; y0 += STRIPE_H, stripeIdx++) {
    const int buf = stripeIdx & 1;
    waitPending(1);
    composeStripe(stripes[buf], y0);
    queueCmd(0, 0x2A, 0, LCD_WIDTH - 1, true);
    queueCmd(1, 0x2B, y0, y0 + STRIPE_H - 1, true);
    queueCmd(2, 0x2C, 0, 0, false);
    queueStripe(buf);
  }
}

static void crtEnd() {
  waitPending(0);
  delete lr;
  lr = nullptr;
  fb = nullptr;
}
