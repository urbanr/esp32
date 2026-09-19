#pragma once

// ===================================================================
// Piny Waveshare ESP32-S3-Touch-AMOLED-2.41 (displej RM690B0 450x600
// pres QSPI, dotyk FT6336 a IMU QMI8658 na I2C, SD karta na SPI).
// Deska nema audio kodek ES8311; expander (EXIO) drzi AMOLED_EN, TE
// a preruseni dotyku / IMU / RTC.
// Pozor: 1.8 bere stejnojmenny soubor z knihovny Mylibrary, tady je
// vlastni - aplikace ho includuji jako "../common/pin_config.h".
// ===================================================================

// displej QSPI
#define LCD_CS     9
#define LCD_SCLK   10
#define LCD_SDIO0  11
#define LCD_SDIO1  12
#define LCD_SDIO2  13
#define LCD_SDIO3  14
#define LCD_RST    21
#define LCD_WIDTH  450
#define LCD_HEIGHT 600
// RM690B0 ma pamet sirsi nez panel: sloupce zacinaji na 16
#define LCD_X_OFF  16
#define LCD_Y_OFF  0

// I2C (dotyk FT6336 0x38, IMU QMI8658 0x6B, RTC PCF85063 0x51, expander 0x20)
#define IIC_SDA    47
#define IIC_SCL    48
#define TP_RST     3
#define TP_INT     -1    // fyzicky na EXIO2, pres GPIO neni - dotyk se cte pollovanim

// expander: bity EXIO
#define EXIO_ADDR       0x20
#define EXIO_AMOLED_TE  0
#define EXIO_AMOLED_EN  1
#define EXIO_TP_INT     2

// napajeni baterie / mereni
#define BAT_PWR    16
#define BAT_ADC    17

// SD karta (SPI)
#define SD_CS      2
#define SD_SCLK    4
#define SD_MOSI    5
#define SD_MISO    6
