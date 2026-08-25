#pragma once

#include <Wire.h>
#include "SensorQMI8658.hpp"
#include "pin_config.h"
#include "config.h"

// ===================================================================
// Vstup: QMI8658 (akcelerometr + gyroskop) -> orientace zarizeni.
// Sleduji se dva jednotkove vektory v souradnicich displeje
// (+X doprava, +Y dolu, +Z do displeje):
//   gDir    - smer gravitace (komplementarni filtr gyro + akcelerometr)
//   flowDir - vodorovny smer letu hvezd (gyro; drzen kolmo na gDir;
//             pomalu se vraci k "primo na divaka")
// ===================================================================

struct Vec3 { float x, y, z; };

static inline Vec3 v3(float x, float y, float z) { return {x, y, z}; }
static inline Vec3 add(Vec3 a, Vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
static inline Vec3 sub(Vec3 a, Vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
static inline Vec3 scale(Vec3 a, float k) { return {a.x * k, a.y * k, a.z * k}; }
static inline float dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
static inline Vec3 cross(Vec3 a, Vec3 b) {
  return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
static inline float len(Vec3 a) { return sqrtf(dot(a, a)); }
static inline Vec3 norm(Vec3 a) {
  const float l = len(a);
  return l > 1e-6f ? scale(a, 1.0f / l) : a;
}

static SensorQMI8658 qmi;
static bool imuOk = false;

static Vec3 gDir = {0, 1, 0};      // vychozi: zarizeni svisle
static Vec3 flowDir = {0, 0, -1};  // vychozi: hvezdy leti z displeje na divaka
static Vec3 lastGMeas = {0, 1, 0}; // posledni zmerena gravitace (jednotkova)
static Vec3 lastW = {0, 0, 0};     // posledni uhlova rychlost (rad/s, bez biasu)
static Vec3 gyroBias = {0, 0, 0};
static Vec3 gyroSum = {0, 0, 0};   // okno vzorku v klidu pro odhad biasu
static int  calibCount = 0;
static bool calibrated = false;
static bool haveGravity = false;
static Vec3 lastARaw = {0, 0, 0};  // predchozi vzorek akcelerometru (detekce klidu)
static float rawAz = 0;            // pro diagnostiku mapovani osy Z
static float rawAMag = 0, rawWMag = 0; // |akcel| (g), |gyro| (dps, vc. biasu) - diagnostika
static float gErrMaxDeg = 0;       // max. rozdil gravitace gyro vs. akcelerometr (deg) - diagnostika
static float gSignScore = 0;       // korelace zmeny gravitace (akcel) s predikci gyra - diagnostika
                                   // znamenek gyra: pri pohybu musi byt kladna
static Vec3 prevGMeas = {0, 1, 0};

static bool inputBeginIMU() {
  if (!qmi.begin(Wire, QMI8658_L_SLAVE_ADDRESS, IIC_SDA, IIC_SCL)) return false;
  qmi.configAccelerometer(SensorQMI8658::ACC_RANGE_4G,
                          SensorQMI8658::ACC_ODR_250Hz);
  qmi.configGyroscope(SensorQMI8658::GYR_RANGE_512DPS,
                      SensorQMI8658::GYR_ODR_224_2Hz);
  qmi.enableAccelerometer();
  qmi.enableGyroscope();
  imuOk = true;
  return true;
}

static inline bool inputCalibrated() { return calibrated; }

// vodorovna slozka vektoru (kolmo na gravitaci)
static inline Vec3 horizontal(Vec3 v) { return sub(v, scale(gDir, dot(v, gDir))); }

// vychozi smer letu: vodorovny prumet normaly displeje smerem k divakovi (-Z);
// lezi-li zarizeni na plocho (prumet ~0), smer od horni hrany k dolni (+Y)
static Vec3 defaultFlow() {
  Vec3 p = horizontal(v3(0, 0, -1));
  if (len(p) < FLAT_LIMIT) p = horizontal(v3(0, 1, 0));
  return norm(p);
}

static void inputSample() {
  float ax = 0, ay = 0, az = 0, gx = 0, gy = 0, gz = 0;
  if (!qmi.getAccelerometer(ax, ay, az)) return;
  if (!qmi.getGyroscope(gx, gy, gz)) return;
  rawAz = az;
  lastGMeas = norm(v3(ACCEL_MAP_GX(ax, ay, az),
                      ACCEL_MAP_GY(ax, ay, az),
                      ACCEL_MAP_GZ(ax, ay, az)));
  const Vec3 w = v3(GYRO_MAP_X(gx, gy, gz), GYRO_MAP_Y(gx, gy, gz), GYRO_MAP_Z(gx, gy, gz));

  if (!haveGravity) {
    haveGravity = true;
    gDir = lastGMeas;
    flowDir = defaultFlow();
  }

  // bias gyra = prumer okna vzorku, kdy je zarizeni v klidu; pohyb okno
  // restartuje. Dokud neni bias zmeren, gyro se nepouziva.
  const Vec3 aRaw = v3(ax, ay, az);
  rawAMag = len(aRaw);
  rawWMag = len(w);
  const bool still = rawWMag < GYRO_STILL_DPS && len(sub(aRaw, lastARaw)) < ACCEL_STILL_G;
  lastARaw = aRaw;
  if (!still) {
    calibCount = 0;
    gyroSum = v3(0, 0, 0);
  } else {
    gyroSum = add(gyroSum, w);
    if (++calibCount >= GYRO_CALIB_SAMPLES) {
      gyroBias = scale(gyroSum, 1.0f / calibCount);
      calibrated = true;
      calibCount = 0;
      gyroSum = v3(0, 0, 0);
    }
  }
  lastW = calibrated ? scale(sub(w, gyroBias), DEG_TO_RAD) : v3(0, 0, 0);
}

static void inputRead(float dt) {
  if (!imuOk) return;
  if (qmi.getDataReady()) inputSample();
  if (!haveGravity) return;

  // otoceni zarizeni o w*dt: vektory pevne ve svete se v souradnicich
  // zarizeni otaceji opacne
  const Vec3 dw = scale(lastW, dt);
  const Vec3 dGyro = scale(cross(dw, gDir), -1);
  gSignScore += dot(dGyro, sub(lastGMeas, prevGMeas));
  prevGMeas = lastGMeas;
  gDir = add(gDir, dGyro);
  flowDir = sub(flowDir, cross(dw, flowDir));

  // komplementarni filtr: gravitace z gyra korigovana akcelerometrem
  float c = dot(norm(gDir), lastGMeas);
  c = c > 1 ? 1 : (c < -1 ? -1 : c);
  const float errDeg = acosf(c) * RAD_TO_DEG;
  if (errDeg > gErrMaxDeg) gErrMaxDeg = errDeg;
  gDir = norm(add(gDir, scale(sub(lastGMeas, gDir), GRAVITY_FILTER_ALPHA)));

  // smer letu je vzdy vodorovny
  flowDir = norm(horizontal(flowDir));

  // pomaly navrat k "primo na divaka"; sila roste s vodorovnou slozkou
  // normaly displeje (svisle = plna), lezi-li zarizeni, navrat neni (jen gyro)
  const Vec3 p = horizontal(v3(0, 0, -1));
  const float p2 = dot(p, p);
  if (RECENTER_RATE > 0 && p2 > FLAT_LIMIT * FLAT_LIMIT) {
    float k = RECENTER_RATE * p2 * dt;
    if (k > 1) k = 1;
    const Vec3 target = scale(p, 1.0f / sqrtf(p2));
    flowDir = norm(add(flowDir, scale(sub(target, flowDir), k)));
  }
}

// baze hvezdneho pole v souradnicich displeje:
// W = odkud hvezdy prileti (dalka), V = nahoru, U = do strany
static void inputBasis(Vec3 &U, Vec3 &V, Vec3 &W) {
  W = scale(flowDir, -1);
  V = scale(gDir, -1);
  U = cross(V, W);
}
