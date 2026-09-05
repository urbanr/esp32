#pragma once

#include <string.h>
#include "driver/spi_master.h"
#include "soc/gpio_reg.h"
#include "pin_config.h"
#include "../common/amoled_app.h"
#include "config.h"
#include "star_input.h"
#include "star_field.h"

// ===================================================================
// Render: projekce hvezd kamerou v oku (pohled po +Z do displeje),
// jas podle vzdalenosti od oka, kresleni po vodorovnych pruzich.
// Pruhy se posilaji vlastnim SPI zarizenim pres DMA (fronta
// transakci, 2 ping-pong buffery): zatimco se jeden pruh prenasi,
// CPU sklada dalsi. Kazdy pruh je samostatny zapis (CASET, PASET,
// RAMWR, data) - stejna sekvence, jakou posila Arduino_GFX.
// SPI sbernici inicializuje sketch (hwInit() v common/amoled_hw.h,
// max. transakce AMOLED_SPI_MAX_TRANSFER), renderInit() na ni jednou
// prida vlastni zarizeni. Od te chvile se pres gfx smi kreslit az
// po waitPending(0) (starEnd()): fronta prazdna, CS HIGH - jinak by
// callback dobihajiciho pruhu prepnul CS pod zapisem knihovny.
// Cely displej se prekresli kazdy snimek, displej nevidi mezistav.
// ===================================================================

static_assert(LCD_HEIGHT % STRIPE_H == 0, "STRIPE_H musi delit LCD_HEIGHT");
#define STRIPE_BYTES (LCD_WIDTH * STRIPE_H * 2)
static_assert(STRIPE_BYTES * 8 < (1 << 18), "pruh musi byt < 32 KB (limit jedne SPI transakce)");

#define LCD_CX (LCD_WIDTH / 2)
#define LCD_CY (LCD_HEIGHT / 2)

struct Dot {
  int16_t x, y;
  uint16_t c;      // RGB565 s prohozenymi bajty (poradi na sbernici)
};

static Dot dots[STAR_COUNT];
static int dotCount = 0;

// DMA buffery pruhu (interni RAM, zarovnane)
static uint16_t stripes[2][LCD_WIDTH * STRIPE_H] __attribute__((aligned(16)));
static spi_device_handle_t lcdDev = nullptr;
static spi_transaction_t colorTrans[2];
static spi_transaction_t cmdTrans[3];
static int pending = 0;   // transakce ve fronte, jeste nevyzvednute

static inline uint16_t rgb565swapped(uint8_t r, uint8_t g, uint8_t b) {
  const uint16_t c = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
  return (c >> 8) | (c << 8);
}

// SPI bus displeje inicializuje sketch (hwInit()), pruh se do jeho
// max. transakce musi vejit
static_assert(STRIPE_BYTES <= AMOLED_SPI_MAX_TRANSFER, "pruh musi byt <= AMOLED_SPI_MAX_TRANSFER (common/amoled_app.h)");

// CS ridime jako GPIO z callbacku ovladace (pred/po kazde transakci) -
// stejne jako Arduino_GFX; HW CS mel prilis tesne casovani a panel
// zapisy zahazoval
static void IRAM_ATTR csLow(spi_transaction_t *) { REG_WRITE(GPIO_OUT_W1TC_REG, 1u << LCD_CS); }
static void IRAM_ATTR csHigh(spi_transaction_t *) { REG_WRITE(GPIO_OUT_W1TS_REG, 1u << LCD_CS); }

// vlastni zarizeni pro pixely - volat az po inicializaci panelu
static bool renderInit() {
  // zarizeni se prida jednou a zustava i mezi prepnutimi aplikaci
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

// projekce vsech hvezd do seznamu bodu na displeji
static void projectStars(Vec3 U, Vec3 V, Vec3 W) {
  dotCount = 0;
  for (int i = 0; i < STAR_COUNT; i++) {
    const Star &s = stars[i];
    const float z = s.u * U.z + s.v * V.z + s.w * W.z;   // hloubka v ose pohledu
    if (z < NEAR_CLIP) continue;
    const float k = EYE_DIST_PX / z;
    const float x = (s.u * U.x + s.v * V.x + s.w * W.x) * k + LCD_CX;
    const float y = (s.u * U.y + s.v * V.y + s.w * W.y) * k + LCD_CY;
    if (x < 0 || x >= LCD_WIDTH || y < 0 || y >= LCD_HEIGHT) continue;
    const float d = sqrtf(s.u * s.u + s.v * s.v + s.w * s.w);
    float br = 1.0f - d / FIELD_RADIUS;
    if (br <= 0) continue;
    br = powf(br, BRIGHT_GAMMA);
    if (br < STAR_MIN_BRIGHT) continue;   // prilis tmave hvezdy se nekresli
    Dot &o = dots[dotCount++];
    o.x = (int16_t)x;
    o.y = (int16_t)y;
    o.c = rgb565swapped((uint8_t)(s.r * br), (uint8_t)(s.g * br), (uint8_t)(s.b * br));
  }
}

static inline void plot(uint16_t *buf, int x, int y, uint16_t c) {
  if (x >= 0 && x < LCD_WIDTH && y >= 0 && y < STRIPE_H) buf[y * LCD_WIDTH + x] = c;
}

// hvezda = ctverec STAR_SIZE_PX x STAR_SIZE_PX s levym hornim rohem v (x, y)
static void composeStripe(uint16_t *buf, int y0) {
  memset(buf, 0, STRIPE_BYTES);
  for (int i = 0; i < dotCount; i++) {
    const Dot &d = dots[i];
    const int y = d.y - y0;
    if (y <= -STAR_SIZE_PX || y >= STRIPE_H) continue;
    for (int dy = 0; dy < STAR_SIZE_PX; dy++)
      for (int dx = 0; dx < STAR_SIZE_PX; dx++)
        plot(buf, d.x + dx, y + dy, d.c);
  }
}

static void renderFrame() {
  int stripeIdx = 0;
  for (int y0 = 0; y0 < LCD_HEIGHT; y0 += STRIPE_H, stripeIdx++) {
    const int buf = stripeIdx & 1;
    // ve fronte smi zustat jen data predchoziho pruhu -> buffer z
    // predminuleho pruhu i prikazove transakce jsou volne
    waitPending(1);
    composeStripe(stripes[buf], y0);
    queueCmd(0, 0x2A, 0, LCD_WIDTH - 1, true);          // CASET
    queueCmd(1, 0x2B, y0, y0 + STRIPE_H - 1, true);     // PASET
    queueCmd(2, 0x2C, 0, 0, false);                     // RAMWR
    queueStripe(buf);
  }
  // posledni pruh dobiha na pozadi behem simulace dalsiho snimku
}
