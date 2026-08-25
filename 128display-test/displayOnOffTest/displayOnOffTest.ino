#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include <SPI.h>

#define SerialDebugging true

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 128

// Piny displeje
const uint8_t OLED_pin_scl_sck  = 18;
const uint8_t OLED_pin_sda_mosi = 23;
const uint8_t OLED_pin_cs_ss    = 5;
const uint8_t OLED_pin_res_rst  = 4;
const uint8_t OLED_pin_dc_rs    = 2;

// Tlačítko
const uint8_t Button_pin = 27;

// Barvy RGB565
const uint16_t OLED_Color_Black   = 0x0000;
const uint16_t OLED_Color_Blue    = 0x001F;
const uint16_t OLED_Color_Red     = 0xF800;
const uint16_t OLED_Color_Green   = 0x07E0;
const uint16_t OLED_Color_Cyan    = 0x07FF;
const uint16_t OLED_Color_Magenta = 0xF81F;
const uint16_t OLED_Color_Yellow  = 0xFFE0;
const uint16_t OLED_Color_White   = 0xFFFF;

uint16_t OLED_Text_Color      = OLED_Color_Black;
uint16_t OLED_Backround_Color = OLED_Color_Blue;

Adafruit_SSD1351 oled(
    SCREEN_WIDTH,
    SCREEN_HEIGHT,
    &SPI,
    OLED_pin_cs_ss,
    OLED_pin_dc_rs,
    OLED_pin_res_rst
);

// Buffer pouze pro oblast s počítadlem.
// 128 × 32 × 2 bajty = 8192 bajtů RAM.
GFXcanvas16 counterCanvas(SCREEN_WIDTH, 32);

volatile bool isButtonPressed = false;
volatile int cycles = 0;

const size_t MaxString = 32;

void IRAM_ATTR senseButtonPressed()
{
    isButtonPressed = true;
}

void displayCounter()
{
    char newTimeString[MaxString] = {0};

    int currentCycles = cycles;

    if (currentCycles == 0) {
        snprintf(
            newTimeString,
            sizeof(newTimeString),
            "%d KOKOTU",
            currentCycles
        );
    }
    else if (currentCycles == 1) {
        snprintf(
            newTimeString,
            sizeof(newTimeString),
            "%d KOKOT",
            currentCycles
        );
    }
    else if (currentCycles <= 4) {
        snprintf(
            newTimeString,
            sizeof(newTimeString),
            "%d KOKOTI",
            currentCycles
        );
    }
    else {
        snprintf(
            newTimeString,
            sizeof(newTimeString),
            "%d KOKOTU",
            currentCycles
        );
    }

    // Všechno se nejdřív vykreslí do bufferu v RAM.
    counterCanvas.fillScreen(OLED_Backround_Color);

    counterCanvas.setTextColor(OLED_Text_Color);
    counterCanvas.setTextSize(2);
    counterCanvas.setCursor(5, 5);
    counterCanvas.print(newTimeString);

    // Hotový obraz se následně přenese na displej.
    oled.drawRGBBitmap(
        0,
        70,
        counterCanvas.getBuffer(),
        SCREEN_WIDTH,
        32
    );
}

void setup()
{
    pinMode(Button_pin, INPUT_PULLUP);

    attachInterrupt(
        digitalPinToInterrupt(Button_pin),
        senseButtonPressed,
        FALLING
    );

#if SerialDebugging
    Serial.begin(115200);
    delay(100);
    Serial.println();
#endif

    delay(250);

    isButtonPressed = false;

    oled.begin();
    oled.setFont();

    // Pozadí celého displeje se smaže jen jednou.
    oled.fillScreen(OLED_Backround_Color);

    // Statický text se vykreslí jen jednou.
    oled.setTextColor(OLED_Text_Color);
    oled.setTextSize(1);

    oled.setCursor(15, 20);
    oled.print("Pocitadlo kokotu");

    oled.setCursor(5, 35);
    oled.print("ve strane Motoriste");

    displayCounter();
}

void loop()
{
    displayCounter();

    cycles++;

    delay(2000);
}