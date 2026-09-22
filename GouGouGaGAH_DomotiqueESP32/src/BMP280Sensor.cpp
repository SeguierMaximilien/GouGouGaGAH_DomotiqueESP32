#include "BMP280Sensor.h"
#include "pinout.h"
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>

static Adafruit_BMP280 bmp;
static bool bmp280_ready = false;

bool bmp280_init() {
  // Suppose que Wire.begin(I2C_SDA, I2C_SCL) a déjà été appelé
  // (une seule fois, dans le programme principal).
  bmp280_ready = bmp.begin(0x76);
  if (bmp280_ready) {
    bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                     Adafruit_BMP280::SAMPLING_X2,
                     Adafruit_BMP280::SAMPLING_X16,
                     Adafruit_BMP280::FILTER_X16,
                     Adafruit_BMP280::STANDBY_MS_500);
  }
  return bmp280_ready;
}

float bmp280_readTemperature() {
  if (!bmp280_ready) return NAN;
  return bmp.readTemperature();
}

float bmp280_readPressure() {
  if (!bmp280_ready) return NAN;
  return bmp.readPressure() / 100.0F; // Pa -> hPa
}
