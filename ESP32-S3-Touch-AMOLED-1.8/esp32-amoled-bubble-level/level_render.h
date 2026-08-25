#pragma once

#include "Arduino_GFX_Library.h"
#include "pin_config.h"
#include "config.h"

// ===================================================================
// Render: radialni gradient kapaliny (LUT), sklo, bublina pres maly
// pohyblivy canvas, cerna referencni kruznice, text naklonu.
// ===================================================================

#define LCD_CX (LCD_WIDTH / 2)
#define LCD_CY (LCD_HEIGHT / 2)

static Arduino_Canvas *bubbleCanvas = nullptr;
static Arduino_Canvas *textCanvas = nullptr;

static uint16_t caseColor565, liquidLut[1024];
static uint16_t bubbleFill565, bubbleEdge565, bubbleHi565, target565, text565;

static inline uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

static uint16_t lerp565(uint8_t r0, uint8_t g0, uint8_t b0,
                        uint8_t r1, uint8_t g1, uint8_t b1, float t) {
  return rgb565((uint8_t)(r0 + (r1 - r0) * t),
                (uint8_t)(g0 + (g1 - g0) * t),
                (uint8_t)(b0 + (b1 - b0) * t));
}

// LUT indexovana kvadratem polomeru (bez sqrt na pixel): index
// q = r^2 * 1023 / R^2, barva odpovida radiusu sqrt(q/1023) * R.
// U samotneho okraje je zapecen tmavy prstenec skla.
static void buildLiquidLut() {
  const float rimFrac = (float)(LIQUID_RADIUS_PX - 4) / LIQUID_RADIUS_PX;
  for (int q = 0; q < 1024; q++) {
    const float t = sqrtf(q / 1023.0f);
    if (t >= rimFrac) liquidLut[q] = rgb565(GLASS_RIM_DARK);
    else liquidLut[q] = lerp565(LIQUID_CENTER_COLOR, LIQUID_EDGE_COLOR, t / rimFrac);
  }
}

static inline uint16_t liquidColorAt(int x, int y) {
  const int dx = x - LCD_CX, dy = y - LCD_CY;
  const uint32_t r2 = (uint32_t)(dx * dx + dy * dy);
  const uint32_t R2 = (uint32_t)LIQUID_RADIUS_PX * LIQUID_RADIUS_PX;
  if (r2 >= R2) return caseColor565;
  return liquidLut[r2 * 1023u / R2];
}

// svetly oblouk odlesku skla vlevo nahore; kresli se posunuty o
// (offX, offY), takze funguje na displej i do canvasu (ten si oreze)
static void drawGlassShineTo(Arduino_GFX *dst, int offX, int offY) {
  const uint16_t c = rgb565(GLASS_RIM_LIGHT);
  for (float a = 195; a <= 255; a += 0.15f) {
    const float ca = cosf(a * DEG_TO_RAD), sa = sinf(a * DEG_TO_RAD);
    for (int r = LIQUID_RADIUS_PX - 9; r <= LIQUID_RADIUS_PX - 6; r++) {
      dst->drawPixel((int)(LCD_CX + ca * r) - offX,
                     (int)(LCD_CY + sa * r) - offY, c);
    }
  }
}

static void drawTargetCircleTo(Arduino_GFX *dst, int offX, int offY) {
  for (int r = TARGET_RADIUS_PX - 1; r <= TARGET_RADIUS_PX + TARGET_THICKNESS_PX - 1; r++)
    dst->drawCircle(LCD_CX - offX, LCD_CY - offY, r, target565);
}

// bublina se stredem (bx, by) v absolutnich display souradnicich
static void drawBubbleTo(Arduino_GFX *dst, int bx, int by, int offX, int offY) {
  const int x = bx - offX, y = by - offY;
  const int rOut = BUBBLE_RADIUS_PX + BUBBLE_EDGE_THICKNESS_PX;
  dst->fillCircle(x, y, rOut, bubbleEdge565);
  dst->fillCircle(x, y, BUBBLE_RADIUS_PX, bubbleFill565);
  dst->drawCircle(x, y, BUBBLE_RADIUS_PX - 1, bubbleHi565);  // svetly lem
  // srpek odlesku nahore: svetla elipsa oriznuta elipsou vyplne
  dst->fillEllipse(x, y - 20, 24, 12, bubbleHi565);
  dst->fillEllipse(x, y - 15, 24, 12, bubbleFill565);
}

static bool renderInit(Arduino_GFX *gfx) {
  caseColor565 = rgb565(CASE_COLOR);
  bubbleFill565 = rgb565(BUBBLE_FILL_COLOR);
  bubbleEdge565 = rgb565(BUBBLE_EDGE_COLOR);
  bubbleHi565 = rgb565(BUBBLE_HIGHLIGHT_COLOR);
  target565 = rgb565(TARGET_COLOR);
  text565 = rgb565(TILT_TEXT_COLOR);
  buildLiquidLut();

  bubbleCanvas = new Arduino_Canvas(BUBBLE_CANVAS_PX, BUBBLE_CANVAS_PX, gfx, 0, 0);
  if (!bubbleCanvas->begin(GFX_SKIP_OUTPUT_BEGIN)) return false;
  textCanvas = new Arduino_Canvas(96, 40, gfx, 4, 4);
  if (!textCanvas->begin(GFX_SKIP_OUTPUT_BEGIN)) return false;
  return true;
}

// cely staticky obraz: pouzdro + kapalina po radcich pres LUT,
// odlesk skla, kruznice
static void drawStaticScene(Arduino_GFX *gfx) {
  static uint16_t row[LCD_WIDTH];
  for (int y = 0; y < LCD_HEIGHT; y++) {
    for (int x = 0; x < LCD_WIDTH; x++) row[x] = liquidColorAt(x, y);
    gfx->draw16bitRGBBitmap(0, y, row, LCD_WIDTH, 1);
  }
  drawGlassShineTo(gfx, 0, 0);
  drawTargetCircleTo(gfx, 0, 0);
}

// prekresli ctvercove okno BUBBLE_CANVAS_PX x BUBBLE_CANVAS_PX s levym
// hornim rohem (wx, wy): gradient -> bublina -> odlesk a kruznice navrch
static void renderWindow(Arduino_GFX *gfx, int wx, int wy, int bubX, int bubY) {
  uint16_t *fb = bubbleCanvas->getFramebuffer();
  for (int j = 0; j < BUBBLE_CANVAS_PX; j++)
    for (int i = 0; i < BUBBLE_CANVAS_PX; i++)
      fb[j * BUBBLE_CANVAS_PX + i] = liquidColorAt(wx + i, wy + j);
  drawBubbleTo(bubbleCanvas, bubX, bubY, wx, wy);
  drawGlassShineTo(bubbleCanvas, wx, wy);
  drawTargetCircleTo(bubbleCanvas, wx, wy);
  gfx->draw16bitRGBBitmap(wx, wy, fb, BUBBLE_CANVAS_PX, BUBBLE_CANVAS_PX);
}

static inline int clampi(int v, int lo, int hi) {
  return v < lo ? lo : (v > hi ? hi : v);
}

// prekresleni pohybu bubliny ze stare pozice na novou; pokud se obe
// bounding-box vejdou do jednoho okna, staci jedno, jinak dve
static void renderBubbleMove(Arduino_GFX *gfx, int oldX, int oldY, int newX, int newY) {
  const int e = BUBBLE_RADIUS_PX + BUBBLE_EDGE_THICKNESS_PX + 1;
  const int minX = min(oldX, newX) - e, maxX = max(oldX, newX) + e;
  const int minY = min(oldY, newY) - e, maxY = max(oldY, newY) + e;
  const int wxMax = LCD_WIDTH - BUBBLE_CANVAS_PX;
  const int wyMax = LCD_HEIGHT - BUBBLE_CANVAS_PX;
  if (maxX - minX <= BUBBLE_CANVAS_PX && maxY - minY <= BUBBLE_CANVAS_PX) {
    renderWindow(gfx, clampi(minX, 0, wxMax), clampi(minY, 0, wyMax), newX, newY);
  } else {
    renderWindow(gfx, clampi(oldX - BUBBLE_CANVAS_PX / 2, 0, wxMax),
                 clampi(oldY - BUBBLE_CANVAS_PX / 2, 0, wyMax), newX, newY);
    renderWindow(gfx, clampi(newX - BUBBLE_CANVAS_PX / 2, 0, wxMax),
                 clampi(newY - BUBBLE_CANVAS_PX / 2, 0, wyMax), newX, newY);
  }
}

// text naklonu v levem hornim rohu; prekresli jen pri zmene hodnoty
static void renderTiltText(float txDeg, float tyDeg) {
  static int lastTx = 9999, lastTy = 9999;
  static uint32_t lastMs = 0;
  const uint32_t now = millis();
  if (now - lastMs < TILT_TEXT_PERIOD_MS) return;
  const int tx = (int)roundf(txDeg * 10), ty = (int)roundf(tyDeg * 10);
  if (tx == lastTx && ty == lastTy) return;
  lastTx = tx; lastTy = ty; lastMs = now;
  textCanvas->fillScreen(caseColor565);
  textCanvas->setTextSize(2);
  textCanvas->setTextColor(text565);
  textCanvas->setCursor(0, 0);
  textCanvas->printf("X:%+5.1f", txDeg);
  textCanvas->setCursor(0, 21);
  textCanvas->printf("Y:%+5.1f", tyDeg);
  textCanvas->flush();
}
