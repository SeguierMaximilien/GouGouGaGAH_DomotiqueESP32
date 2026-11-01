#ifndef MODULE_RELAIS_H
#define MODULE_RELAIS_H

#include <Arduino.h>

// Configure la broche du relais et le met au repos (coupé).
void relais_setup();

// Colle (true) ou relâche (false) le relais.
void relais_set(bool actif);

// Inverse l'état du relais.
void relais_basculer();

// Retourne true si le relais est collé.
bool relais_estActif();

#endif // MODULE_RELAIS_H
