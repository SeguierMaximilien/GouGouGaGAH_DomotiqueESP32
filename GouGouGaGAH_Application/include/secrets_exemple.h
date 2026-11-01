#ifndef SECRETS_H
#define SECRETS_H

// ========================================================
// MODELE : copie ce fichier en "secrets.h" (même dossier)
// et remplis-le. secrets.h n'est jamais envoyé sur GitHub.
// ========================================================

// ---- WiFi de la maison (2,4 GHz, l'ESP32 ne voit pas le 5 GHz) ----
#define WIFI_SSID       "NomDeTaBox"
#define WIFI_MOTDEPASSE "MotDePasseWifi"

// ---- Broker MQTT = le Raspberry Pi qui fait tourner Home Assistant ----
// Mets l'adresse IP affichée sur l'écran du Pi (ex : 192.168.1.110).
#define MQTT_SERVEUR    "192.168.1.110"
#define MQTT_PORT       1883
// Le compte "esp32" créé dans Home Assistant (Paramètres > Personnes)
#define MQTT_UTILISATEUR "esp32"
#define MQTT_MOTDEPASSE  "MotDePasseEsp32"

#endif // SECRETS_H
