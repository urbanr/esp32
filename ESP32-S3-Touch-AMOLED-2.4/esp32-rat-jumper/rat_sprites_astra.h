#pragma once

#include <Arduino.h>
#include "rat_sprites_def.h"

// ===================================================================
// Sada spritu ASTRA - vygenerovano skriptem z balicku kanal-komplet.zip
// (ChatGPT Astra). Neupravovat rucne; znaky -> barvy podle ASTRA_LEGEND
// (barvy nad C_COUNT jsou v PALETTE_EXTRA). Obsahuje i dlazdice prostredi.
// ===================================================================

#define SPRITES_HAVE_TILES 1
#define SPRITES_HAVE_CUPS 1
#define SPRITES_HAVE_ADDON 1

#define PALETTE_EXTRA_COUNT 2
static const uint8_t PALETTE_EXTRA[PALETTE_EXTRA_COUNT > 0 ? PALETTE_EXTRA_COUNT : 1][3] = {
  {184, 92, 118}, {16, 14, 22}
};

// znak -> index barvy (C_* nebo C_COUNT + k)
struct LegendEntry { char ch; uint8_t idx; };
static const LegendEntry ASTRA_LEGEND[] = {
  { '!', C_GOLD_DARK },
  { '#', C_FOAM },
  { '0', C_CEIL },
  { '1', C_CEIL_LIGHT },
  { '2', C_PAPER_DARK },
  { '3', C_STONE },
  { '4', C_STONE_DARK },
  { '5', C_WATER },
  { '6', C_WATER_LIGHT },
  { '7', C_WATER_DARK },
  { '8', C_MOSS },
  { '9', C_TEXT },
  { 'C', C_SLIME },
  { 'G', C_RAT },
  { 'H', C_BRICK_DARK },
  { 'K', C_PINK },
  { 'L', C_CAN },
  { 'M', C_PAPER },
  { 'N', C_BRICK },
  { 'O', C_RED },
  { 'R', C_PIPE_DARK },
  { 'S', C_PIPE },
  { 'V', C_STONE_LIGHT },
  { 'X', C_BRICK2 },
  { 'b', C_CRATE },
  { 'c', C_CRATE_DARK },
  { 'e', C_CRATE_LIGHT },
  { 'g', C_COUNT + 0 },
  { 'k', C_OUTLINE },
  { 'l', C_WHITE },
  { 'm', C_PEAR },
  { 'n', C_SLIME_DARK },
  { 'o', C_SPIDER },
  { 'p', C_RAT_LIGHT },
  { 'r', C_RAT_DARK },
  { 's', C_PIPE_LIGHT },
  { 't', C_BROWN },
  { 'u', C_GOLD },
  { 'v', C_PEAR_DARK },
  { 'w', C_COUNT + 1 },
  { 'x', C_MORTAR },
  { 'y', C_SPIDER_LIGHT },
};
#define ASTRA_LEGEND_COUNT ((int)(sizeof(ASTRA_LEGEND) / sizeof(ASTRA_LEGEND[0])))

// png/rat_run_0.png 20x12
SPRITE(SPR_RAT_RUN0,
  "....................",
  "...........kkk......",
  "..........kkKkkk....",
  "..........kKggkGk...",
  "........kkkKggkllk..",
  "K....kkkppkkKkklwk..",
  "kKKK.kppGGGkkkGlwkKK",
  ".kkkkGGGGGGGGGGGGGKK",
  "....kGrGGGGGGGGrrGkk",
  ".....kkkrrrrrkrkkk..",
  "......KKkkkkkkkkKK..",
  ".....kkkk.......kkk.")

// png/rat_run_1.png 20x12
SPRITE(SPR_RAT_RUN1,
  "....................",
  "...........kkk......",
  "..........kkKkkk....",
  "..........kKggkGk...",
  "........kkkKggkllk..",
  ".....kkkppkkKkklwk..",
  ".....kppGGGkkkGlwkKK",
  "...KkGGGGGGGGGGGGGKK",
  "KKKkkGrGGGGGGGGrrGkk",
  "kkk..kkrkrrrrrkkkk..",
  ".......kkkKkkKK.....",
  ".........kkkkkk.....")

// png/rat_jump.png 20x12
SPRITE(SPR_RAT_JUMP,
  "....................",
  "...........kkk......",
  "..........kkKkkk....",
  "KK........kKggkGk...",
  "kkK.....kkkKggkllk..",
  "..K..kkkppkkKkklwk..",
  "..kKKkppGGGkkkGlwkKK",
  "...kkGGGGGGGGGGGGGKK",
  ".....GrGGGGGGGGrrGkk",
  ".....kkrkrKrrrKkkk..",
  ".......kkkkkkkkk....",
  "....................")

// png/spider_0.png 11x9
SPRITE(SPR_SPIDER0,
  "kk.........",
  "..k.kkk.k..",
  "kk.kyyyk.kk",
  "..kooooyk..",
  "kkkoooookkk",
  "..koooook..",
  "kkkoOoOokkk",
  "..kkoookk..",
  "....kkk..kk")

// png/spider_1.png 11x9
SPRITE(SPR_SPIDER1,
  ".........kk",
  "..k.kkk.k..",
  "kk.kyyyk.kk",
  "..kooooyk..",
  "kkkoooookkk",
  "..koooook..",
  "kkkoOoOokkk",
  "..kkoookk..",
  "kk..kkk....")

// png/can.png 7x10
SPRITE(SPR_CAN,
  ".kkkkk.",
  "kLlLLLk",
  "kLrrrLk",
  "kLlLLLk",
  "kOlOOOk",
  "kOlOOOk",
  "kOlOOOk",
  "kLlLLLk",
  "krrrrrk",
  ".kkkkk.")

// png/pear.png 8x10
SPRITE(SPR_PEAR,
  ".....kk.",
  ".....k..",
  "...kkk..",
  "...mmk..",
  "..kmmk..",
  ".kmMmkk.",
  "kkMmmmmk",
  "kmmmmmvk",
  "kkvvvvmk",
  "..kkkk..")

// png/paper.png 8x7
SPRITE(SPR_PAPER,
  "..kkkk..",
  ".kMMMMk.",
  "kMVMMMMk",
  "kMMVVMk.",
  "kMMVMVk.",
  ".kMVMMMk",
  "..kkkk..")

// png/toilet_roll.png 9x8
SPRITE(SPR_ROLL,
  "..kkk....",
  ".klllkkk.",
  "klltllVM.",
  "kltktlVM.",
  "kltktlVM.",
  "klltllVM.",
  ".klllkMM.",
  "..kkkkkk.")

// png/pipe.png 10x14
SPRITE(SPR_PIPE_STUB,
  "kkkkkkkkkk",
  "kssssssssk",
  "kSSSSSSSSk",
  "kkkkkkkkkk",
  "..ksSSRk..",
  "..ksSSRk..",
  "..ksSSRk..",
  "..ksSSRk..",
  "..ksSSRk..",
  "..ksSSRk..",
  "..ksSSRk..",
  ".kkkkkkkk.",
  ".kSSSSSSk.",
  ".kkkkkkkk.")

// png/crate.png 12x11
SPRITE(SPR_CRATE,
  "kkkkkkkkkkkk",
  "keeeeeeeeeek",
  "kbebcbbbcebk",
  "kbbecbbbebbk",
  "kbbbebbecbbk",
  "kbbbceebcbbk",
  "kbbbebbecbbk",
  "kbbecbbbebbk",
  "kbebcbbbcebk",
  "kcccccccccck",
  "kkkkkkkkkkkk")

// png/slime.png 16x5
SPRITE(SPR_SLIME,
  ".....kkkk.......",
  ".....kCMkk...k..",
  "..kkkCCCCkkkCCk.",
  "kkCnnnnCCCCCCCCk",
  ".kkkkkkkkkkkkkk.")

// png/heart_full.png 7x6
SPRITE(SPR_HEART,
  ".kk.kk.",
  "kKOkOOk",
  "kOOOOOk",
  "kOOOOOk",
  "..kOk..",
  "...k...")

// png/heart_empty.png 7x6
SPRITE(SPR_HEART_EMPTY,
  ".kk.kk.",
  "kwwkwwk",
  "kwwwwwk",
  "kwwwwwk",
  "..kwk..",
  "...k...")

// tiles/brick_0.png 12x6
SPRITE(TILE_BRICK0,
  "NNNNNNNNNNNx",
  "NNNNNNNNNNNx",
  "NNNNNNNNNNNx",
  "NNNNNNNNNNNx",
  "NNNNNNNNNNNx",
  "xxxxxxxxxxxx")

// tiles/brick_1.png 12x6
SPRITE(TILE_BRICK1,
  "XXXXXXXXXXXx",
  "XXXXXXXXXXXx",
  "XXXXXXXXXXXx",
  "XXXXXXXXXXXx",
  "XXXXXXXXXXXx",
  "xxxxxxxxxxxx")

// tiles/brick_2.png 12x6
SPRITE(TILE_BRICK2,
  "HHHHHHHHHHHx",
  "HHHHHHHHHHHx",
  "HHHHHHHHHHHx",
  "HHHHHHHHHHHx",
  "HHHHHHHHHHHx",
  "xxxxxxxxxxxx")

// tiles/ceiling_repeat.png 24x12
SPRITE(TILE_CEILING,
  "000000111111000000111111",
  "000000111111000000111111",
  "kkkkkkkkkkkkkkkkkkkkkkkk",
  "111111000000111111000000",
  "111111000000111111000000",
  "kkkkkkkkkkkkkkkkkkkkkkkk",
  "000000111111000000111111",
  "000000111111000000111111",
  "kkkkkkkkkkkkkkkkkkkkkkkk",
  ".....0..................",
  ".....0..................",
  ".....0..................")

// tiles/walkway_repeat.png 10x6
SPRITE(TILE_WALKWAY,
  "2222222222",
  "3333333334",
  "3333333334",
  "3333333334",
  "3333333334",
  "4444444444")

// tiles/hole_edge_left.png 3x6
SPRITE(TILE_HOLE_EDGE_L,
  "442",
  "44k",
  "44k",
  "44k",
  "44k",
  "44k")

// tiles/hole_edge_right.png 3x6
SPRITE(TILE_HOLE_EDGE_R,
  "244",
  "k44",
  "k44",
  "k44",
  "k44",
  "k44")

// tiles/water_0.png 24x19
SPRITE(TILE_WATER0,
  "555555555555555555555555",
  "555555555555555666666665",
  "666555555566666555555556",
  "555666666655555555555555",
  "555555555555555555555555",
  "555555555555555555555555",
  "555555555555555555777755",
  "555555555555555555555555",
  "556666555555555555555555",
  "555555555555555555555555",
  "555555555555555555555555",
  "555555555555555555555555",
  "555555555555555555555555",
  "555555555555577775555555",
  "555555555555555555555555",
  "555555555555555555555555",
  "555555555555555555555555",
  "555555555555555555555555",
  "555555555555555555555555")

// tiles/water_1.png 24x19
SPRITE(TILE_WATER1,
  "555555555555555555555555",
  "555555555666666665555555",
  "555566666555555556666555",
  "666655555555555555555666",
  "555555555555555555555555",
  "555555555555555555555555",
  "755555555555555555555777",
  "555555555555555555555555",
  "555556666555555555555555",
  "555555555555555555555555",
  "555555555555555555555555",
  "555555555555555555555555",
  "555555555555555555555555",
  "555555555555555577775555",
  "555555555555555555555555",
  "555555555555555555555555",
  "555555555555555555555555",
  "555555555555555555555555",
  "555555555555555555555555")

// tiles/water_2.png 24x19
SPRITE(TILE_WATER2,
  "555555555555555555555555",
  "555666666665555555555555",
  "666555555556666555555566",
  "555555555555555666666655",
  "555555555555555555555555",
  "555555555555555555555555",
  "777755555555555555555555",
  "555555555555555555555555",
  "555555556666555555555555",
  "555555555555555555555555",
  "555555555555555555555555",
  "555555555555555555555555",
  "555555555555555555555555",
  "555555555555555555577775",
  "555555555555555555555555",
  "555555555555555555555555",
  "555555555555555555555555",
  "555555555555555555555555",
  "555555555555555555555555")

// tiles/water_3.png 24x19
SPRITE(TILE_WATER3,
  "555555555555555555555555",
  "666665555555555555555666",
  "555556666555555566666555",
  "555555555666666655555555",
  "555555555555555555555555",
  "555555555555555555555555",
  "555777755555555555555555",
  "555555555555555555555555",
  "555555555556666555555555",
  "555555555555555555555555",
  "555555555555555555555555",
  "555555555555555555555555",
  "555555555555555555555555",
  "775555555555555555555577",
  "555555555555555555555555",
  "555555555555555555555555",
  "555555555555555555555555",
  "555555555555555555555555",
  "555555555555555555555555")

// tiles/hole_water_0.png 24x6
SPRITE(TILE_HOLE_WATER0,
  "666666666666666666666666",
  "555555555555555555555555",
  "555555555555555555555555",
  "777755555555555555555555",
  "555555555555555555555555",
  "555555555555555555555555")

// tiles/hole_water_1.png 24x6
SPRITE(TILE_HOLE_WATER1,
  "666666666666666666666666",
  "555555555555555555555555",
  "555555555555555555555555",
  "555555777755555555555555",
  "555555555555555555555555",
  "555555555555555555555555")

// tiles/hole_water_2.png 24x6
SPRITE(TILE_HOLE_WATER2,
  "666666666666666666666666",
  "555555555555555555555555",
  "555555555555555555555555",
  "555555555555777755555555",
  "555555555555555555555555",
  "555555555555555555555555")

// tiles/hole_water_3.png 24x6
SPRITE(TILE_HOLE_WATER3,
  "666666666666666666666666",
  "555555555555555555555555",
  "555555555555555555555555",
  "555555555555555555777755",
  "555555555555555555555555",
  "555555555555555555555555")

// tiles/shelf_repeat.png 14x6
SPRITE(TILE_SHELF,
  "kkkkkkkkkkkkkk",
  "RRssssssssssss",
  "RRSSSSSSSSSSSS",
  "RRSSSSSSSSSSSS",
  "RRRRRRRRRRRRRR",
  "kkkkkkkkkkkkkk")

// tiles/shelf_cap_left.png 3x6
SPRITE(TILE_SHELF_CAP_L,
  "kkk",
  "ksS",
  "kSS",
  "kSS",
  "kSS",
  "kkk")

// tiles/shelf_cap_right.png 3x6
SPRITE(TILE_SHELF_CAP_R,
  "kkk",
  "ksk",
  "kSk",
  "kSk",
  "kSk",
  "kkk")

// tiles/moss.png 3x2
SPRITE(TILE_MOSS,
  ".8.",
  "888")

// tiles/panel_intro.png 122x62
SPRITE(TILE_PANEL_INTRO,
  "99999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999",
  "9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9",
  "9o9999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999o9",
  "9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9",
  "99999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999")

// tiles/panel_game_over.png 122x56
SPRITE(TILE_PANEL_OVER,
  "99999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999",
  "9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9",
  "9o9999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9o9",
  "9o9999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999o9",
  "9oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo9",
  "99999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999")

// tiles/spider_thread.png 1x8
SPRITE(TILE_THREAD,
  "p",
  "p",
  "p",
  "p",
  "p",
  "p",
  "p",
  "p")

// tiles/splash_0.png 16x10
SPRITE(TILE_SPLASH0,
  "................",
  "................",
  "................",
  "................",
  "................",
  "................",
  ".......6.6......",
  ".....6.6.6.6....",
  ".....6.....6....",
  "................")

// tiles/splash_1.png 16x10
SPRITE(TILE_SPLASH1,
  "................",
  "................",
  "................",
  "................",
  "......6...6.....",
  "......6...6.....",
  "..6...........6.",
  "..6...........6.",
  "................",
  "................")

// tiles/splash_2.png 16x10
SPRITE(TILE_SPLASH2,
  "................",
  "................",
  ".....6.....6....",
  ".....6.....6....",
  "................",
  "................",
  "................",
  "................",
  "................",
  "................")

// tiles/splash_3.png 16x10
SPRITE(TILE_SPLASH3,
  "....6.......6...",
  "....6.......6...",
  "................",
  "................",
  "................",
  "................",
  "................",
  "................",
  "................",
  "................")

// tiles/pickup_spark_0.png 7x7
SPRITE(TILE_SPARK0,
  ".......",
  ".......",
  "...9...",
  "..9.9..",
  "...9...",
  ".......",
  ".......")

// tiles/pickup_spark_1.png 7x7
SPRITE(TILE_SPARK1,
  ".......",
  "...9...",
  ".......",
  ".9...9.",
  ".......",
  "...9...",
  ".......")

// tiles/pickup_spark_2.png 7x7
SPRITE(TILE_SPARK2,
  "...9...",
  ".......",
  ".......",
  "9.....9",
  ".......",
  ".......",
  "...9...")

// tiles/pickup_spark_3.png 7x7
SPRITE(TILE_SPARK3,
  ".......",
  ".......",
  ".......",
  ".......",
  ".......",
  ".......",
  ".......")

// png/cup_silver.png 10x12
SPRITE(SPR_CUP_SILVER,
  "..kkkkkk..",
  "kkklLLGkkk",
  "k.klLLGk.k",
  "k.klLLGk.k",
  ".kklLLGkk.",
  "..klLGGk..",
  "...kLGk...",
  "....kk....",
  "....lL....",
  "...klLGk..",
  "..klLLGGk.",
  "..kkkkkkk.")

// png/cup_gold.png 10x12
SPRITE(SPR_CUP_GOLD,
  "..kkkkkk..",
  "kkk9uu!kkk",
  "k.k9uu!k.k",
  "k.k9uu!k.k",
  ".kk9uu!kk.",
  "..k9u!!k..",
  "...ku!k...",
  "....kk....",
  "....9u....",
  "...k9u!k..",
  "..k9uu!!k.",
  "..kkkkkkk.")

// png/gold_key.png 12x10
SPRITE(SPR_KEY,
  "..kkk.......",
  ".k999k......",
  "k9ukuuk.....",
  "kuk.kuk.....",
  "kuukukkk....",
  ".kuuuuukk...",
  "..kkk.uukk..",
  "......kuukk.",
  ".......kuukk",
  "........kuu.")

// png/bubble_pickup_0.png 10x10
SPRITE(SPR_BUBBLE0,
  "...kkkk...",
  "..kll66k..",
  ".kl....6k.",
  "k6l.....6k",
  "k6......6k",
  "k6......6k",
  "k6......#k",
  ".k6....#k.",
  "..k666#k..",
  "...kkkk...")

// png/bubble_pickup_1.png 10x10
SPRITE(SPR_BUBBLE1,
  "...kkkk...",
  "..kll66k..",
  ".kl....6k.",
  "k6l.....6k",
  "k6......6k",
  "k6......6k",
  "k6......Kk",
  ".k6....Kk.",
  "..k666Kk..",
  "...kkkk...")

// png/bubble_shield_0.png 26x20
SPRITE(SPR_SHIELD0,
  ".........66666666.........",
  "......666#.......666......",
  ".....6.##...........6.....",
  "...66#ll.............66...",
  "..6.#l.................6..",
  "..6#.l.................6..",
  ".6#.l...................6.",
  "6.#......................6",
  "6#.......................6",
  "6........................6",
  "6........................6",
  "6........................6",
  "6........................6",
  ".6.....................p6.",
  "..6...................p6..",
  "..6..................p.6..",
  "...66..............pp66...",
  ".....6...........pp.6.....",
  "......666........666......",
  ".........66666666.........")

// png/bubble_shield_1.png 26x20
SPRITE(SPR_SHIELD1,
  ".........66666666.........",
  "......666#.......666......",
  ".....6.##...........6.....",
  "...66#ll.............66...",
  "..6.#l.................6..",
  "..6#.l.................6..",
  ".6#.l...................6.",
  "6.#......................6",
  "6#.......................6",
  "6........................6",
  "6........................6",
  "6........................6",
  "6........................6",
  ".6.....................K6.",
  "..6...................K6..",
  "..6..................K.6..",
  "...66..............KK66...",
  ".....6...........KK.6.....",
  "......666........666......",
  ".........66666666.........")

// png/rubber_duck_0.png 24x14
SPRITE(SPR_DUCK0,
  "..............kkkkk.....",
  ".............kkuuukk....",
  ".............k9999ukk...",
  "............ku9999kuk...",
  "............ku9999kuk...",
  "............ku99999usssk",
  ".k.........kkuuuuuuusssk",
  ".kk........kuuuuuuuuskkk",
  "kkukkkkkkkkkuuuuuuuuk...",
  ".kk9uuu99999uuuuuuuuk...",
  ".kkuuu9uuuuu!!uuuuuuuk..",
  "..kkuu!!!!!!uuuuuuukk...",
  "....kuuuuuuuuuuuuukk....",
  ".....kkkkkkkkkkkkkk.....")

// png/rubber_duck_1.png 24x14
SPRITE(SPR_DUCK1,
  "..............kkkkk.....",
  ".............kkuuukk....",
  ".............k9999ukk...",
  "............ku9999kuk...",
  "............ku9999kuk...",
  "............ku99999usssk",
  ".k.........kkuuuuuuusssk",
  ".kk........kuuuuuuuuskkk",
  "kkukkkkkkkkkuuuuuuuuk...",
  ".kku9uu99999uuuuuuuuk...",
  ".kkuuu9uuuuu!!uuuuuuuk..",
  "..kkuu!!!!!!uuuuuuukk...",
  "....kuuuuuuuuuuuuukk....",
  ".....kkkkkkkkkkkkkk.....")

// png/snail_0.png 14x9
SPRITE(SPR_SNAIL0,
  ".........k...k",
  "...kkkkk..k..k",
  "..kkSRRkk.k..k",
  ".kSsRSSRSkk.k.",
  ".kSSRRSRSkkkk.",
  ".kSSRRRRSklvk.",
  "..kkSSSkkkkkvk",
  "kkvkkkkkvvvmkk",
  ".kkkkkkkkkkkkk")

// png/snail_1.png 14x9
SPRITE(SPR_SNAIL1,
  ".........k...k",
  "...kkkkk..k..k",
  "..kkSRRkk.k..k",
  ".kSsRSSRSkk.k.",
  ".kSSRRSRSkkkk.",
  ".kSSRRRRSklvk.",
  "..kkSSSkkkkkvk",
  "kkvkkkkkvvmvkk",
  ".kkkkkkkkkkkkk")

// png/drip_pipe.png 12x12
SPRITE(SPR_DRIP_PIPE,
  "...ksSSRk...",
  "...ksSSRk...",
  "...ksSSRk...",
  "...ksSSRk...",
  "...ksSSRk...",
  "...ksSSRk...",
  "...ksSSRk...",
  ".kkkkkkkkkk.",
  ".kssssssssk.",
  ".kSSSSSSSSk.",
  ".kkk7777kkk.",
  "...kkkkkk...")

// png/drop_ready.png 5x7
SPRITE(SPR_DROP_READY,
  "..k..",
  "..k..",
  ".k6k.",
  ".k#k.",
  "k#66k",
  "k666k",
  ".kkk.")

// png/drop_flash.png 5x7
SPRITE(SPR_DROP_FLASH,
  "..k..",
  "..k..",
  ".k6k.",
  ".klk.",
  "kl66k",
  "k666k",
  ".kkk.")

// png/drop_falling.png 5x7
SPRITE(SPR_DROP_FALL,
  "..k..",
  "..k..",
  ".k6k.",
  ".k#k.",
  "k#66k",
  "k666k",
  ".kkk.")

// png/water_jet_pipe.png 14x8
SPRITE(SPR_JET_PIPE,
  ".kkkkkkkkkkkk.",
  ".kss777777ssk.",
  ".kssssssssssk.",
  ".kkkkkkkkkkkk.",
  "...ksSSSSSk...",
  "...ksSSSSSk...",
  "...ksSSSSSk...",
  "...ksSSSSSk...")

// png/water_jet_repeat_0.png 12x8
SPRITE(SPR_JET_SEG0,
  "...65#5556..",
  "..6555#556..",
  "..655#556...",
  "..6555#556..",
  "...65#5556..",
  "..6555#556..",
  "..655#556...",
  "..6555#556..")

// png/water_jet_repeat_1.png 12x8
SPRITE(SPR_JET_SEG1,
  "..6555#556..",
  "..655#556...",
  "..6555#556..",
  "...65#5556..",
  "..6555#556..",
  "..655#556...",
  "..6555#556..",
  "...65#5556..")

// png/water_jet_repeat_2.png 12x8
SPRITE(SPR_JET_SEG2,
  "..655#556...",
  "..6555#556..",
  "...65#5556..",
  "..6555#556..",
  "..655#556...",
  "..6555#556..",
  "...65#5556..",
  "..6555#556..")

// png/water_jet_repeat_3.png 12x8
SPRITE(SPR_JET_SEG3,
  "..6555#556..",
  "...65#5556..",
  "..6555#556..",
  "..655#556...",
  "..6555#556..",
  "...65#5556..",
  "..6555#556..",
  "..655#556...")

// png/water_jet_top_0.png 16x8
SPRITE(SPR_JET_TOP0,
  ".....6..6.......",
  ".....6#666...6..",
  "..6.6#66#666##6.",
  ".6#66#666###6666",
  "666##66#66666...",
  "...6655#55566...",
  "....655#5556....",
  "....655#5556....")

// png/water_jet_top_1.png 16x8
SPRITE(SPR_JET_TOP1,
  ".....6..6.......",
  ".....6#666...6..",
  "..6.6#66#666##6.",
  ".6#66#666###6666",
  "666##666#6666...",
  "...66555#5566...",
  "....6555#556....",
  "....6555#556....")

// png/water_jet_top_2.png 16x8
SPRITE(SPR_JET_TOP2,
  ".....6..6.......",
  ".....6#666...6..",
  "..6.6#66#666##6.",
  ".6#66#666###6666",
  "666##66#66666...",
  "...6655#55566...",
  "....655#5556....",
  "....655#5556....")

// png/water_jet_top_3.png 16x8
SPRITE(SPR_JET_TOP3,
  ".....6..6.......",
  ".....6#666...6..",
  "..6.6#66#666##6.",
  ".6#66#666###6666",
  "666##666#6666...",
  "...66555#5566...",
  "....6555#556....",
  "....6555#556....")

// png/reward_chest_closed.png 14x13
SPRITE(SPR_CHEST_CLOSED,
  "..............",
  "...kkkkkkkk...",
  ".kuukkkkkkuuk.",
  ".kuueeeeeeuuk.",
  "kkuueeeeeeuukk",
  "kkuueeuueeuukk",
  "kbuubbkubbuubk",
  "kbuubbuubbuubk",
  "kbuubbuubbuubk",
  "kbuubbbbbbuubk",
  "kcuuccccccuuck",
  "kbuubbbbbbuubk",
  "kkkkkkkkkkkkkk")

// png/reward_chest_open.png 14x13
SPRITE(SPR_CHEST_OPEN,
  "kkkkkkkkkkkkkk",
  "kcccccccccccck",
  "kcccccccccccck",
  "kcccccccccccck",
  "kkkk99kkk99kkk",
  "kww9ww999wwwwk",
  "kwwwwwwwwwwwwk",
  "kbuubbbbbbuubk",
  "kbuubbbbbbuubk",
  "kbuubbbbbbuubk",
  "kcuuccccccuuck",
  "kbuubbbbbbuubk",
  "kkkkkkkkkkkkkk")
