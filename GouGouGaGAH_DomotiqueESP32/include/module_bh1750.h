#ifndef MODULE_BH1750_H
#define MODULE_BH1750_H

#include <Arduino.h>

// Initialise le capteur BH1750 (I2C doit déjà être démarré avec Wire.begin()).
// Retourne true si le capteur a répondu correctement.
bool bh1750_setup();

// Retourne la dernière valeur de luminosité en lux.
// Retourne -1.0 si le capteur n'a pas été initialisé correctement.
float bh1750_readLux();

#endif // MODULE_BH1750_H
