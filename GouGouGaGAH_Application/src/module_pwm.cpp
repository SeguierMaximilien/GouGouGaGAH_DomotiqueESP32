#include "module_pwm.h"
#include "pinout.h"

static const uint32_t FREQUENCE = 5000; // Hz : pas de scintillement visible
static const uint8_t RESOLUTION = 8;    // bits -> valeurs 0..255
static int pourcentActuel = 0;

void pwm_setup() {
  ledcAttach(PWM_PIN, FREQUENCE, RESOLUTION);
  pwm_set(0);
  Serial.println(F("[PWM] OK (0 %)"));
}

void pwm_set(int pourcent) {
  pourcentActuel = constrain(pourcent, 0, 100);
  ledcWrite(PWM_PIN, pourcentActuel * 255 / 100);
}

int pwm_get() {
  return pourcentActuel;
}
