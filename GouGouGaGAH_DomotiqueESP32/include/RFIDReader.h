#ifndef RFID_READER_H
#define RFID_READER_H

#include <Arduino.h>

// Initialise le lecteur RC522 (SPI, partagé avec l'écran/tactile).
bool rfid_init();

// Vérifie si une carte est présente. Remplit uidStr ("XX XX XX XX")
// si une carte est détectée. Retourne true dans ce cas.
bool rfid_checkCard(String &uidStr);

#endif // RFID_READER_H
