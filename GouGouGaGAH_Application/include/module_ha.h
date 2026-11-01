#ifndef MODULE_HA_H
#define MODULE_HA_H

#include <Arduino.h>

// Liaison avec Home Assistant : WiFi + MQTT, avec découverte automatique
// (capteurs, porte, éclairage PWM et Tempo apparaissent tout seuls dans Home Assistant).
// Identifiants dans include/secrets.h (voir secrets_exemple.h).

// Lance la connexion WiFi (non bloquant).
void ha_setup();

// À appeler à chaque tour de loop() : garde WiFi + MQTT connectés,
// envoie les mesures toutes les 10 s et reçoit les ordres de Home Assistant.
void ha_loop();

// Signale un badge lu par le RC522 et ce qui en a été fait ("autorise", "refuse", "ajoute"...).
void ha_publierBadge(const String &uid, const String &resultat);

bool ha_wifiConnecte();
bool ha_estConnecte(); // connecté au broker MQTT

// Dernier ordre texte reçu sur gougougagah/ordre (automatisations Home Assistant)
String ha_dernierOrdre();

#endif // MODULE_HA_H
