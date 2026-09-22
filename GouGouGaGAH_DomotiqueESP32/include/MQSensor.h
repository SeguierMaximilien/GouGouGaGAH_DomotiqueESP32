#ifndef MQ_SENSOR_H
#define MQ_SENSOR_H

#include <Arduino.h>

// Configure la broche analogique du capteur MQ.
void mq_init();

// Retourne la valeur brute lue (0-4095 sur ESP32).
int mq_readRaw();

// Retourne la lecture en pourcentage de la pleine échelle (0-100%).
float mq_readPercent();

#endif // MQ_SENSOR_H
