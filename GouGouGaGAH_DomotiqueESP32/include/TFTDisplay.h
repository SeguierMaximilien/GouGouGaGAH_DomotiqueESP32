#ifndef TFT_DISPLAY_H
#define TFT_DISPLAY_H

#include <Arduino.h>

// Initialise l'écran TFT ILI9341 (SPI).
void tft_init();

// Affiche un écran de test avec du texte et des blocs de couleur.
void tft_showTestScreen();

// Efface l'écran (noir).
void tft_clear();

#endif // TFT_DISPLAY_H
