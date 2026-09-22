#ifndef BMP280_SENSOR_H
#define BMP280_SENSOR_H

#include <Arduino.h>

// Initialise le capteur BMP280 (I2C, adresse 0x76). Retourne true si OK.
bool bmp280_init();

// Retourne la température en °C, ou NAN si le capteur n'est pas initialisé.
float bmp280_readTemperature();

// Retourne la pression en hPa, ou NAN si le capteur n'est pas initialisé.
float bmp280_readPressure();

#endif // BMP280_SENSOR_H
