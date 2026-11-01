#ifndef MODULE_MQ_H
#define MODULE_MQ_H

#include <Arduino.h>

// Configure la broche analogique du capteur MQ.
void mq_setup();

// Retourne la valeur brute lue (0-4095 sur l'ADC 12 bits de l'ESP32).
int mq_readRaw();

// Retourne la valeur en pourcentage (0-100%) de la plage ADC.
float mq_readPercent();

#endif // MODULE_MQ_H
