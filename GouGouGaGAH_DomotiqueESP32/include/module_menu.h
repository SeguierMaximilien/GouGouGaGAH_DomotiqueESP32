#ifndef MODULE_MENU_H
#define MODULE_MENU_H

#include <Arduino.h>

// Affiche le menu principal de test sur l'écran tactile
// (tft_setup() et les autres *_setup() doivent déjà avoir été appelés).
void menu_setup();

// À appeler à chaque tour de loop() : gère le tactile et rafraîchit l'écran affiché.
void menu_loop();

#endif // MODULE_MENU_H
