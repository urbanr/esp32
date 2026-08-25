#pragma once

// ===== hvezdne pole =====
#define STAR_COUNT            2700    // celkovy pocet simulovanych hvezd (v zornem poli je jich cca 1/6)
#define FIELD_RADIUS          1500.0f // polomer koule hvezd kolem oka (jednotky = display px)
#define STAR_SPEED            900.0f  // rychlost letu hvezd (jednotky/s)
#define EYE_DIST_PX           200.0f  // vzdalenost oka od displeje (perspektiva; mensi = sirsi zaber)
#define NEAR_CLIP             2.0f    // hvezdy blize k oku (v ose pohledu) se nekresli

// ===== jas a barvy =====
#define BRIGHT_GAMMA          0.7f    // jas = (1 - d/R)^gamma; >1 = vic tmavych hvezd, hlubsi pole
#define STAR_TINT_PROB_PCT    60      // % hvezd s barevnym nadechem (zbytek ciste bile)
#define STAR_TINT_MAX         0.7f    // max. sila nadechu 0..1 (1 = cista barva z palety)
// paleta nadechu (RGB888) - hvezda se nahodne priklonu k jedne z barev
#define STAR_TINT_COLORS      { {255, 235, 170}, {255, 200, 110}, {255, 160, 80} }
#define STAR_SIZE_PX          3       // strana ctverce hvezdy v px (1 = jeden pixel, 2 = 2x2, ...)
#define STAR_MIN_BRIGHT       0.06f   // hvezdy s jasem (0..1) pod prahem se vubec nekresli

// ===== orientace (IMU) =====
#define GRAVITY_FILTER_ALPHA  0.05f   // vaha akcelerometru v komplementarnim filtru gravitace (za snimek)
#define GYRO_CALIB_SAMPLES    100     // vzorku v klidu pro zmereni biasu gyra (~4 s); mereni se
                                      // opakuje kdykoli zarizeni takto dlouho nehybne lezi
#define GYRO_STILL_DPS        15.0f   // |gyro| (dps, vc. biasu - ten byva i ~5 dps), pod kterym je zarizeni "v klidu"
#define ACCEL_STILL_G         0.03f   // zmena akcelerace mezi vzorky (g), pod kterou je zarizeni "v klidu"
                                      // (zmena, ne |a| - 1: |a| ma na tomto kusu offset ~0.91 g)
#define RECENTER_RATE         0.3f    // rychlost navratu smeru letu k "primo na me" (1/s); 0 = ciste gyro
#define FLAT_LIMIT            0.3f    // vodorovna slozka normaly displeje, pod kterou zarizeni "lezi":
                                      // zadny navrat smeru (jen gyro), pri startu smer "od horni hrany k dolni"

// Mapovani os senzoru QMI8658 na osy displeje: +X doprava, +Y dolu, +Z do displeje (od divaka).
// Akcelerometr -> smer gravitace (g); X/Y prevzato ze sand a bubble-level, Z dopocitano tak, aby
// slo o stejnou rotaci. Kontrola: zarizeni lezi displejem nahoru -> ACCEL_MAP_GZ musi byt ~ +1.
// Kdyby vychazelo -1, prohodit na: GZ = (az), GYRO_X = (-(gy)), GYRO_Y = (gx).
#define ACCEL_MAP_GX(ax, ay, az)  (-(ay))
#define ACCEL_MAP_GY(ax, ay, az)  (ax)
#define ACCEL_MAP_GZ(ax, ay, az)  (-(az))
// gyroskop -> uhlova rychlost kolem os displeje (dps), stejna rotace jako u akcelerometru
#define GYRO_MAP_X(gx, gy, gz)    (gy)
#define GYRO_MAP_Y(gx, gy, gz)    (-(gx))
#define GYRO_MAP_Z(gx, gy, gz)    (gz)

// ===== render =====
#define LCD_QSPI_HZ           40000000 // takt QSPI pro prenos pixelu (80 MHz = rychlejsi, overit obraz)
#define STRIPE_H              32      // vyska pruhu DMA bufferu (deli LCD_HEIGHT, pruh < 32 KB)
#define DISPLAY_BRIGHTNESS    200     // 0-255
#define DEBUG_PERIOD_MS       1000    // perioda diagnostiky na USBSerial; 0 = vypnuto
