#ifndef MODULE_BMP280_H
#define MODULE_BMP280_H

#include <Arduino.h>

// Initialise le capteur BMP280 (I2C doit déjà être démarré avec Wire.begin()).
// Retourne true si le capteur a répondu correctement (adresse 0x76).
bool bmp280_setup();

// Retourne la dernière température lue en degrés Celsius.
// Retourne -999.0 si le capteur n'a pas été initialisé correctement.
float bmp280_readTemperature();

// Retourne la dernière pression lue en hPa.
// Retourne -1.0 si le capteur n'a pas été initialisé correctement.
float bmp280_readPressure();

#endif // MODULE_BMP280_H
