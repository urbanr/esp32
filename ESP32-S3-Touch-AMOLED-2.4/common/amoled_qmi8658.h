#pragma once

#include <Arduino.h>
#include <Wire.h>

// ===================================================================
// Minimalni ovladac IMU QMI8658 (akcelerometr + gyroskop pres I2C).
// Nahrazuje knihovnu SensorLib, ktera se s touto deskou neprelozi:
// variant desky 2.41 definuje makra QSPI_D0 a spol., ktera koliduji
// s polozkami vyctu v SensorBHI260AP.hpp (a knihovna se preklada cela).
// Rozhrani je zamerne stejne jako u SensorQMI8658, aby moduly vstupu
// aplikaci zustaly beze zmeny.
// ===================================================================

#define QMI8658_L_SLAVE_ADDRESS 0x6B

class SensorQMI8658 {
public:
  // hodnoty registru CTRL2 (akcelerometr) a CTRL3 (gyroskop)
  enum AccRange : uint8_t { ACC_RANGE_2G = 0, ACC_RANGE_4G = 1, ACC_RANGE_8G = 2, ACC_RANGE_16G = 3 };
  enum AccOdr   : uint8_t { ACC_ODR_1000Hz = 3, ACC_ODR_500Hz = 4, ACC_ODR_250Hz = 5, ACC_ODR_125Hz = 6 };
  enum GyrRange : uint8_t { GYR_RANGE_256DPS = 4, GYR_RANGE_512DPS = 5, GYR_RANGE_1024DPS = 6 };
  enum GyrOdr   : uint8_t { GYR_ODR_896_8Hz = 3, GYR_ODR_448_4Hz = 4, GYR_ODR_224_2Hz = 5, GYR_ODR_112_1Hz = 6 };

  bool begin(TwoWire &w, uint8_t addr, int sda, int scl) {
    _w = &w;
    _addr = addr;
    (void)sda; (void)scl;             // sbernici otevira hwInit()
    uint8_t id = 0;
    if (!read(0x00, &id, 1) || id != 0x05) return false;
    write(0x60, 0xB0);                // softreset
    delay(20);
    write(0x02, 0x60);                // CTRL1: auto increment adresy, SPI 4dratovy
    write(0x08, 0x00);                // CTRL7: zatim oba senzory vypnute
    return true;
  }

  void configAccelerometer(uint8_t range, uint8_t odr) { write(0x03, (uint8_t)((range << 4) | odr)); _aScale = 32768.0f / (2 << range); }
  void configGyroscope(uint8_t range, uint8_t odr)     { write(0x04, (uint8_t)((range << 4) | odr)); _gScale = 32768.0f / (16 << range); }

  void enableAccelerometer() { _ctrl7 |= 0x01; write(0x08, _ctrl7); }
  void enableGyroscope()     { _ctrl7 |= 0x02; write(0x08, _ctrl7); }

  // Pozor: priznaky nove hodnoty ve STATUS0 se plni jen v rezimu
  // syncSmpl, ktery nepouzivame - v beznem rezimu zustavaji nulove
  // a cekani na ne by znamenalo, ze aplikace nedostane data nikdy.
  // Datove registry drzi vzdy posledni vzorek, takze staci vedet,
  // ze je senzor zapnuty.
  bool getDataReady() { return _ctrl7 != 0; }

  bool getAccelerometer(float &x, float &y, float &z) { return readVec(0x35, _aScale, x, y, z); }   // v g
  bool getGyroscope(float &x, float &y, float &z)     { return readVec(0x3B, _gScale, x, y, z); }   // ve stupnich/s

private:
  TwoWire *_w = nullptr;
  uint8_t _addr = QMI8658_L_SLAVE_ADDRESS;
  uint8_t _ctrl7 = 0;
  float _aScale = 32768.0f / 4.0f;     // LSB na g
  float _gScale = 32768.0f / 512.0f;   // LSB na dps

  bool write(uint8_t reg, uint8_t val) {
    _w->beginTransmission(_addr);
    _w->write(reg);
    _w->write(val);
    return _w->endTransmission() == 0;
  }

  bool read(uint8_t reg, uint8_t *buf, size_t n) {
    _w->beginTransmission(_addr);
    _w->write(reg);
    if (_w->endTransmission(false) != 0) return false;
    if (_w->requestFrom((int)_addr, (int)n) != (int)n) return false;
    for (size_t i = 0; i < n; i++) buf[i] = _w->read();
    return true;
  }

  bool readVec(uint8_t reg, float scale, float &x, float &y, float &z) {
    uint8_t b[6];
    if (!read(reg, b, 6)) return false;
    x = (int16_t)(b[0] | (b[1] << 8)) / scale;
    y = (int16_t)(b[2] | (b[3] << 8)) / scale;
    z = (int16_t)(b[4] | (b[5] << 8)) / scale;
    return true;
  }
};
