#pragma once

#include "rat_palette.h"
#include "rat_sprites_def.h"

// ===================================================================
// Sprity spolecne pro obe sady: netopyr (jen parada) a ozdoby na zdi
// (kulate okno kanalu, ventil, mriz odpadu). Pouzivaji jen znaky, ktere
// zadna sada nema v legende (viz EXTRA_LEGEND; generator ASTRA je ma
// vyrazene z poolu), a barvy ze zakladni palety.
// ===================================================================

struct ExtraLegend { char ch; uint8_t idx; };
static const ExtraLegend EXTRA_LEGEND[] = {
  { 'A', C_SILVER_LIGHT }, { 'B', C_SILVER }, { 'D', C_SILVER_DARK },
  { 'F', C_GOLD_LIGHT },   { 'I', C_GOLD },   { 'J', C_GOLD_DARK },
  { 'P', C_BAT },          { 'Q', C_BAT_LIGHT },
  { 'T', C_STONE_DARK },   { 'U', C_STONE },  { 'W', C_STONE_LIGHT },
  { 'Y', C_WATER },        { 'Z', C_BLACK },  { 'i', C_WATER_LIGHT },
  { 'a', C_PIPE },         { 'd', C_PIPE_DARK }, { 'f', C_PIPE_LIGHT },
  { 'h', C_MOSS },         { 'j', C_MOSS_LIGHT }, { 'q', C_CEIL_DARK },
  { 'z', C_RED },
};
#define EXTRA_LEGEND_COUNT ((int)(sizeof(EXTRA_LEGEND) / sizeof(EXTRA_LEGEND[0])))

// netopyr 15x6, dve faze kridel (nahore / dole)
SPRITE(SPR_BAT0,
  "kk...........kk",
  "kPPk..k.k..kPPk",
  ".kPPPkkPkkPPPk.",
  "..kPPPzPzPPPk..",
  "...kk.kPk.kk...",
  "......k.k......")
SPRITE(SPR_BAT1,
  "......k.k......",
  ".....kPPPk.....",
  "kkkkkPzPzPkkkkk",
  "kPPPPPPPPPPPPPk",
  ".kkkk.kPk.kkkk.",
  ".......k.......")

// kulate okno kanalu 15x14: kamenny prstenec, za mrizi modrave svetlo
SPRITE(SPR_WINDOW,
  ".....kkkkk.....",
  "...kkUWWWUkk...",
  "..kUWYYTYYWUk..",
  ".kUWYYYTYYYWUk.",
  ".kUYYYYTYYYYUk.",
  "kUWYYYYTYYYYWUk",
  "kUYTTTTTTTTTYUk",
  "kUYYYiYTYYYYYUk",
  "kUWYYYYTYYYYWUk",
  ".kUYYYYTYYYYUk.",
  ".kUWYYYTYYYWUk.",
  "..kUWYYTYYWUk..",
  "...kkUWWWUkk...",
  ".....kkkkk.....")

// ventil 9x11: kolo na trubce vedouci dolu
SPRITE(SPR_VALVE,
  "...kkk...",
  "..kfaak..",
  ".kakfkak.",
  "kfakakaak",
  "kakkkkkak",
  "kaakakadk",
  ".kakdkak.",
  "..kaadk..",
  "...kkk...",
  "..kdadk..",
  "..kdadk..")

// mriz odpadu ve zdi 12x8 s mechem pod ni
SPRITE(SPR_GRATE,
  "kkkkkkkkkkkk",
  "kUTTTTTTTTUk",
  "kUTZTZTZTZUk",
  "kUTZTZTZTZUk",
  "kUTZTZTZTZUk",
  "kUTTTTTTTTUk",
  "kkkkkkkkkkkk",
  ".hjh...hh...")
