#ifndef MODULE_RFID_H
#define MODULE_RFID_H

#include <Arduino.h>

// Initialise le lecteur RC522 (SPI.begin() doit déjà avoir été appelé).
// Retourne true si le module a répondu correctement.
bool rfid_setup();

// À appeler régulièrement dans loop(). Si une carte est détectée,
// retourne true et remplit uidStr avec l'UID en hexadécimal (ex: "04 A3 F1 2B").
bool rfid_checkCard(String &uidStr);

#endif // MODULE_RFID_H
