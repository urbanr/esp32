#pragma once

// ===== let =====
#define FLIGHT_S              60.0f  // delka celeho letu (s)
#define PLANET_COUNT          4      // pocet cilovych planet
#define ROUTE_TOTAL_KM        1500000.0f // "uleteno" za cely let (km) - jen pro udaje
#define SPEED_MIN_KMS         2.0f   // rychlost na zacatku/konci useku (km/s)
#define SPEED_MAX_KMS         48.0f  // rychlost uprostred useku (km/s)
#define TELEMETRY_HZ          4      // jak casto "tikaji" cisla telemetrie
#define FLIGHT_BUTTON_PIN     0      // BOOT tlacitko - start / pauza / pokracovani / novy let

// ===== zasoby (% na konci letu; rychlost ubytku se losuje pri startu) =====
#define SUPPLY_END_MIN        10
#define SUPPLY_END_MAX        60
#define WASTE_END_MIN         40
#define WASTE_END_MAX         95
#define SUPPLY_WARN_PCT       20     // pod timto blika varovani (toaleta nad 100 - SUPPLY_WARN_PCT)

// ===== swipe =====
#define SWIPE_MIN_PX          50     // min. posun prstu pro tah (display px)
#define SWIPE_MAX_MS          800    // max. delka tahu
#define TAP_MOVE_PX           20     // max. posun prstu pri tuknuti
#define TAP_MAX_MS            300    // max. delka tuknuti

// ===== virtualni obrazovka: mrizka CRT_SCALE x CRT_SCALE display px na 1 bod =====
// vychozi hodnota; za behu se meni na obrazovce CRT (swipe nahoru/dolu, rozsah 2-6):
// 2 = 184x224 bodu (30 sloupcu textu), 3 = 123x150 (20 sloupcu), 4 = 92x112 (drobny font 3x5) ...
#define CRT_SCALE             4
#define SCREEN_CORNER_PX      60     // polomer zaobleni ramecku (display px) - displej ma kulate rohy

// ===== CRT filtr (viz ship_crt.h) =====
#define CRT_DECAY             182    // dosvit fosforu: podil jasu, ktery zustane do dalsiho snimku (0-255)
#define CRT_BLOOM             176    // podil rozmazane slozky v bodu (0-255) - mekke hrany
#define CRT_GLOW              119     // pridavek zare kolem svetlych mist (0-255 z rozmazane slozky)
#define CRT_ROW_TOP           148    // jas horniho radku bodu (0-255); u mrizky 2 se nepouziva
#define CRT_ROW_BOT           26     // jas dolniho radku bodu (0-255) = scanline; hlavni radek je vzdy 255
#define CRT_ROW_BLEED         160    // prosviceni sousedniho radku do okrajovych radku (0-255)
#define CRT_GRILLE            246    // jas posledniho sloupce bodu (0-255) = svisle prouzky masky
#define CRT_VIGNETTE          0.30f  // ztmaveni okraju a rohu (0 = zadne)
#define CRT_FLICKER           35     // nahodne kolisani jasu snimku (0-255)
#define CRT_HUM               8      // jas plovouciho pruhu ("hum bar", 0-63)
#define CRT_HUM_ROWS          16     // vyska pruhu (LR radky)
#define CRT_HUM_SPEED         0.35f  // rychlost pruhu (LR radky za snimek)
#define CRT_NOISE             18      // pravdepodobnost jiskry na LR radek (promile)
#define CRT_GAMMA             0.97f  // <1 = svetlejsi stredni tony
#define CRT_TINT_R            0.22f  // podil cervene a modre v barve fosforu (0 = cista zelena)
#define CRT_TINT_B            0.30f
#define BLINK_MS              500    // perioda blikani hlasek

// ===== zvuk (ES8311 + I2S, synteza v ship_audio.h) =====
#define AUDIO_RATE            16000  // vzorkovaci frekvence
#define AUDIO_VOLUME          70     // hlasitost kodeku 0-100
#define AUDIO_MASTER          0.6f   // celkove zeslabeni syntezy 0-1
#define AUDIO_ENGINE_GAIN     0.55f  // sila motoru (sum) za letu

// ===== render =====
#define LCD_QSPI_HZ           40000000
#define STRIPE_H              32     // vyska pruhu DMA bufferu (deli LCD_HEIGHT)
#define DEBUG_PERIOD_MS       0      // perioda vypisu fps na USBSerial; 0 = vypnuto
