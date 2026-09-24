#include "module_bh1750.h"
#include <BH1750.h>

static BH1750 lightMeter;
static bool bh1750_ready = false;

bool bh1750_setup() {
  bh1750_ready = lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);
  if (bh1750_ready) {
    Serial.println(F("[BH1750] OK"));
  } else {
    Serial.println(F("[BH1750] ECHEC (verifier cablage I2C / ADDR->GND)"));
  }
  return bh1750_ready;
}

float bh1750_readLux() {
  if (!bh1750_ready) return -1.0;
  return lightMeter.readLightLevel();
}
