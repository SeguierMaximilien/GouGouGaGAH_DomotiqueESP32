#ifndef BH1750_SENSOR_H
#define BH1750_SENSOR_H

#include <Arduino.h>

// Initialise le capteur BH1750 (I2C). Retourne true si OK.
bool bh1750_init();

// Retourne la luminosité en lux, ou -1 si le capteur n'est pas initialisé.
float bh1750_readLux();

#endif // BH1750_SENSOR_H
