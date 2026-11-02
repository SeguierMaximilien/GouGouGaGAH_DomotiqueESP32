#ifndef CONFIG_RESEAU_H
#define CONFIG_RESEAU_H

// ========================================================
// Adresse du broker MQTT (= le Raspberry Pi avec Home Assistant).
// Ce fichier est envoyé sur GitHub : il remplace l'adresse de secrets.h,
// comme ça un simple Pull suffit quand le Pi change d'adresse.
// Le nom et le mot de passe du WiFi restent dans secrets.h.
// ========================================================

#undef MQTT_SERVEUR

// ---- Configuration actuelle (02/11/2026) ----
// Pi en WiFi, wlan0 = 10.252.23.42/24 (http://homeassistant.local:8123)
#define MQTT_SERVEUR "10.252.23.42"

// ---- Ancienne configuration (maison) ----
// Pi en WiFi sur "Free_HOME" (réseau caché), http://192.168.1.110:8123
// #define MQTT_SERVEUR "192.168.1.110"
// Encore avant : Pi en Ethernet
// #define MQTT_SERVEUR "192.168.1.18"

#endif // CONFIG_RESEAU_H
