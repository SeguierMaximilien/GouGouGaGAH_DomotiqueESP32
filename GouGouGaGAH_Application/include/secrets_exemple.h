#ifndef SECRETS_H
#define SECRETS_H

// ========================================================
// MODELE : copie ce fichier en "secrets.h" (même dossier)
// et remplis-le. secrets.h n'est jamais envoyé sur GitHub.
// ========================================================

// ---- WiFi (2,4 GHz, l'ESP32 ne voit pas le 5 GHz) ----
// Configuration actuelle (02/11/2026) : le même WiFi que le Pi (10.252.23.x)
#define WIFI_SSID       "NomDuNouveauWifi"
#define WIFI_MOTDEPASSE "MotDePasseDuNouveauWifi"
// Ancienne configuration (maison) :
// #define WIFI_SSID       "Free_HOME"        // réseau caché
// #define WIFI_MOTDEPASSE "MotDePasseFreeHome"

// ---- Broker MQTT = le Raspberry Pi qui fait tourner Home Assistant ----
// L'adresse utilisée est celle de config_reseau.h (envoyé sur GitHub),
// celle-ci est ignorée. Actuel : 10.252.23.42 ; ancien : 192.168.1.110.
#define MQTT_SERVEUR    "10.252.23.42"
#define MQTT_PORT       1883
// Le compte "esp32" créé dans Home Assistant (Paramètres > Personnes)
#define MQTT_UTILISATEUR "esp32"
#define MQTT_MOTDEPASSE  "MotDePasseEsp32"

#endif // SECRETS_H
