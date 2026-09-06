#pragma once

#include <string.h>
#include "driver/spi_master.h"
#include "soc/gpio_reg.h"
#include "Arduino_GFX_Library.h"
#include "pin_config.h"
#include "../common/amoled_app.h"
#include "config.h"
#include "zomb_gfx.h"     // PALETTE, C_COUNT

// ===================================================================
// Barevna "stara obrazovka" (kopie rat_crt.h z krysy skokana). Hra se
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
static uint32_t usWait = 0, usCompose = 0, usBlend = 0;   // diagnostika casu snimku

static uint8_t palR[256], palG[256], palB[256];
static uint8_t colLx[LCD_HEIGHT], colP[LCD_HEIGHT];    // fyzicky radek -> logicky sloupec, faze 0-2
static uint8_t rowLy[LCD_WIDTH], rowK[LCD_WIDTH];      // fyzicky sloupec -> logicky radek, faze 0-2
static uint8_t vigL[LCD_WIDTH], vigRowL[LCD_HEIGHT];   // uroven jasu 0-7 (sloupec = logicky radek, radek)
static uint8_t vigBase[LCD_WIDTH];                     // uroven bez putujiciho pruhu
// [uroven vinetace][faze radku = kanal masky][kanal][hodnota] -> prispevek do RGB565
// s prohozenymi bajty (OR tri prispevku = cely pixel); maska a vinetace uz zapocitane
static uint16_t lut16[8][3][3][256];
static uint8_t flickLut[256];                          // jas snimku (blikani)
static uint8_t topLut[256], botLut[256], bleedLut[256];  // vahy radku scanline (horni, dolni, prosvit)
#if !CRT_SOFT
// rychly rezim: [uroven vinetace][faze radku p][faze sloupce k][index palety] -> cely pixel RGB565 (prohozene bajty)
static uint16_t fastLut[8][3][3][256];
static uint8_t colIdxBuf[LH];                          // indexy aktualniho logickeho sloupce
static int fastCol = -1;
static int16_t rPx0[LH];                               // fyzicky sloupec faze k=0 pro logicky radek r
static uint8_t rCnt[LH];                               // pocet fyzickych sloupcu radku (3, krajni 2)
#endif
static uint16_t preOff[LCD_WIDTH];                     // fyzicky sloupec -> offset do colBuf[.] (k, radek)

// parametry filtru za behu (predvolby prepina hra swipem dolu, viz zomb_crt_presets.h);
// vychozi hodnoty z config.h, po zmene znovu zavolat crtBuildTables()
static int crtRowTop = CRT_ROW_TOP, crtRowBot = CRT_ROW_BOT, crtRowBleed = CRT_ROW_BLEED, crtMask = CRT_MASK, crtFlicker = CRT_FLICKER;
static float crtMaskGain = CRT_MASK_GAIN, crtVignette = CRT_VIGNETTE;
static float crtGamma = 1.0f;          // gama palety
static int crtHum = 0;                 // hloubka putujiciho pruhu jasu 0-255 (0 = zadny)
static int crtNoise = 0;               // pocet nahodnych jisker na snimek
static float crtHumPos = 0;
static int crtMode = 0;                // 0 = TECKY (tvrde scanlines, RGB maska, pruh, blikani), 1 = LINKY (mekke radky, rozmazani)
static int crtBlend = 96;              // LINKY: vodorovne rozmazani 0-255 (podil souseda v krajnich tretinach bodu)

static inline uint16_t part565swapped(int ch, int v) {
  uint16_t c = ch == 0 ? ((v & 0xF8) << 8) : (ch == 1 ? ((v & 0xFC) << 3) : (v >> 3));
  return (c >> 8) | (c << 8);
}

static void softBuild();
static void crtBuildTables() {
  for (int i = 0; i < 256; i++) {
    const uint8_t *c = PALETTE[0];
    if (i < C_COUNT) c = PALETTE[i];
#ifdef PALETTE_EXTRA_COUNT
    else if (i - C_COUNT < PALETTE_EXTRA_COUNT) c = PALETTE_EXTRA[i - C_COUNT];
#endif
    palR[i] = (uint8_t)lroundf(255.0f * powf(c[0] / 255.0f, crtGamma));
    palG[i] = (uint8_t)lroundf(255.0f * powf(c[1] / 255.0f, crtGamma));
    palB[i] = (uint8_t)lroundf(255.0f * powf(c[2] / 255.0f, crtGamma));
    flickLut[i] = (uint8_t)i;
  }
  for (int py = 0; py < LCD_HEIGHT; py++) {
    const int q = ROTATE_CW ? py : (LCD_HEIGHT - 1 - py);
    colLx[py] = (uint8_t)(q / CRT_SCALE);
    colP[py] = (uint8_t)(q % CRT_SCALE);
    const float d = (py - LCD_HEIGHT / 2.0f) / (LCD_HEIGHT / 2.0f);
    const float e = fabsf(d) > 0.6f ? (fabsf(d) - 0.6f) / 0.4f : 0.0f;   // jen krajnich 40 %
    int l = (int)lroundf((1.0f - crtVignette * e * e) * 8.0f) - 1;
    vigRowL[py] = (uint8_t)(l < 0 ? 0 : (l > 7 ? 7 : l));
  }
  for (int px = 0; px < LCD_WIDTH; px++) {
    const int q = ROTATE_CW ? (LCD_WIDTH - 1 - px) : px;
    rowLy[px] = (uint8_t)(q / CRT_SCALE);
    rowK[px] = (uint8_t)(q % CRT_SCALE);
    preOff[px] = (uint16_t)(rowK[px] * LH * 3 + rowLy[px] * 3);
    const float d = (px - LCD_WIDTH / 2.0f) / (LCD_WIDTH / 2.0f);
    const float e = fabsf(d) > 0.6f ? (fabsf(d) - 0.6f) / 0.4f : 0.0f;   // jen krajnich 40 %
    int l = (int)lroundf((1.0f - crtVignette * e * e) * 8.0f) - 1;
    vigL[px] = vigBase[px] = (uint8_t)(l < 0 ? 0 : (l > 7 ? 7 : l));
  }
#if !CRT_SOFT
  for (int r = 0; r < LH; r++) {
    rPx0[r] = (int16_t)(ROTATE_CW ? (LCD_WIDTH - 1 - r * CRT_SCALE) : r * CRT_SCALE);
    const int rest = LCD_WIDTH - r * CRT_SCALE;
    rCnt[r] = (uint8_t)(rest < CRT_SCALE ? rest : CRT_SCALE);
  }
#endif
  for (int v = 0; v < 256; v++) {
    topLut[v] = (uint8_t)((v * crtRowTop) >> 8);
    botLut[v] = (uint8_t)((v * crtRowBot) >> 8);
    bleedLut[v] = (uint8_t)((v * crtRowBleed) >> 8);
  }
  for (int lvl = 0; lvl < 8; lvl++) {
    const float vig = (lvl + 1) / 8.0f;
    for (int p = 0; p < 3; p++)
      for (int ch = 0; ch < 3; ch++) {
        const float mask = (p == ch ? 1.0f : (256 - crtMask) / 256.0f) * crtMaskGain;
        for (int v = 0; v < 256; v++) {
          int o = (int)(v * mask * vig + 0.5f);
          lut16[lvl][p][ch][v] = part565swapped(ch, o > 255 ? 255 : o);
        }
      }
  }
  softBuild();
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

// predpocet jednoho logickeho sloupce: barvy z palety (s blikanim) a tri
// varianty radku podle faze sloupce k (scanline + prosvit sousedniho radku)
// pres tabulky topLut/botLut/bleedLut -> dst[k][r][ch]
static void computeCol(int c, uint8_t dst[3][LH][3]) {
  static uint8_t rgb[LH][3];
  const uint8_t *col = fb + c;
  for (int r = 0; r < LH; r++, col += LW) {
    const int i = *col;
    rgb[r][0] = flickLut[palR[i]]; rgb[r][1] = flickLut[palG[i]]; rgb[r][2] = flickLut[palB[i]];
  }
  for (int r = 0; r < LH; r++) {
    const uint8_t *b0 = rgb[r];
    const uint8_t *bp = r > 0 ? rgb[r - 1] : b0;
    const uint8_t *bn = r < LH - 1 ? rgb[r + 1] : b0;
    for (int ch = 0; ch < 3; ch++) {
      dst[1][r][ch] = b0[ch];
      int t = topLut[b0[ch]] + bleedLut[bp[ch]];
      dst[0][r][ch] = (uint8_t)(t > 255 ? 255 : t);
      t = botLut[b0[ch]] + bleedLut[bn[ch]];
      dst[2][r][ch] = (uint8_t)(t > 255 ? 255 : t);
    }
  }
}

// posuvne okno tri logickych sloupcu (predchozi, aktualni, dalsi)
static uint8_t colBuf[3][3][LH][3];
static uint8_t (*colPrev)[LH][3], (*colCur)[LH][3], (*colNext)[LH][3];
static int curCol = -1;

static void seekCol(int c) {
  if (c == curCol) return;
  if (c == curCol + 1) {
    uint8_t (*t)[LH][3] = colPrev;
    colPrev = colCur; colCur = colNext; colNext = t;
    if (c + 1 < LW) computeCol(c + 1, colNext); else memcpy(colNext, colCur, sizeof(colBuf[0]));
  } else {
    colPrev = colBuf[0]; colCur = colBuf[1]; colNext = colBuf[2];
    computeCol(c, colCur);
    if (c > 0) computeCol(c - 1, colPrev); else memcpy(colPrev, colCur, sizeof(colBuf[0]));
    if (c + 1 < LW) computeCol(c + 1, colNext); else memcpy(colNext, colCur, sizeof(colBuf[0]));
  }
  curCol = c;
}

// jeden fyzicky pruh: pro kazdy fyzicky radek jeden logicky sloupec;
// krajni faze (p 0/2) = 3 dily vlastni sloupec + 1 dil soused (stopa
// paprsku); pixel = OR tri prispevku z tabulky (maska, vinetace, RGB565)
static void composeStripe(uint16_t *buf, int y0) {
  for (int py = y0; py < y0 + STRIPE_H; py++) {
    const int c = colLx[py], p = colP[py];
    const uint32_t tb = micros();
    seekCol(c);
    usBlend += micros() - tb;
    const int vr = vigRowL[py];
    const uint16_t *bases[8];
    for (int lvl = 0; lvl < 8; lvl++) bases[lvl] = &lut16[lvl][p][0][0];
    const uint8_t *cur = &colCur[0][0][0];
    const uint8_t *nb = p == 0 ? &colPrev[0][0][0] : &colNext[0][0][0];
    const uint16_t *off = preOff;
    const uint8_t *vl = vigL;
    uint16_t *out = buf + (py - y0) * LCD_WIDTH;
    if (p == 1) {
      for (int px = 0; px < LCD_WIDTH; px++) {
        const uint8_t *v = cur + off[px];
        const uint16_t *l = bases[(vl[px] + vr) >> 1];
        out[px] = l[v[0]] | l[256 + v[1]] | l[512 + v[2]];
      }
    } else {
      for (int px = 0; px < LCD_WIDTH; px++) {
        const uint8_t *v = cur + off[px], *w = nb + off[px];
        const uint16_t *l = bases[(vl[px] + vr) >> 1];
        out[px] = l[(3 * v[0] + w[0]) >> 2] | l[256 + ((3 * v[1] + w[1]) >> 2)] | l[512 + ((3 * v[2] + w[2]) >> 2)];
      }
    }
  }
}

#if !CRT_SOFT
// rychly rezim: tabulka celeho pixelu; blikani se zapocita do tabulky (prepocet kazdy snimek)
static void fastBuild(int flick) {
  const int kw[3] = { crtRowTop, 256, crtRowBot };
  for (int lvl = 0; lvl < 8; lvl++)
    for (int p = 0; p < 3; p++)
      for (int k = 0; k < 3; k++) {
        const int base = (kw[k] * flick) >> 8;
        int m[3];
        for (int ch = 0; ch < 3; ch++)
          m[ch] = (int)((p == ch ? 256 : 256 - crtMask) * crtMaskGain * (lvl + 1) / 8.0f * base / 256.0f);
        uint16_t *t = fastLut[lvl][p][k];
        for (int i = 0; i < 256; i++) {
          int r = (palR[i] * m[0]) >> 8, g = (palG[i] * m[1]) >> 8, b = (palB[i] * m[2]) >> 8;
          if (r > 255) r = 255;
          if (g > 255) g = 255;
          if (b > 255) b = 255;
          const uint16_t c = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
          t[i] = (c >> 8) | (c << 8);
        }
      }
}

// jeden fyzicky pruh v rychlem rezimu: pixel = jedno cteni tabulky
static void composeStripeFast(uint16_t *buf, int y0) {
  for (int py = y0; py < y0 + STRIPE_H; py++) {
    const int c = colLx[py], p = colP[py];
    if (c != fastCol) {
      const uint8_t *col = fb + c;
      for (int r = 0; r < LH; r++, col += LW) colIdxBuf[r] = *col;
      fastCol = c;
    }
    const int vr = vigRowL[py];
    const uint16_t *bases[8];
    for (int lvl = 0; lvl < 8; lvl++) bases[lvl] = &fastLut[lvl][p][0][0];
    uint16_t *out = buf + (py - y0) * LCD_WIDTH;
    // po trojicich: jeden logicky radek r = tri fyzicke sloupce (k = 0, 1, 2)
    const int dir = ROTATE_CW ? -1 : 1;
    for (int r = 0; r < LH; r++) {
      const int idx = colIdxBuf[r];
      int px = rPx0[r];
      for (int k = 0; k < rCnt[r]; k++, px += dir)
        out[px] = bases[(vigL[px] + vr) >> 1][k * 256 + idx];
    }
  }
}
#endif

// ---- rezim LINKY: mekke scanlines (prosvit sousednich radku) a lehke vodorovne
// rozmazani pres tabulky dvojic barev; jeden pixel = jedno cteni tabulky, sousedni
// indexy jsou v posuvnem okne, takze to nestoji vic nez rezim TECKY ----
#define PAIR_N 64
static_assert(C_COUNT <= PAIR_N, "paleta musi mit nejvys 64 barev (tabulky dvojic)");
static uint16_t pairRow[3][2][PAIR_N][PAIR_N];   // [faze p = kanal masky][0 horni okraj bodu s radkem r-1, 1 dolni s r+1][barva][soused]
static uint16_t pairCol[2][PAIR_N][PAIR_N];      // stredni radek bodu: [0 se sloupcem c-1 (p=0), 1 s c+1 (p=2)][barva][soused]
static uint16_t lutMid[PAIR_N];                  // stred bodu bez rozmazani (p=1)
static uint8_t colIdxPrev[LH], colIdxNext[LH];
static int softCol = -1;

static inline uint16_t pack565s(int r, int g, int b) {
  if (r > 255) r = 255;
  if (g > 255) g = 255;
  if (b > 255) b = 255;
  const uint16_t c = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
  return (c >> 8) | (c << 8);
}

// RGB maska i v rezimu LINKY: faze p (fyzicky radek v bodu) nese jeden kanal plne, ostatni utlumene
static inline int maskW(int p, int ch) { return (int)((p == ch ? 256 : 256 - crtMask) * crtMaskGain); }

static void softBuild() {
  const int kw[2] = { crtRowTop, crtRowBot };
  for (int a = 0; a < PAIR_N; a++) {
    const int ia = a < C_COUNT ? a : 0;
    const int ca[3] = { palR[ia], palG[ia], palB[ia] };
    lutMid[a] = pack565s((ca[0] * maskW(1, 0)) >> 8, (ca[1] * maskW(1, 1)) >> 8, (ca[2] * maskW(1, 2)) >> 8);
    for (int b = 0; b < PAIR_N; b++) {
      const int ib = b < C_COUNT ? b : 0;
      const int cb[3] = { palR[ib], palG[ib], palB[ib] };
      for (int p = 0; p < 3; p++)
        for (int e = 0; e < 2; e++) {
          int o[3];
          for (int ch = 0; ch < 3; ch++) o[ch] = (((ca[ch] * kw[e] + cb[ch] * crtRowBleed) >> 8) * maskW(p, ch)) >> 8;
          pairRow[p][e][a][b] = pack565s(o[0], o[1], o[2]);
        }
      for (int q = 0; q < 2; q++) {
        const int p = q == 0 ? 0 : 2;
        int o[3];
        for (int ch = 0; ch < 3; ch++) o[ch] = (((ca[ch] * (256 - crtBlend) + cb[ch] * crtBlend) >> 8) * maskW(p, ch)) >> 8;
        pairCol[q][a][b] = pack565s(o[0], o[1], o[2]);
      }
    }
  }
}

static void composeStripeSoft(uint16_t *buf, int y0) {
  for (int py = y0; py < y0 + STRIPE_H; py++) {
    const int c = colLx[py], p = colP[py];
    if (c != softCol) {
      const uint8_t *col = fb + c, *cp = fb + (c > 0 ? c - 1 : c), *cn = fb + (c + 1 < LW ? c + 1 : c);
      for (int r = 0; r < LH; r++, col += LW, cp += LW, cn += LW) { colIdxBuf[r] = *col; colIdxPrev[r] = *cp; colIdxNext[r] = *cn; }
      softCol = c;
    }
    const uint8_t *nb = p == 0 ? colIdxPrev : (p == 2 ? colIdxNext : nullptr);
    const uint16_t *pc = &pairCol[p == 0 ? 0 : 1][0][0];
    const uint16_t (*pr)[PAIR_N][PAIR_N] = pairRow[p];
    uint16_t *out = buf + (py - y0) * LCD_WIDTH;
    const int dir = ROTATE_CW ? -1 : 1;
    int prev = colIdxBuf[0], cur = prev;
    for (int r = 0; r < LH; r++) {
      const int next = r + 1 < LH ? colIdxBuf[r + 1] : cur;
      int px = rPx0[r];
      const int n = rCnt[r];
      out[px] = pr[0][cur][prev]; px += dir;
      if (n > 1) { out[px] = nb ? pc[cur * PAIR_N + nb[r]] : lutMid[cur]; px += dir; }
      if (n > 2) out[px] = pr[1][cur][next];
      prev = cur; cur = next;
    }
  }
}

// putujici pruh jasu (hum) po logickych radcich = fyzickych sloupcich, jiskry do canvasu
static void crtFrameEffects() {
  if (crtHum > 0) {
    crtHumPos += 0.9f;
    if (crtHumPos > LH + 30) crtHumPos = -30;
    for (int px = 0; px < LCD_WIDTH; px++) {
      const float d = fabsf(rowLy[px] - crtHumPos);
      const int dim = d < 16 ? (int)(crtHum * (1.0f - d / 16.0f) * 8 / 256) : 0;
      const int l = vigBase[px] - dim;
      vigL[px] = (uint8_t)(l < 0 ? 0 : l);
    }
  }
  for (int i = 0; i < crtNoise; i++) fb[random(LW * LH)] = (uint8_t)(1 + random(C_COUNT - 1));
}

static void crtPresent() {
  crtFrameEffects();
  const int flick = 256 - random(crtFlicker + 1);
  for (int i = 0; i < 256; i++) flickLut[i] = (uint8_t)((i * flick) >> 8);
  curCol = -1;   // novy snimek: okno sloupcu se prepocita
  softCol = -1;
#if !CRT_SOFT
  if (crtMode == 0) fastBuild(flick);
  fastCol = -1;
#endif

  int stripeIdx = 0;
  for (int y0 = 0; y0 < LCD_HEIGHT; y0 += STRIPE_H, stripeIdx++) {
    const int buf = stripeIdx & 1;
    const uint32_t t0 = micros();
    waitPending(1);
    const uint32_t t1 = micros();
#if CRT_SOFT
    composeStripe(stripes[buf], y0);
#else
    if (crtMode == 1) composeStripeSoft(stripes[buf], y0);
    else composeStripeFast(stripes[buf], y0);
#endif
    usWait += t1 - t0;
    usCompose += micros() - t1;
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
