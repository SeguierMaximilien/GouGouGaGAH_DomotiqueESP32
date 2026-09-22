#ifndef TOUCH_SCREEN_H
#define TOUCH_SCREEN_H

#include <Arduino.h>

// Initialise le contrôleur tactile XPT2046 (SPI, partagé avec l'écran).
void touch_init();

// Retourne true si l'écran est touché. Remplit x/y (coordonnées brutes)
// si un contact est détecté.
bool touch_getPoint(int &x, int &y);

#endif // TOUCH_SCREEN_H
