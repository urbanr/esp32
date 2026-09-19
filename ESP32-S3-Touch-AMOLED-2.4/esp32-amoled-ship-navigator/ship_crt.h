#pragma once

#include <string.h>
#include <stdlib.h>
#include "driver/spi_master.h"
#include "soc/gpio_reg.h"
#include "Arduino_GFX_Library.h"
#include "pin_config.h"
#include "../common/amoled_app.h"
#include "config.h"

// ===================================================================
// "Stara zelena obrazovka": scena se kresli do canvasu lrW x lrH
// (1 bod = crt.scale x crt.scale px displeje, 2-6), barva = intenzita
// fosforu (zelena slozka RGB565, 0-63). Na displej se posila po pruzich
// 368 x STRIPE_H vlastnim SPI zarizenim pres DMA (stejny postup jako
// esp32-amoled-starfield/star_render.h). Pri skladani pruhu se na kazdy
// LR radek aplikuje filtr napodobujici CRT:
//   - dosvit fosforu (persistence): jas klesa postupne, pohyb necha stopu
//   - bloom / zare: rozmazani 5 bodu + pridavek kolem svetlych mist
//   - stopa paprsku: vodorovne prolnuti sousedu pri zvetseni
//   - scanlines: posledni radek bodu tmavsi (u 3+ px i prvni),
//     prosvicene sousednim LR radkem; posledni sloupec bodu tmavsi
//     = svisle prouzky jako maska (aperture grille)
//   - vinetace: okraje a rohy tmavsi
//   - blikani jasu snimku, plovouci svetlejsi pruh, nahodne jiskry
// Parametry jsou za behu menitelne (obrazovka CRT), vychozi z config.h.
// SPI sbernici inicializuje sketch (hwInit()), zarizeni se prida jednou;
// pres gfx se smi kreslit az po crtEnd() (waitPending(0)).
// ===================================================================

static_assert(LCD_HEIGHT % STRIPE_H == 0, "STRIPE_H musi delit LCD_HEIGHT");
#define STRIPE_BYTES (LCD_WIDTH * STRIPE_H * 2)
static_assert(STRIPE_BYTES <= AMOLED_SPI_MAX_TRANSFER, "pruh musi byt <= AMOLED_SPI_MAX_TRANSFER (common/amoled_app.h)");

#define CRT_SCALE_MIN 2
#define CRT_SCALE_MAX 6
#define LR_MAX_W ((LCD_WIDTH + CRT_SCALE_MIN - 1) / CRT_SCALE_MIN)   // 184

// parametry filtru (vychozi hodnoty = config.h)
struct CrtParams {
  int   scale;      // px displeje na bod
  int   decay;      // dosvit 0-255
  int   bloom;      // podil rozmazani 0-255
  int   glow;       // zare 0-255
  int   rowTop;     // jas horniho radku bodu 0-255 (scale >= 3)
  int   rowBot;     // jas dolniho radku bodu 0-255 (scanline)
  int   rowBleed;   // prosvit sousedniho radku 0-255
  int   grille;     // jas posledniho sloupce bodu 0-255 (svisle prouzky)
  float vignette;   // 0-0.6
  int   flicker;    // 0-255
  int   hum;        // jas pruhu 0-63
  int   noise;      // jiskry, promile na radek
  float gamma;
};
static CrtParams crt = { CRT_SCALE, CRT_DECAY, CRT_BLOOM, CRT_GLOW, CRT_ROW_TOP, CRT_ROW_BOT,
                         CRT_ROW_BLEED, CRT_GRILLE, CRT_VIGNETTE, CRT_FLICKER, CRT_HUM, CRT_NOISE, CRT_GAMMA };

// rozmery bufferu nizkeho rozliseni pro aktualni mrizku
static int lrW = 0, lrH = 0;
#define LR_W lrW
#define LR_H lrH

// barva "inkoustu" pro kresleni do canvasu: intenzita 0-63
static inline uint16_t ink(int i) { return (uint16_t)(i << 5); }
#define INK_FULL   ink(63)
#define INK_BRIGHT ink(48)
#define INK_DIM    ink(26)
#define INK_FAINT  ink(12)
#define INK_OFF    ink(0)

static Arduino_Canvas *lr = nullptr;   // buffer nizkeho rozliseni
static uint8_t *glow = nullptr;        // dosvit fosforu po bodech (lrW * lrH)

// DMA buffery pruhu (interni RAM, zarovnane)
static uint16_t stripes[2][LCD_WIDTH * STRIPE_H] __attribute__((aligned(16)));
static spi_device_handle_t lcdDev = nullptr;
static spi_transaction_t colorTrans[2];
static spi_transaction_t cmdTrans[3];
static int pending = 0;   // transakce ve fronte, jeste nevyzvednute

// paleta fosforu pro 8 urovni vinetace: intenzita 0-63 -> RGB565
// s prohozenymi bajty (poradi na sbernici)
static uint16_t phosL[8][64];
static uint8_t vigX[LCD_WIDTH], vigY[LCD_HEIGHT];      // uroven vinetace 0-7
static uint8_t colIdx[LCD_WIDTH], colPhase[LCD_WIDTH]; // display sloupec -> LR bod (+2 okraj), faze 0..scale-1
// vahy radku a sloupcu uvnitr bodu (0-256): vlastni jas, prosvit predchoziho / dalsiho LR radku
static int rowOwn[CRT_SCALE_MAX], rowPrevW[CRT_SCALE_MAX], rowNextW[CRT_SCALE_MAX], colW[CRT_SCALE_MAX];

// tri LR radky (predchozi, aktualni, dalsi) po filtru, s okrajem 2 bodu
static uint8_t rowBuf[3][LR_MAX_W + 4];
static uint8_t *rowPrev, *rowCur, *rowNext;
static int rowCurIdx;
static int frameFlick = 256;   // jas snimku (0-256)
static float humPos = 0;       // pozice plovouciho pruhu (LR radky)

static inline uint16_t rgb565swapped(uint8_t r, uint8_t g, uint8_t b) {
  const uint16_t c = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
  return (c >> 8) | (c << 8);
}

// prepocet tabulek z parametru crt (volat po kazde zmene)
static void crtBuildTables() {
  const int s = crt.scale;
  for (int k = 0; k < CRT_SCALE_MAX; k++) {
    rowOwn[k] = 256; rowPrevW[k] = 0; rowNextW[k] = 0; colW[k] = 256;
  }
  if (s >= 3) { rowOwn[0] = crt.rowTop; rowPrevW[0] = crt.rowBleed; }
  rowOwn[s - 1] = crt.rowBot;
  rowNextW[s - 1] = crt.rowBleed;
  colW[s - 1] = crt.grille;

  for (int l = 0; l < 8; l++) {
    const float vig = (l + 1) / 8.0f;
    for (int i = 0; i < 64; i++) {
      const float g = 255.0f * powf(i / 63.0f * vig, crt.gamma);
      phosL[l][i] = rgb565swapped((uint8_t)(g * CRT_TINT_R), (uint8_t)g, (uint8_t)(g * CRT_TINT_B));
    }
  }
  // vinetace: kvadraticky pokles od stredu, kvantovany na 8 urovni
  for (int x = 0; x < LCD_WIDTH; x++) {
    const float d = (x - LCD_WIDTH / 2.0f) / (LCD_WIDTH / 2.0f);
    int l = (int)lroundf((1.0f - crt.vignette * d * d) * 8.0f) - 1;
    vigX[x] = (uint8_t)(l < 0 ? 0 : (l > 7 ? 7 : l));
    colIdx[x] = (uint8_t)(x / s + 2);
    colPhase[x] = (uint8_t)(x % s);
  }
  for (int y = 0; y < LCD_HEIGHT; y++) {
    const float d = (y - LCD_HEIGHT / 2.0f) / (LCD_HEIGHT / 2.0f);
    int l = (int)lroundf((1.0f - crt.vignette * d * d) * 8.0f) - 1;
    vigY[y] = (uint8_t)(l < 0 ? 0 : (l > 7 ? 7 : l));
  }
}

// CS ridime jako GPIO z callbacku ovladace (pred/po kazde transakci)
static void IRAM_ATTR csLow(spi_transaction_t *) { REG_WRITE(GPIO_OUT_W1TC_REG, 1u << LCD_CS); }
static void IRAM_ATTR csHigh(spi_transaction_t *) { REG_WRITE(GPIO_OUT_W1TS_REG, 1u << LCD_CS); }

// vlastni SPI zarizeni pro pixely - prida se jednou a zustava
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

static void waitPending(int maxLeft);

// buffery pro danou mrizku (canvas + dosvit); false = malo RAM
static bool crtAllocBuffers(Arduino_GFX *gfx) {
  waitPending(0);
  delete lr;
  lr = nullptr;
  free(glow);
  glow = nullptr;
  lrW = (LCD_WIDTH + crt.scale - 1) / crt.scale;
  lrH = (LCD_HEIGHT + crt.scale - 1) / crt.scale;
  lr = new Arduino_Canvas(lrW, lrH, gfx, 0, 0);
  if (!lr->begin(GFX_SKIP_OUTPUT_BEGIN)) return false;
  lr->setTextWrap(false);
  glow = (uint8_t *)calloc(lrW * lrH, 1);
  return glow != nullptr;
}

static Arduino_GFX *crtGfx = nullptr;

// canvas + tabulky + SPI zarizeni; false = malo RAM nebo chyba SPI
static bool crtInit(Arduino_GFX *gfx) {
  crtGfx = gfx;
  crtBuildTables();
  if (!crtDeviceInit()) return false;
  return crtAllocBuffers(gfx);
}

// zmena mrizky za behu (2-6); pri nedostatku RAM se vrati k predchozi
static bool crtSetScale(int s) {
  if (s < CRT_SCALE_MIN) s = CRT_SCALE_MIN;
  if (s > CRT_SCALE_MAX) s = CRT_SCALE_MAX;
  const int old = crt.scale;
  crt.scale = s;
  if (!crtAllocBuffers(crtGfx)) {
    crt.scale = old;
    crtAllocBuffers(crtGfx);
    crtBuildTables();
    return false;
  }
  crtBuildTables();
  return true;
}

static void queueTrans(spi_transaction_t *t) {
  ESP_ERROR_CHECK(spi_device_queue_trans(lcdDev, t, portMAX_DELAY));
  pending++;
}

// pocka, az ve fronte zbyva nejvyse maxLeft transakci
static void waitPending(int maxLeft) {
  spi_transaction_t *r;
  while (pending > maxLeft) {
    spi_device_get_trans_result(lcdDev, &r, portMAX_DELAY);
    pending--;
  }
}

// prikaz panelu: opcode 0x02, adresa = registr << 8, data jednolinkove
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

// data pruhu po RAMWR: opcode 0x32, adresa 0x003C00, pixely po 4 linkach
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

// jeden LR radek z canvasu pres dosvit, bloom, blikani, pruh a jiskry
// do dst[2 .. lrW+1] (okraje 0)
static void filterRow(int ly, uint8_t *dst) {
  memset(dst, 0, lrW + 4);
  if (ly < 0 || ly >= lrH) return;

  static uint8_t a[LR_MAX_W + 4];   // po dosvitu, s okrajem
  memset(a, 0, lrW + 4);
  const uint16_t *src = lr->getFramebuffer() + ly * lrW;
  uint8_t *g = glow + ly * lrW;
  for (int x = 0; x < lrW; x++) {
    const int v = (src[x] >> 5) & 0x3F;
    int d = (g[x] * crt.decay) >> 8;      // dosvit: jas klesa, novy obsah ho prebiji
    if (v > d) d = v;
    g[x] = (uint8_t)d;
    a[x + 2] = (uint8_t)d;
  }

  const bool hum = (ly >= (int)humPos && ly < (int)humPos + CRT_HUM_ROWS);
  for (int x = 0; x < lrW; x++) {
    const uint8_t *p = a + x + 2;
    const int b = (p[-2] + 2 * p[-1] + 4 * p[0] + 2 * p[1] + p[2]) / 10;   // rozmazani
    int v = ((p[0] * (256 - crt.bloom) + b * crt.bloom) >> 8) + ((b * crt.glow) >> 8);
    v = (v * frameFlick) >> 8;
    if (hum) v += crt.hum;
    dst[x + 2] = (uint8_t)(v > 63 ? 63 : v);
  }
  if (random(1000) < crt.noise) {
    const int nx = random(lrW) + 2;
    const int v = dst[nx] + 20;
    dst[nx] = (uint8_t)(v > 63 ? 63 : v);
  }
}

// posun okna tri radku tak, aby rowCur byl LR radek ly
static void seekRow(int ly) {
  if (ly == 0 && rowCurIdx != 0) {
    // zacatek snimku
    rowPrev = rowBuf[0]; rowCur = rowBuf[1]; rowNext = rowBuf[2];
    filterRow(-1, rowPrev);
    filterRow(0, rowCur);
    filterRow(1, rowNext);
    rowCurIdx = 0;
  }
  while (rowCurIdx < ly) {
    uint8_t *t = rowPrev;
    rowPrev = rowCur;
    rowCur = rowNext;
    rowNext = t;
    rowCurIdx++;
    filterRow(rowCurIdx + 1, rowNext);
  }
}

// vodorovna stopa paprsku: krajni sloupce bodu prolnute se sousedem
static inline int beam(const uint8_t *row, int x) {
  const int i = colIdx[x];
  const int c = row[i];
  const int p = colPhase[x];
  if (p == 0) return (3 * c + row[i - 1]) >> 2;
  if (p == crt.scale - 1) return (3 * c + row[i + 1]) >> 2;
  return c;
}

// pruh displeje od radku y0
static void composeStripe(uint16_t *buf, int y0) {
  for (int y = y0; y < y0 + STRIPE_H; y++) {
    const int ly = y / crt.scale, k = y % crt.scale;
    seekRow(ly);
    uint16_t *out = buf + (y - y0) * LCD_WIDTH;
    const uint8_t vy = vigY[y];
    const int own = rowOwn[k], wp = rowPrevW[k], wn = rowNextW[k];
    for (int x = 0; x < LCD_WIDTH; x++) {
      int v = beam(rowCur, x) * own;
      if (wp) v += beam(rowPrev, x) * wp;
      if (wn) v += beam(rowNext, x) * wn;
      v = (v >> 8) * colW[colPhase[x]] >> 8;
      if (v > 63) v = 63;
      out[x] = phosL[(vigX[x] + vy) >> 1][v];
    }
  }
}

// cely buffer na displej (14 pruhu, posledni dobiha na pozadi)
static void crtPresent() {
  frameFlick = 256 - random(crt.flicker + 1);
  humPos += CRT_HUM_SPEED;
  if (humPos > lrH + CRT_HUM_ROWS) humPos = -CRT_HUM_ROWS * 4.0f;
  rowCurIdx = -1;   // vynuti restart okna radku v seekRow(0)

  int stripeIdx = 0;
  for (int y0 = 0; y0 < LCD_HEIGHT; y0 += STRIPE_H, stripeIdx++) {
    const int buf = stripeIdx & 1;
    waitPending(1);
    composeStripe(stripes[buf], y0);
    queueCmd(0, 0x2A, 0, LCD_WIDTH - 1, true);          // CASET
    queueCmd(1, 0x2B, y0, y0 + STRIPE_H - 1, true);     // PASET
    queueCmd(2, 0x2C, 0, 0, false);                     // RAMWR
    queueStripe(buf);
  }
}

// dokonceni DMA a uvolneni bufferu; po navratu se smi kreslit pres gfx
static void crtEnd() {
  waitPending(0);
  delete lr;
  lr = nullptr;
  free(glow);
  glow = nullptr;
}
