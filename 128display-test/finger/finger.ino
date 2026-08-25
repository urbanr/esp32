// ESP32 + 1.5" RGB OLED (SSD1351, 128x128, SPI).
// Displej je černý; jednou v každém 30s okně se v náhodný okamžik
// na 2 s ukáže reliéfní obrázek přes celý displej. Bitmapa je ve
// finger_relief.h (128x128 RGB565 v PROGMEM, z PNG přes emboss filtr).
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include <SPI.h>
#include "finger_relief.h"

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 128

// Piny displeje (SCL=18, SDA=23 jsou hardwarové VSPI piny)
const uint8_t OLED_pin_cs_ss   = 5;
const uint8_t OLED_pin_res_rst = 4;
const uint8_t OLED_pin_dc_rs   = 2;

const uint32_t WINDOW_MS = 30000;  // okno, ve kterém se obrázek ukáže právě jednou
const uint32_t SHOW_MS   = 2000;   // jak dlouho zůstane

Adafruit_SSD1351 oled(
    SCREEN_WIDTH,
    SCREEN_HEIGHT,
    &SPI,
    OLED_pin_cs_ss,
    OLED_pin_dc_rs,
    OLED_pin_res_rst
);

uint32_t windowStart = 0;
uint32_t showAt      = 0;
bool     shown       = false;

void scheduleShow()
{
    windowStart = millis();
    showAt      = windowStart + random(0, WINDOW_MS - SHOW_MS);
    shown       = false;
    Serial.printf("Obrazek za %lu ms\n", (unsigned long)(showAt - windowStart));
}

void showFinger()
{
    Serial.println("Zobrazuji obrazek");
    // Flash je na ESP32 mapovaná do adresního prostoru, takže lze použít
    // rychlou cestu drawRGBBitmap (setAddrWindow + writePixels po řádcích).
    oled.drawRGBBitmap(0, 0, (uint16_t *)fingerRelief, FINGER_W, FINGER_H);
    delay(SHOW_MS);
    oled.fillScreen(0x0000);
}

void setup()
{
    Serial.begin(115200);
    delay(250);

    oled.begin();
    oled.fillScreen(0x0000);

    scheduleShow();
}

void loop()
{
    uint32_t now = millis();

    if (!shown && (int32_t)(now - showAt) >= 0) {
        showFinger();
        shown = true;
        now = millis();
    }

    if (now - windowStart >= WINDOW_MS) {
        scheduleShow();
    }

    delay(10);
}
