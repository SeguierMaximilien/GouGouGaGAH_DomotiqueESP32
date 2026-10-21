#ifndef MODULE_TFT_H
#define MODULE_TFT_H

#include <Arduino.h>
#include <Adafruit_ILI9341.h>

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

// Comme tft_checkTouch, mais convertit en pixels écran (0..tft.width()-1, 0..tft.height()-1)
// à partir des valeurs de calibration TOUCH_* définies dans module_tft.cpp.
bool tft_checkTouchPixel(int &x, int &y);

// Accès direct à l'écran, pour les modules qui dessinent eux-mêmes (menu...).
Adafruit_ILI9341 &tft_get();

#endif // MODULE_TFT_H
