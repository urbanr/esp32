#pragma once

#include "config.h"
#include "rat_sprites_def.h"

// vyber sady spritu (SPRITE_SET v config.h): KLASIK = rucni ASCII,
// ASTRA = balicek kanal-komplet (ChatGPT) vcetne dlazdic prostredi
#if SPRITE_SET == SPRITES_ASTRA
#include "rat_sprites_astra.h"
#else
#include "rat_sprites_klasik.h"
#endif

#ifndef SPRITES_HAVE_TILES
#define SPRITES_HAVE_TILES 0
#endif
