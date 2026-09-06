#pragma once

#include <Arduino.h>
#include <esp_random.h>
#include "../common/amoled_app.h"
#include "config.h"
#include "zomb_gfx.h"
#include "zomb_crt.h"
#include "zomb_world.h"
#include "zomb_render.h"
#include "zomb_audio.h"
#include "zomb_save.h"

// ===================================================================
// Modul hry Krysy a zombici: zombBegin() / zombLoop() / zombEnd().
// Vstup: drzeni BOOT = plyn, tuknuti = vyskok; v prikladu tuknuti do
// tretiny displeje = odpoved A/B/C; v garazi plus pod ikonou = nakup,
// tuknuti dole = vyjet; v uvodu tuknuti na krysu = volba a start.
// Dotyk je v souradnicich displeje (na vysku), prevadi se na logicke
// body hry stejne jako v zomb_crt.h (ROTATE_CW).
// ===================================================================

static uint32_t lastUs = 0;
static bool touchPrev = false, swiped = false;
static int touchX0 = 0, touchY0 = 0;   // logicke souradnice stisku (swipe)
static bool audioOk = false;

static void debugPrint(uint32_t usFrame, uint32_t usDraw, uint32_t usPresent) {
#if DEBUG_PERIOD_MS > 0
  static uint32_t lastMs = 0, fSum = 0, dSum = 0, pSum = 0;
  static int frames = 0;
  frames++;
  fSum += usFrame; dSum += usDraw; pSum += usPresent;
  const uint32_t now = millis();
  if (now - lastMs < DEBUG_PERIOD_MS) return;
  lastMs = now;
  USBSerial.printf("fps %.1f  us snimek %lu kresleni %lu pruhy %lu  stav %d x %.0f v %.1f benzin %.2f body %d zvuk %d bloky %lu peak %lu\n",
                   1e6f * frames / fSum, fSum / frames, dSum / frames, pSum / frames, state, ratX, ratV, fuel, points, audioOk, audioBlocks, audioPeak);
  frames = 0; fSum = dSum = pSum = 0; usWait = usCompose = usBlend = 0;
#endif
}

// dotyk -> logicke souradnice hry
static inline void touchLogical(int &lx, int &ly) {
#if ROTATE_CW
  lx = touchY / CRT_SCALE;
  ly = (LCD_WIDTH - 1 - touchX) / CRT_SCALE;
#else
  lx = (LCD_HEIGHT - 1 - touchY) / CRT_SCALE;
  ly = touchX / CRT_SCALE;
#endif
}

static void handleInput() {
  const bool tap = touchDown && !touchPrev;
  touchPrev = touchDown;
  throttle = digitalRead(0) == LOW;
  int lx, ly;
  touchLogical(lx, ly);
  if (tap) { touchX0 = lx; touchY0 = ly; swiped = false; }
  else if (touchDown && !swiped && abs(ly - touchY0) > 30 && abs(lx - touchX0) < 30) {   // swipe dolu = nahodny CRT, nahoru = oblibene
    swiped = true;
    if (ly > touchY0) crtRandomize(); else crtNextFavorite();   // oboji se ulozi na kartu
    crtMsgT = 2.0f;
    { const int v[9] = { crtCur.mode, crtCur.top, crtCur.bot, crtCur.bleed, crtCur.blend, crtCur.mask, crtCur.flicker, crtCur.hum, crtCur.noise };
      settingsStore(v, 9, crtCur.gain, crtCur.gamma); }
    return;
  }
  if (!tap) return;
  switch (state) {
    case ST_TITLE: {
      const int i = lx / 50;
      if (i < unlocked) { rat = i; startSegment(); }
      break;
    }
    case ST_DRIVE: ratJump(); break;
    case ST_MATH: mathAnswer(lx / 50); break;
    case ST_GARAGE:
      if (garageNoMoney) startSegment();
      else if (ly >= 82 && ly <= 106) buyUpgrade(lx / 48 > 2 ? 2 : lx / 48);
      else if (ly > 106) startSegment();
      break;
    case ST_LEVEL_DONE: state = ST_TITLE; break;
    default: break;
  }
}

bool zombBegin() {
  randomSeed(esp_random());
  pinMode(0, INPUT_PULLUP);
  touchPrev = touchDown;
  if (!crtInit(gfx)) { USBSerial.println("CRT init fail (canvas/SPI)"); return false; }
  audioOk = zombAudioBegin();
  USBSerial.println(audioOk ? "ES8311 OK" : "ES8311 init fail - bez zvuku");
  resetProgress();   // postup se neuklada, hra jede vzdy od zacatku
  USBSerial.println(saveBegin() ? "SD karta OK" : "SD karta neni - nastaveni CRT se nepamatuje");
  {
    CrtParams p = CRT_DEFAULT;
    int v[9];
    float gain, gamma;
    if (settingsLoad(v, 9, gain, gamma) && (v[0] == 0 || v[0] == 1)) {
      p = { v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7], v[8], gain, gamma };
      USBSerial.println("CRT nastaveni z karty");
    }
    crtApply(p);
    crtMsgT = 0;
  }
  segStart = 0; segEnd = segmentLen();
  genSegment();
  ratX = 30; ratY = ground(ratX);
  state = ST_TITLE;
  sound(SND_LEVEL);   // znelka po startu
  lastUs = micros();
  return true;
}

void zombLoop() {
  const uint32_t now = micros();
  float dt = (now - lastUs) * 1e-6f;
  lastUs = now;
  if (dt > 0.05f) dt = 0.05f;

  handleInput();
  worldUpdate(dt);
  if (audioOk) zombAudioPump(); else pendingSound = SND_NONE;
  const uint32_t t1 = micros();
  drawScene();
  const uint32_t t2 = micros();
  crtPresent();
  debugPrint(micros() - now, t2 - t1, micros() - t2);
}

void zombEnd() {
  crtEnd();
  if (audioOk) zombAudioEnd();
  saveEnd();
}
