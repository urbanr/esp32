#pragma once

#include <Arduino.h>

// ===================================================================
// Zaslepka zvuku: deska ESP32-S3-Touch-AMOLED-2.41 nema audio kodek
// ES8311 ani zesilovac. Rozhrani je stejne jako u 1.8, jen audioBegin()
// vzdy selze - aplikace (rat_audio.h) pak bezi bez zvuku. Zvlast
// nepouzivat piny I2S z 1.8: 9 a 10 jsou tady QSPI displeje.
// ===================================================================

#ifndef AUDIO_RATE
#define AUDIO_RATE   16000
#endif
#define AUDIO_BLOCK  256

typedef void (*AudioFillFn)(int16_t *stereo, int frames);

static bool audioBegin(AudioFillFn, int) { return false; }
static void audioEnd() {}
