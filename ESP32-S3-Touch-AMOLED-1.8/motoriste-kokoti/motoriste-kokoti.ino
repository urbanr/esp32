#include <Arduino.h>
#include "Arduino_GFX_Library.h"
#include "Arduino_DriveBus_Library.h"
#include "pin_config.h"
#include <Adafruit_XCA9554.h>

#include "HWCDC.h"
HWCDC USBSerial;


Adafruit_XCA9554 expander;


// ================= DISPLAY =================

Arduino_DataBus *bus = new Arduino_ESP32QSPI(
  LCD_CS,
  LCD_SCLK,
  LCD_SDIO0,
  LCD_SDIO1,
  LCD_SDIO2,
  LCD_SDIO3
);


Arduino_SH8601 *gfx = new Arduino_SH8601(
    bus,
    GFX_NOT_DEFINED,
    0,
    LCD_WIDTH,
    LCD_HEIGHT
);


// canvas jen pro oblast pocitadla (cely radek, vyska 80 px, umisteny na y=240)
Arduino_Canvas *canvas = new Arduino_Canvas(
    LCD_WIDTH,
    80,
    gfx,
    0,
    240
);


// ================= DATA =================

int cycles = 0;


// ================= CENTER TEXT =================

void printCentered(
    Arduino_GFX *dst,
    const char *text,
    int y,
    int size
)
{
    int16_t x1;
    int16_t y1;
    uint16_t w;
    uint16_t h;


    dst->setTextSize(size);


    dst->getTextBounds(
        text,
        0,
        0,
        &x1,
        &y1,
        &w,
        &h
    );


    dst->setCursor(
        (LCD_WIDTH - w) / 2,
        y
    );


    dst->print(text);
}


// ================= COUNTER =================

void drawCounter()
{
    char text[32];


    if (cycles == 0)
        sprintf(text,"%d KOKOTU",cycles);
    else if (cycles == 1)
        sprintf(text,"%d KOKOT",cycles);
    else if (cycles <= 4)
        sprintf(text,"%d KOKOTI",cycles);
    else
        sprintf(text,"%d KOKOTU",cycles);


    // kreslime do bufferu v RAM, ne primo na displej
    canvas->fillScreen(RGB565_WHITE);

    canvas->setTextColor(RGB565_BLACK);


    // y je relativni ke canvasu: puvodnich 250 na displeji = 10 v canvasu
    printCentered(
        canvas,
        text,
        10,
        4
    );


    // hotovy obraz se posle na displej najednou -> zadne problikavani
    canvas->flush();
}


// ================= SETUP =================

void setup()
{
    USBSerial.begin(115200);


    Wire.begin(
        IIC_SDA,
        IIC_SCL
    );


    // napajeni displeje pres expander

    if(!expander.begin(0x20))
    {
        while(1);
    }


    expander.pinMode(0,OUTPUT);
    expander.pinMode(1,OUTPUT);
    expander.pinMode(2,OUTPUT);


    expander.digitalWrite(0,LOW);
    expander.digitalWrite(1,LOW);
    expander.digitalWrite(2,LOW);

    delay(20);


    expander.digitalWrite(0,HIGH);
    expander.digitalWrite(1,HIGH);
    expander.digitalWrite(2,HIGH);



      gfx->begin();


    // alokace bufferu canvasu
    // GFX_SKIP_OUTPUT_BEGIN = nevolat znovu gfx->begin(), uz probehl
    if(!canvas->begin(GFX_SKIP_OUTPUT_BEGIN))
    {
        USBSerial.println("Canvas alloc failed!");
        while(1);
    }


    gfx->fillScreen(
        RGB565_WHITE
    );


    for(int i=0;i<=255;i++)
    {
        gfx->setBrightness(i);
        delay(3);
    }



    gfx->setTextColor(
        RGB565_BLACK
    );


    // prvni radek

    printCentered(
        gfx,
        "Pocitadlo kokotu",
        40,
        3
    );


    // druhy radek

    printCentered(
        gfx,
        "ve strane Motoriste",
        110,
        3
    );


    drawCounter();

}



// ================= LOOP =================

void loop()
{
    drawCounter();

    cycles++;

    delay(2000);
}