#include "module_bmp280.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>

static Adafruit_BMP280 bmp;
static bool bmp280_ready = false;

bool bmp280_setup() {
  bmp280_ready = bmp.begin(0x76);
  if (bmp280_ready) {
    bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                     Adafruit_BMP280::SAMPLING_X2,
                     Adafruit_BMP280::SAMPLING_X16,
                     Adafruit_BMP280::FILTER_X16,
                     Adafruit_BMP280::STANDBY_MS_500);
    Serial.println(F("[BMP280] OK"));
  } else {
    Serial.println(F("[BMP280] ECHEC (essayer 0x77, verifier CSB->3V3 / SDO->GND)"));
  }
  return bmp280_ready;
}

float bmp280_readTemperature() {
  if (!bmp280_ready) return -999.0;
  return bmp.readTemperature();
}

float bmp280_readPressure() {
  if (!bmp280_ready) return -1.0;
  return bmp.readPressure() / 100.0F; // Pa -> hPa
}
