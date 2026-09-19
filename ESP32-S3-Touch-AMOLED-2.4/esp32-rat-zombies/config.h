#pragma once

// ===== obraz: displej na sirku (tlacitko dole), stejna mrizka jako krysa skokan =====
#define CRT_SCALE             3
#define ROTATE_CW             0
#define LW                    ((LCD_HEIGHT + CRT_SCALE - 1) / CRT_SCALE)   // 150
#define LH                    ((LCD_WIDTH + CRT_SCALE - 1) / CRT_SCALE)    // 123

// ===== svet (logicke body) =====
#define GROUND_BASE           94     // stredni vyska terenu (y), teren se vlni kolem ni
#define TERRAIN_AMP           6.0f   // amplituda zvlneni v levelu 1 (body), + TERRAIN_AMP_STEP za level
#define TERRAIN_AMP_STEP      3.0f
#define BAND_Y                82     // horni hrana tmaveho pasu pod domy
#define SEGMENT_LEN           1800   // vzdalenost mezi garazemi (body drahy) v levelu 1
#define SEGMENT_LEN_STEP      200    // + za kazdy dalsi level
#define GARAGES_PER_LEVEL     5      // garazi na level; po levelu dalsi krysa
#define FLAT_ZONE             70     // rovina u garazi (body)
#define RAT_SCREEN_X          30     // x kotvy krysy na obrazovce
#define WHEEL_BASE            28     // rozvor kol

// ===== krysa: vlastnosti (uroven 1-10) =====
#define V_MAX_BASE            50.0f  // max rychlost pri motoru 1 (body/s)
#define V_MAX_STEP            6.0f   // + za uroven motoru
#define ACCEL_BASE            25.0f  // zrychleni (body/s^2)
#define ACCEL_STEP            3.0f
#define DRAG                  14.0f  // zpomaleni bez plynu
#define TANK_BASE             1500.0f // dojezd (body drahy) pri nadrzi 1 a kolech 1
#define TANK_STEP             250.0f
#define WHEEL_SAVE            0.06f  // uspora spotreby za uroven kol
#define MUD_SLOW              0.20f  // bahno: snizeni max rychlosti pri kolech 1 (linearne k 0 pri kolech 10)
#define SLOPE_FUEL            1.5f   // spotreba do kopce: x(1 + SLOPE_FUEL * sklon)
#define FUEL_MAX              1.2f   // nadrz muze pretect na 120 %
#define GRAVITY               300.0f
#define JUMP_V                150.0f // vyska skoku ~37 bodu (kontejner ma 24)
#define MIN_SPEED_HIT         8.0f   // pod tohle zombik nezpomali
#define COST_PER_LEVEL        10     // cena vylepseni z urovne n na n+1 = COST_PER_LEVEL * n
#define RAT_BASE_LEVEL        { 1, 3, 5 }   // vychozi uroven vlastnosti: chlupata, drevena, ocelova
#define RAT_SPEED_MULT        { 1.0f, 1.15f, 1.3f }   // dalsi krysy jsou celkove rychlejsi
#define RAT_SLOW_MULT         { 1.0f, 0.7f, 0.45f }   // ... a zombici je mene zdrzi

// ===== trat: pocty na usek (level 1), zmeny za level =====
#define COINS_PER_SEG         40
#define COINS_STEP            4
#define MATH_PCT_SEG          40     // % sance, ze se v useku objevi jedno znamenko (plus / krat nahodne)
#define FUEL_CANS_PER_SEG     2
#define BALLS_PER_SEG         1
#define ZOMB_THIN_PER_SEG     8
#define ZOMB_BLOCK_PER_SEG    3
#define ZOMB_ROUND_PER_SEG    1
#define ZOMB_STEP             2      // + hubeni za level; hranati a kulati +1
#define OBST_PER_SEG          4      // klada, potrubi, kontejner, rampa
#define ZOMB_SLOW             { 10.0f, 18.0f, 28.0f }   // zpomaleni podle siluety
#define ZOMB_WALK_MIN         4.0f   // rychlost vravorani proti kryse (body/s)
#define ZOMB_WALK_MAX         8.0f
#define FUEL_CAN_PCT          0.05f
#define MATH_PCT              0.10f
#define MATH_PTS              2
#define BALL_PTS              3
#define BALL_S                16.0f  // jak dlouho vydrzi bourak od sebrani (pak zmizi)
#define SWING_RANGE           30     // vzdalenost zombika pred cumakem, ktera spusti svih
#define MAX_PARTS             12     // dily rozpadlych zombiku najednou
#define MAX_BLOOD             24     // kapky krve pri narazu
#define CORNER_R              10     // polomer zaoblenych rohu displeje (body) pro ramecky

// ===== CRT filtr (jako krysa skokan) =====
#define CRT_SOFT              0
#define CRT_ROW_TOP           225
#define CRT_ROW_BOT           80
#define CRT_ROW_BLEED         110
#define CRT_MASK              120
#define CRT_MASK_GAIN         1.25f
#define CRT_VIGNETTE          0.0f
#define CRT_FLICKER           10

// ===== zvuk =====
#define AUDIO_VOLUME          60     // registr kodeku: 38 = -47 dB (skoro neslysitelne), 60 = -20 dB
#define AUDIO_MASTER          0.9f

// ===== render =====
#define LCD_QSPI_HZ           40000000
#define STRIPE_H              32
#define DEBUG_PERIOD_MS       1000
