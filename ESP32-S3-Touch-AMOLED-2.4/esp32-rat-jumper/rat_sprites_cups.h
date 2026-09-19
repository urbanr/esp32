#pragma once

#include "rat_sprites_def.h"

// ===================================================================
// Pohary sady KLASIK (bonusove odpadky): stribrny za CUP_SILVER_PTS,
// zlaty za CUP_GOLD_PTS. Rucne kreslene, znaky z EXTRA_LEGEND
// (rat_sprites_extra.h). Sada ASTRA ma vlastni (png/cup_*.png ->
// SPRITES_HAVE_CUPS); tento soubor je pro ni jen zaloha bez tech PNG.
// ===================================================================

// stribrny pohar 11x10 (maly)
SPRITE(SPR_CUP_SILVER,
  ".kkkkkkkkk.",
  "kABBBBBBBDk",
  "kkABBBBBDkk",
  "k.kABBBDk.k",
  "kk.kABDk.kk",
  "...kkBkk...",
  "....kBk....",
  "...kBBBk...",
  "..kDDDDDk..",
  "..kkkkkkk..")

// zlaty pohar 13x12 (velky)
SPRITE(SPR_CUP_GOLD,
  "..kkkkkkkkk..",
  ".kFIIIIIIIJk.",
  "kkFIFIIIIIJkk",
  "k.kFIIIIIJk.k",
  "k.kkFIIIJkk.k",
  "kk..kFIJk..kk",
  ".....kIk.....",
  ".....kIk.....",
  "....kIIIk....",
  "...kJJJJJk...",
  "..kJJJJJJJk..",
  "..kkkkkkkkk..")
