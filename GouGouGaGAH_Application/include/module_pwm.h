#ifndef MODULE_PWM_H
#define MODULE_PWM_H

#include <Arduino.h>

// Sortie PWM (gradation d'éclairage, vitesse de VMC) sur PWM_PIN.

void pwm_setup();

// Règle le rapport cyclique en pourcentage (0 = éteint, 100 = plein).
void pwm_set(int pourcent);

int pwm_get();

#endif // MODULE_PWM_H
