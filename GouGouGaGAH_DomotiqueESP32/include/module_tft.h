#ifndef MODULE_TFT_H
#define MODULE_TFT_H

#include <Arduino.h>

// Initialise l'écran TFT ILI9341 et le tactile XPT2046
// (SPI.begin() doit déjà avoir été appelé).
void tft_setup();

// Affiche un écran de test (blocs de couleur + texte).
void tft_showTestScreen();

// Affiche une ligne de texte à une position donnée (petit helper).
void tft_printLine(int x, int y, const String &text);

// À appeler régulièrement dans loop(). Si l'écran est touché,
// remplit x/y avec les coordonnées brutes et retourne true.
bool tft_checkTouch(int &x, int &y);

#endif // MODULE_TFT_H
