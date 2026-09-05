#pragma once

#include <string.h>
#include "driver/spi_master.h"
#include "soc/gpio_reg.h"
#include "Arduino_GFX_Library.h"
#include "pin_config.h"
#include "../common/amoled_app.h"
#include "config.h"

// ===================================================================
// "Stara zelena obrazovka": scena se kresli do canvasu LR_W x LR_H
// (1 bod = CRT_SCALE x CRT_SCALE px displeje), barva = intenzita
// fosforu (zelena slozka RGB565, 0-63). Na displej se posila po pruzich
// 368 x STRIPE_H vlastnim SPI zarizenim pres DMA (stejny postup jako
// esp32-amoled-starfield/star_render.h). Pri skladani pruhu se na kazdy
// LR radek aplikuje filtr napodobujici CRT:
//   - dosvit fosforu (persistence): jas klesa postupne, pohyb necha stopu
//   - bloom / zare: rozmazani 5 bodu + pridavek kolem svetlych mist
//   - stopa paprsku: vodorovne prolnuti sousedu pri zvetseni
//   - scanlines: z trojice radku je stredni plny, okrajove tmavsi
//     a prosvicene sousednim radkem
//   - vinetace: okraje a rohy tmavsi
//   - blikani jasu snimku, plovouci svetlejsi pruh, nahodne jiskry
// SPI sbernici inicializuje sketch (hwInit()), zarizeni se prida jednou;
// pres gfx se smi kreslit az po crtEnd() (waitPending(0)).
// ===================================================================

static_assert(CRT_SCALE == 3, "filtr pocita s trojici radku a sloupcu na bod");
static_assert(LCD_HEIGHT % STRIPE_H == 0, "STRIPE_H musi delit LCD_HEIGHT");
#define STRIPE_BYTES (LCD_WIDTH * STRIPE_H * 2)
static_assert(STRIPE_BYTES <= AMOLED_SPI_MAX_TRANSFER, "pruh musi byt <= AMOLED_SPI_MAX_TRANSFER (common/amoled_app.h)");

// barva "inkoustu" pro kresleni do canvasu: intenzita 0-63
static inline uint16_t ink(int i) { return (uint16_t)(i << 5); }
#define INK_FULL   ink(63)
#define INK_BRIGHT ink(48)
#define INK_DIM    ink(26)
#define INK_FAINT  ink(12)
#define INK_OFF    ink(0)

static Arduino_Canvas *lr = nullptr;   // buffer nizkeho rozliseni

// DMA buffery pruhu (interni RAM, zarovnane)
static uint16_t stripes[2][LCD_WIDTH * STRIPE_H] __attribute__((aligned(16)));
static spi_device_handle_t lcdDev = nullptr;
static spi_transaction_t colorTrans[2];
static spi_transaction_t cmdTrans[3];
static int pending = 0;   // transakce ve fronte, jeste nevyzvednute

// dosvit fosforu: jas z predchozich snimku po bodech
static uint8_t glow[LR_W * LR_H];

// paleta fosforu pro 8 urovni vinetace: intenzita 0-63 -> RGB565
// s prohozenymi bajty (poradi na sbernici)
static uint16_t phosL[8][64];
static uint8_t vigX[LCD_WIDTH], vigY[LCD_HEIGHT];   // uroven vinetace 0-7
static uint8_t colIdx[LCD_WIDTH], colPhase[LCD_WIDTH]; // display sloupec -> LR bod (+2 okraj), faze 0-2

// tri LR radky (predchozi, aktualni, dalsi) po filtru, s okrajem 2 bodu
static uint8_t rowBuf[3][LR_W + 4];
static uint8_t *rowPrev, *rowCur, *rowNext;
static int rowCurIdx;
static int frameFlick = 256;   // jas snimku (0-256)
static float humPos = 0;       // pozice plovouciho pruhu (LR radky)

static inline uint16_t rgb565swapped(uint8_t r, uint8_t g, uint8_t b) {
  const uint16_t c = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
  return (c >> 8) | (c << 8);
}

static void buildTables() {
  for (int l = 0; l < 8; l++) {
    const float vig = (l + 1) / 8.0f;
    for (int i = 0; i < 64; i++) {
      const float g = 255.0f * powf(i / 63.0f * vig, CRT_GAMMA);
      phosL[l][i] = rgb565swapped((uint8_t)(g * CRT_TINT_R), (uint8_t)g, (uint8_t)(g * CRT_TINT_B));
    }
  }
  // vinetace: kvadraticky pokles od stredu, kvantovany na 8 urovni
  for (int x = 0; x < LCD_WIDTH; x++) {
    const float d = (x - LCD_WIDTH / 2.0f) / (LCD_WIDTH / 2.0f);
    const float f = 1.0f - CRT_VIGNETTE * d * d;
    int l = (int)lroundf(f * 8.0f) - 1;
    vigX[x] = (uint8_t)(l < 0 ? 0 : (l > 7 ? 7 : l));
    colIdx[x] = (uint8_t)(x / CRT_SCALE + 2);
    colPhase[x] = (uint8_t)(x % CRT_SCALE);
  }
  for (int y = 0; y < LCD_HEIGHT; y++) {
    const float d = (y - LCD_HEIGHT / 2.0f) / (LCD_HEIGHT / 2.0f);
    const float f = 1.0f - CRT_VIGNETTE * d * d;
    int l = (int)lroundf(f * 8.0f) - 1;
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

// canvas + tabulky + SPI zarizeni; false = malo RAM nebo chyba SPI
static bool crtInit(Arduino_GFX *gfx) {
  buildTables();
  memset(glow, 0, sizeof(glow));
  if (!lr) {
    lr = new Arduino_Canvas(LR_W, LR_H, gfx, 0, 0);
    if (!lr->begin(GFX_SKIP_OUTPUT_BEGIN)) return false;
  }
  return crtDeviceInit();
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
// do dst[2 .. LR_W+1] (okraje 0)
static void filterRow(int ly, uint8_t *dst) {
  memset(dst, 0, LR_W + 4);
  if (ly < 0 || ly >= LR_H) return;

  static uint8_t a[LR_W + 4];   // po dosvitu, s okrajem
  memset(a, 0, sizeof(a));
  const uint16_t *src = lr->getFramebuffer() + ly * LR_W;
  uint8_t *g = glow + ly * LR_W;
  for (int x = 0; x < LR_W; x++) {
    const int v = (src[x] >> 5) & 0x3F;
    int d = (g[x] * CRT_DECAY) >> 8;      // dosvit: jas klesa, novy obsah ho prebiji
    if (v > d) d = v;
    g[x] = (uint8_t)d;
    a[x + 2] = (uint8_t)d;
  }

  const bool hum = (ly >= (int)humPos && ly < (int)humPos + CRT_HUM_ROWS);
  for (int x = 0; x < LR_W; x++) {
    const uint8_t *p = a + x + 2;
    const int b = (p[-2] + 2 * p[-1] + 4 * p[0] + 2 * p[1] + p[2]) / 10;   // rozmazani
    int v = ((p[0] * (256 - CRT_BLOOM) + b * CRT_BLOOM) >> 8) + ((b * CRT_GLOW) >> 8);
    v = (v * frameFlick) >> 8;
    if (hum) v += CRT_HUM;
    dst[x + 2] = (uint8_t)(v > 63 ? 63 : v);
  }
  if (random(1000) < CRT_NOISE) {
    const int nx = random(LR_W) + 2;
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

// vodorovna stopa paprsku: kraje bodu prolnute se sousedem
static inline int beam(const uint8_t *row, int x) {
  const int i = colIdx[x];
  const int c = row[i];
  switch (colPhase[x]) {
    case 0:  return (2 * c + row[i - 1]) / 3;
    case 2:  return (2 * c + row[i + 1]) / 3;
    default: return c;
  }
}

// pruh displeje od radku y0
static void composeStripe(uint16_t *buf, int y0) {
  for (int y = y0; y < y0 + STRIPE_H; y++) {
    const int ly = y / CRT_SCALE, k = y % CRT_SCALE;
    seekRow(ly);
    uint16_t *out = buf + (y - y0) * LCD_WIDTH;
    const uint8_t vy = vigY[y];
    for (int x = 0; x < LCD_WIDTH; x++) {
      const int c = beam(rowCur, x);
      int v;
      if (k == 1) v = c;
      else if (k == 0) v = (c * CRT_ROW_TOP + beam(rowPrev, x) * CRT_ROW_BLEED) >> 8;
      else v = (c * CRT_ROW_BOT + beam(rowNext, x) * CRT_ROW_BLEED) >> 8;
      if (v > 63) v = 63;
      out[x] = phosL[(vigX[x] + vy) >> 1][v];
    }
  }
}

// cely buffer na displej (14 pruhu, posledni dobiha na pozadi)
static void crtPresent() {
  frameFlick = 256 - random(CRT_FLICKER + 1);
  humPos += CRT_HUM_SPEED;
  if (humPos > LR_H + CRT_HUM_ROWS) humPos = -CRT_HUM_ROWS * 4.0f;
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

// dokonceni DMA a uvolneni canvasu; po navratu se smi kreslit pres gfx
static void crtEnd() {
  waitPending(0);
  delete lr;
  lr = nullptr;
}
