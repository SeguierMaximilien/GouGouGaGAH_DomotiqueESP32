#include "BH1750Sensor.h"
#include "pinout.h"
#include <Wire.h>
#include <BH1750.h>

static BH1750 lightMeter;
static bool bh1750_ready = false;

bool bh1750_init() {
  // Suppose que Wire.begin(I2C_SDA, I2C_SCL) a déjà été appelé
  // (une seule fois, dans le programme principal).
  bh1750_ready = lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);
  return bh1750_ready;
}

float bh1750_readLux() {
  if (!bh1750_ready) return -1;
  return lightMeter.readLightLevel();
}
