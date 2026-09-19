#pragma once

#include <Arduino.h>

// ===================================================================
// Paleta hry (indexy do 8bitoveho framebufferu) a legenda znaku pro
// sprity v ASCII (rat_sprites.h). Barvy jsou RGB888, CRT filtr si je
// prevadi sam. Index 0 = pruhledna (ve spritech '.'), kresli se jako
// cerna, kdyz se pouzije jako vypln.
// ===================================================================

enum : uint8_t {
  C_TRANSP = 0, C_BLACK, C_OUTLINE,
  C_BRICK, C_BRICK2, C_BRICK_DARK, C_MORTAR,
  C_CEIL, C_CEIL_DARK, C_CEIL_LIGHT,
  C_WATER, C_WATER_DARK, C_WATER_LIGHT, C_FOAM,
  C_STONE, C_STONE_DARK, C_STONE_LIGHT,
  C_PIPE, C_PIPE_DARK, C_PIPE_LIGHT,
  C_MOSS, C_MOSS_DARK, C_MOSS_LIGHT,
  C_RAT, C_RAT_DARK, C_RAT_LIGHT, C_PINK,
  C_WHITE, C_RED, C_YELLOW,
  C_SPIDER, C_SPIDER_LIGHT, C_THREAD,
  C_CAN, C_CAN_DARK, C_CAN_RED,
  C_PEAR, C_PEAR_DARK, C_BROWN,
  C_PAPER, C_PAPER_DARK,
  C_SLIME, C_SLIME_DARK,
  C_CRATE, C_CRATE_DARK, C_CRATE_LIGHT,
  C_TEXT, C_TEXT_DARK, C_HUD_BG,
  C_SILVER_LIGHT, C_SILVER, C_SILVER_DARK,
  C_GOLD_LIGHT, C_GOLD, C_GOLD_DARK,
  C_BAT, C_BAT_LIGHT,
  C_COUNT
};

static const uint8_t PALETTE[C_COUNT][3] = {
  {0, 0, 0}, {10, 8, 12}, {40, 24, 20},
  {122, 72, 58}, {104, 60, 50}, {78, 44, 40}, {58, 38, 36},
  {70, 64, 72}, {44, 40, 48}, {98, 92, 100},
  {24, 70, 110}, {16, 48, 80}, {60, 120, 165}, {175, 225, 240},
  {128, 130, 120}, {86, 88, 82}, {172, 174, 162},
  {196, 110, 48}, {130, 66, 28}, {242, 172, 100},
  {70, 150, 60}, {40, 100, 40}, {124, 204, 92},
  {142, 142, 152}, {92, 92, 102}, {202, 202, 212}, {240, 140, 150},
  {250, 250, 245}, {220, 40, 40}, {250, 220, 60},
  {72, 40, 92}, {124, 84, 154}, {205, 205, 205},
  {200, 205, 215}, {120, 125, 140}, {210, 50, 60},
  {182, 212, 70}, {122, 152, 40}, {112, 72, 30},
  {236, 230, 210}, {182, 176, 160},
  {112, 222, 60}, {62, 152, 40},
  {172, 122, 60}, {112, 76, 36}, {212, 162, 92},
  {250, 232, 120}, {122, 102, 40}, {30, 20, 30},
  {250, 250, 245}, {200, 205, 215}, {142, 142, 152},
  {250, 232, 120}, {212, 162, 60}, {130, 96, 28},
  {34, 24, 42}, {96, 72, 116},
};

// legenda znaku spritu -> index barvy ('.' = pruhledna)
static uint8_t legend[128];

static void paletteInit() {
  memset(legend, C_TRANSP, sizeof(legend));
  legend[(int)'k'] = C_OUTLINE;
  legend[(int)'K'] = C_BLACK;
  legend[(int)'g'] = C_RAT;
  legend[(int)'G'] = C_RAT_DARK;
  legend[(int)'l'] = C_RAT_LIGHT;
  legend[(int)'p'] = C_PINK;
  legend[(int)'w'] = C_WHITE;
  legend[(int)'r'] = C_RED;
  legend[(int)'y'] = C_YELLOW;
  legend[(int)'o'] = C_PIPE;
  legend[(int)'O'] = C_PIPE_DARK;
  legend[(int)'L'] = C_PIPE_LIGHT;
  legend[(int)'m'] = C_MOSS;
  legend[(int)'M'] = C_MOSS_DARK;
  legend[(int)'v'] = C_SPIDER_LIGHT;
  legend[(int)'V'] = C_SPIDER;
  legend[(int)'t'] = C_THREAD;
  legend[(int)'s'] = C_CAN;
  legend[(int)'S'] = C_CAN_DARK;
  legend[(int)'R'] = C_CAN_RED;
  legend[(int)'e'] = C_PEAR;
  legend[(int)'E'] = C_PEAR_DARK;
  legend[(int)'b'] = C_BROWN;
  legend[(int)'c'] = C_PAPER;
  legend[(int)'C'] = C_PAPER_DARK;
  legend[(int)'n'] = C_SLIME;
  legend[(int)'N'] = C_SLIME_DARK;
  legend[(int)'x'] = C_CRATE;
  legend[(int)'X'] = C_CRATE_DARK;
  legend[(int)'H'] = C_CRATE_LIGHT;
}
