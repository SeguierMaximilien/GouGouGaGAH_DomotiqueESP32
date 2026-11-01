#ifndef MODULE_HA_H
#define MODULE_HA_H

#include <Arduino.h>

// Liaison avec Home Assistant : WiFi + MQTT, avec découverte automatique
// (les capteurs et le relais apparaissent tout seuls dans Home Assistant).
// Identifiants dans include/secrets.h (voir secrets_exemple.h).

// Lance la connexion WiFi (non bloquant).
void ha_setup();

// À appeler à chaque tour de loop() : garde WiFi + MQTT connectés,
// envoie les mesures toutes les 10 s et reçoit les ordres du relais.
void ha_loop();

// Signale un badge lu par le RC522 (envoyé tout de suite à Home Assistant).
void ha_publierBadge(const String &uid);

// true si l'ESP32 est connecté au broker MQTT.
bool ha_estConnecte();

#endif // MODULE_HA_H
