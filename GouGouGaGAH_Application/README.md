# GouGouGaGAH_Application : programme final de l'assistant domotique

Le programme de **test** des composants reste dans `../GouGouGaGAH_DomotiqueESP32`.
Ce dossier-ci contient le **programme final** (projet PlatformIO séparé).

## Avant de flasher

1. Copie `include/secrets_exemple.h` en `include/secrets.h` (même dossier).
2. Remplis `secrets.h` : WiFi (2,4 GHz, le même que le Raspberry Pi)
   et le compte `esp32` créé dans Home Assistant. Ce fichier n'est jamais envoyé sur GitHub.
   L'adresse IP du Pi est dans `include/config_reseau.h` (actuellement `10.252.23.42`,
   l'ancienne `192.168.1.110` est gardée en commentaire).
3. Dans VS Code, ouvre **ce dossier** dans PlatformIO, puis Build et Upload.

## Ce que fait le programme

| Fonction | Où |
|---|---|
| Lumière, température, pression, gaz envoyés à Home Assistant toutes les 10 s (MQTT) | `module_ha` |
| Badge autorisé : la porte s'ouvre (relais collé 3 s). Badges mémorisés en NVS | `module_acces` |
| Écran : Capteurs, Porte (bouton OUVRIR), Badges (Ajouter / Suppr. / Tout effacer), Éclairage, Infos | `module_menu` |
| Sortie PWM 0-100 % (GPIO2), réglable depuis l'écran ou Home Assistant | `module_pwm` |
| LED RGB (GPIO8) : couleur Tempo du jour (longue) puis du lendemain (courte), clignote en continu | `module_tempo` |

Couleurs de la LED : bleu, blanc, rouge ; orange = couleur pas encore connue (pas de WiFi ou pas encore publiée).

## Dans Home Assistant

L'appareil **GouGouGaGAH** apparaît tout seul (Paramètres > Appareils et services > MQTT) avec :
Luminosité, Température, Pression, Gaz, Badges autorisés, Tempo aujourd'hui / demain,
Dernier badge, Dernier accès, Porte (ouverte/fermée), bouton **Ouvrir la porte**, curseur **Éclairage**.

## Messages MQTT (pour le dossier)

| Topic | Sens | Contenu |
|---|---|---|
| `gougougagah/etat` | ESP32 → HA | JSON des mesures + Tempo + nombre de badges |
| `gougougagah/porte` | ESP32 → HA | `ON` (ouverte) / `OFF` |
| `gougougagah/porte/ouvrir` | HA → ESP32 | ordre d'ouverture |
| `gougougagah/pwm` / `gougougagah/pwm/set` | les deux | 0 à 100 |
| `gougougagah/badge` | ESP32 → HA | `{"uid":"...","resultat":"autorise/refuse/ajoute/..."}` |
| `gougougagah/dispo` | ESP32 → HA | `online` / `offline` |
