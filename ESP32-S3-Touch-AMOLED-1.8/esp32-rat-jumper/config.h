#pragma once

// ===== obraz: displej na sirku (tlacitko dole), hruba mrizka =====
#define CRT_SCALE             3      // px displeje na bod (pevne; filtr pocita s 3)
#define ROTATE_CW             0      // 1: logicke x roste s fyzickym y displeje; 0: opacne (kdyz je obraz vzhuru nohama)
#define LW                    ((LCD_HEIGHT + CRT_SCALE - 1) / CRT_SCALE)   // sirka hry v bodech: 150
#define LH                    ((LCD_WIDTH + CRT_SCALE - 1) / CRT_SCALE)    // vyska hry v bodech: 123

// ===== svet (logicke body) =====
#define CEIL_H                9      // strop
#define FLOOR_Y               98     // horni hrana chodniku (nohy krysy)
#define FLOOR_H               6
#define WATER_Y               (FLOOR_Y + FLOOR_H)   // hladina vody pod chodnikem
#define LANE1_Y               74     // horni hrana policek 1. patra
#define LANE2_Y               50     // 2. patro
#define PLATFORM_H            6
#define RAT_X                 24     // leva hrana krysy
#define RAT_W                 20
#define RAT_H                 12

// ===== fyzika (body/s) =====
#define GRAVITY               330.0f
#define JUMP_V                128.0f // tuknuti
#define JUMP_HIGH_V           178.0f // BOOT
#define COYOTE_S              0.08f  // skok je mozny jeste chvilku po opusteni hrany
#define SPEED_START           44.0f  // rychlost sveta na zacatku
#define SPEED_MAX             96.0f
#define SPEED_GAIN            1.2f   // zrychleni za sekundu hry
#define SPAWN_GAP_MIN         24     // mezera mezi prekazkami (body, pri SPEED_START)
#define SPAWN_GAP_MAX         52

// ===== pravidla =====
#define LIVES                 3
#define INVULN_S              1.6f   // nezranitelnost po zasahu (blikani)

// ===== CRT filtr (barevny) =====
#define CRT_ROW_TOP           225    // jas horniho radku bodu 0-255 (scanline: stred 255)
#define CRT_ROW_BOT           80     // jas dolniho radku bodu
#define CRT_ROW_BLEED         110    // prosvit sousedniho radku do okrajovych radku 0-255
#define CRT_MASK              120    // sila RGB masky: o kolik (0-255) se utlumi cizi kanaly v prouzku
#define CRT_MASK_GAIN         1.25f  // kompenzace jasu masky
#define CRT_VIGNETTE          0.30f
#define CRT_FLICKER           10     // nahodne kolisani jasu snimku (0-255)

// ===== zvuk =====
#define AUDIO_VOLUME          48     // hlasitost kodeku 0-100
#define AUDIO_MASTER          0.7f

// ===== render =====
#define LCD_QSPI_HZ           40000000
#define STRIPE_H              32
#define DEBUG_PERIOD_MS       1000
