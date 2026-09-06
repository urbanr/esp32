#pragma once

// ===== sada spritu (vyber pred kompilaci) =====
#define SPRITES_KLASIK        0      // rucne kreslene ASCII sprity, prostredi procedurálne
#define SPRITES_ASTRA         1      // balicek kanal-komplet.zip (ChatGPT Astra) vcetne dlazdic
#ifndef SPRITE_SET
#define SPRITE_SET            SPRITES_ASTRA   // lze prebit z prikazove radky: --build-property compiler.cpp.extra_flags=-DSPRITE_SET=0
#endif

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
#define CUP_EVERY             10     // kazdy n-ty odpadek je pohar: stribrny, kazdy druhy z nich zlaty
#define CUP_SILVER_PTS        3
#define CUP_GOLD_PTS          10
#define CHEST_PTS             5      // bedynka otevrena klicem
#define KEY_CHANCE            8      // % odpadku, ktere jsou zlaty klic
#define BUBBLE_CHANCE         7      // % odpadku, ktere jsou mydlova bublina (stit na jeden naraz)
#define CHEST_CHANCE          35     // % prekazek, za nimiz stoji bedynka
#define SNAIL_SPEED           9.0f   // snek leze proti kryse (body/s navic k posunu sveta)
#define DRIP_WAIT_S           1.3f   // kapajici trubka: pauza, blikani kapky, pak pad
#define DRIP_BLINK_S          0.8f
#define DROP_SPEED            110.0f // body/s
#define JET_LIFT              150.0f // rychlost, kterou proud vody vynasi krysu (body/s)

// ===== parada (bez vlivu na hru) =====
#define BAT_GAP_MIN_S         6.0f   // pauza mezi prelety netopyra
#define BAT_GAP_MAX_S         16.0f
#define BAT_SPEED             70.0f  // rychlost netopyra proti smeru hry (navic k posunu sveta), body/s
#define WALL_DECO_STEP        110    // rozestup ozdob na zdi (kulate okno, ventil, mriz), body paralaxy

// ===== CRT filtr (barevny) =====
#define CRT_SOFT              0      // 1 = mekky rezim (prosvit radku, prolnuti sloupcu; ~16 fps), 0 = rychly (jen scanlines + maska + vineta)
#define CRT_ROW_TOP           225    // jas horniho radku bodu 0-255 (scanline: stred 255)
#define CRT_ROW_BOT           80     // jas dolniho radku bodu
#define CRT_ROW_BLEED         110    // prosvit sousedniho radku do okrajovych radku 0-255
#define CRT_MASK              120    // sila RGB masky: o kolik (0-255) se utlumi cizi kanaly v prouzku
#define CRT_MASK_GAIN         1.25f  // kompenzace jasu masky
#define CRT_VIGNETTE          0.0f   // 0 = bez vinetace
#define CRT_FLICKER           10     // nahodne kolisani jasu snimku (0-255)

// ===== zvuk =====
#define AUDIO_VOLUME          38     // hlasitost kodeku 0-100
#define AUDIO_MASTER          0.7f

// ===== render =====
#define LCD_QSPI_HZ           40000000
#define STRIPE_H              32
#define DEBUG_PERIOD_MS       1000
