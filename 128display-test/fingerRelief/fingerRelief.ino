// Počítadlo kokotů + jednou za 30 s (v náhodný okamžik) se na 2 s ukáže
// reliéfní obrázek přes celý displej. Bitmapa je v finger_relief.h
// (128x128 RGB565 v PROGMEM, vygenerováno z PNG přes emboss filtr).
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include <SPI.h>
#include "finger_relief.h"

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 128

// Piny displeje
const uint8_t OLED_pin_scl_sck  = 18;
const uint8_t OLED_pin_sda_mosi = 23;
const uint8_t OLED_pin_cs_ss    = 5;
const uint8_t OLED_pin_res_rst  = 4;
const uint8_t OLED_pin_dc_rs    = 2;

// Barvy RGB565
const uint16_t OLED_Color_Black = 0x0000;
const uint16_t OLED_Color_Blue  = 0x001F;

uint16_t OLED_Text_Color      = OLED_Color_Black;
uint16_t OLED_Backround_Color = OLED_Color_Blue;

// Časování obrázku
const uint32_t FINGER_WINDOW_MS = 30000;  // okno, ve kterém se obrázek ukáže právě jednou
const uint32_t FINGER_SHOW_MS   = 2000;   // jak dlouho zůstane
const uint32_t COUNTER_STEP_MS  = 2000;   // interval počítadla

Adafruit_SSD1351 oled(
    SCREEN_WIDTH,
    SCREEN_HEIGHT,
    &SPI,
    OLED_pin_cs_ss,
    OLED_pin_dc_rs,
    OLED_pin_res_rst
);

// Buffer pouze pro oblast s počítadlem (128 × 32 × 2 B = 8 KB).
GFXcanvas16 counterCanvas(SCREEN_WIDTH, 32);

int cycles = 0;

uint32_t windowStart   = 0;
uint32_t fingerAt      = 0;
bool     fingerShown   = false;
uint32_t lastCounterAt = 0;

const size_t MaxString = 32;

void displayCounter()
{
    char newTimeString[MaxString] = {0};
    int currentCycles = cycles;

    if (currentCycles == 0) {
        snprintf(newTimeString, sizeof(newTimeString), "%d KOKOTU", currentCycles);
    }
    else if (currentCycles == 1) {
        snprintf(newTimeString, sizeof(newTimeString), "%d KOKOT", currentCycles);
    }
    else if (currentCycles <= 4) {
        snprintf(newTimeString, sizeof(newTimeString), "%d KOKOTI", currentCycles);
    }
    else {
        snprintf(newTimeString, sizeof(newTimeString), "%d KOKOTU", currentCycles);
    }

    counterCanvas.fillScreen(OLED_Backround_Color);
    counterCanvas.setTextColor(OLED_Text_Color);
    counterCanvas.setTextSize(2);
    counterCanvas.setCursor(5, 5);
    counterCanvas.print(newTimeString);

    oled.drawRGBBitmap(0, 70, counterCanvas.getBuffer(), SCREEN_WIDTH, 32);
}

void drawStaticScreen()
{
    oled.fillScreen(OLED_Backround_Color);
    oled.setTextColor(OLED_Text_Color);
    oled.setTextSize(1);
    oled.setCursor(15, 20);
    oled.print("Pocitadlo kokotu");
    oled.setCursor(5, 35);
    oled.print("ve strane Motoriste");
}

void scheduleFinger()
{
    windowStart = millis();
    // náhodný okamžik v okně tak, aby se obrázek stihl celý zobrazit
    fingerAt    = windowStart + random(0, FINGER_WINDOW_MS - FINGER_SHOW_MS);
    fingerShown = false;
    Serial.printf("Obrazek za %lu ms\n", (unsigned long)(fingerAt - windowStart));
}

void showFinger()
{
    // Flash je na ESP32 mapovaná do adresního prostoru, takže lze použít
    // rychlou cestu drawRGBBitmap (setAddrWindow + writePixels po řádcích).
    Serial.println("Zobrazuji obrazek");
    oled.drawRGBBitmap(0, 0, (uint16_t *)fingerRelief, FINGER_W, FINGER_H);
    delay(FINGER_SHOW_MS);
    drawStaticScreen();
    displayCounter();
}

void setup()
{
    Serial.begin(115200);
    delay(250);

    oled.begin();
    oled.setFont();

    // Při startu obrázek ukázat hned (ověření, že se vykresluje).
    showFinger();

    lastCounterAt = millis();
    scheduleFinger();
}

void loop()
{
    uint32_t now = millis();

    if (!fingerShown && (int32_t)(now - fingerAt) >= 0) {
        showFinger();
        fingerShown = true;
        now = millis();
    }

    if (now - windowStart >= FINGER_WINDOW_MS) {
        scheduleFinger();
    }

    if (now - lastCounterAt >= COUNTER_STEP_MS) {
        lastCounterAt = now;
        cycles++;
        displayCounter();
    }

    delay(10);
}
