#include "module_ha.h"
#include "module_bh1750.h"
#include "module_bmp280.h"
#include "module_mq.h"
#include "module_acces.h"
#include "module_pwm.h"
#include "module_tempo.h"
#include <WiFi.h>
#include <PubSubClient.h>

#if __has_include("secrets.h")
#include "secrets.h"
#else
#warning "include/secrets.h manquant : copie secrets_exemple.h en secrets.h et remplis-le"
#include "secrets_exemple.h"
#endif

// Tous les messages de la carte commencent par ce préfixe
#define BASE             "gougougagah"
#define TOPIC_DISPO      BASE "/dispo"          // online / offline
#define TOPIC_ETAT       BASE "/etat"           // mesures en JSON
#define TOPIC_PORTE      BASE "/porte"          // ON (ouverte) / OFF
#define TOPIC_PORTE_CMD  BASE "/porte/ouvrir"   // ordre d'ouverture venant de Home Assistant
#define TOPIC_PWM        BASE "/pwm"            // 0..100 %
#define TOPIC_PWM_CMD    BASE "/pwm/set"
#define TOPIC_BADGE      BASE "/badge"          // {"uid":...,"resultat":...}
#define TOPIC_HA_STATUS  "homeassistant/status" // Home Assistant vient de (re)démarrer

static const unsigned long PERIODE_MESURES = 10000; // ms entre deux envois
static const unsigned long PERIODE_RECONNEXION = 10000;

static WiFiClient wifiClient;
static PubSubClient mqtt(wifiClient);
static unsigned long dernierEnvoi = 0;
static unsigned long derniereTentative = 0;
static bool wifiAnnonce = false;
static int portePubliee = -1; // -1 = jamais envoyé
static int pwmPublie = -1;

// ------------------------------------------------------ découverte HA ----

// Bloc "appareil" commun : regroupe toutes les entités sous un seul appareil
static const char *APPAREIL =
  "\"dev\":{\"ids\":[\"" BASE "\"],\"name\":\"GouGouGaGAH\","
  "\"mf\":\"Witherwatch\",\"mdl\":\"ESP32-C6\"},"
  "\"avty_t\":\"" TOPIC_DISPO "\"";

static void annoncer(const char *type, const char *id, const String &champs) {
  String topic = String("homeassistant/") + type + "/" BASE "_" + id + "/config";
  String json = "{\"uniq_id\":\"" BASE "_" + String(id) + "\"," + champs + "," + APPAREIL + "}";
  mqtt.publish(topic.c_str(), json.c_str(), true);
}

// Capteur lu dans un champ JSON. mesure = true pour une valeur numérique (courbes + statistiques)
static void annoncerCapteur(const char *id, const char *nom, const char *topic, const char *champ,
                            bool mesure, const char *classe, const char *unite, const char *icone) {
  String c = String("\"name\":\"") + nom + "\",\"stat_t\":\"" + topic + "\","
             "\"val_tpl\":\"{{ value_json." + champ + " }}\"";
  if (mesure) c += ",\"stat_cla\":\"measurement\"";
  if (classe) c += String(",\"dev_cla\":\"") + classe + "\"";
  if (unite)  c += String(",\"unit_of_meas\":\"") + unite + "\"";
  if (icone)  c += String(",\"ic\":\"") + icone + "\"";
  annoncer("sensor", id, c);
}

static void annoncerTout() {
  annoncerCapteur("lux", "Luminosite", TOPIC_ETAT, "lux", true, "illuminance", "lx", nullptr);
  annoncerCapteur("temperature", "Temperature", TOPIC_ETAT, "temperature", true, "temperature", "°C", nullptr);
  annoncerCapteur("pression", "Pression", TOPIC_ETAT, "pression", true, "atmospheric_pressure", "hPa", nullptr);
  annoncerCapteur("gaz", "Gaz (brut)", TOPIC_ETAT, "gaz", true, nullptr, nullptr, "mdi:molecule");
  annoncerCapteur("badges", "Badges autorises", TOPIC_ETAT, "badges", true, nullptr, nullptr, "mdi:card-multiple");
  annoncerCapteur("tempo_jour", "Tempo aujourd'hui", TOPIC_ETAT, "tempo_jour", false, nullptr, nullptr, "mdi:calendar-today");
  annoncerCapteur("tempo_demain", "Tempo demain", TOPIC_ETAT, "tempo_demain", false, nullptr, nullptr, "mdi:calendar-arrow-right");
  annoncerCapteur("badge", "Dernier badge", TOPIC_BADGE, "uid", false, nullptr, nullptr, "mdi:card-account-details");
  annoncerCapteur("acces", "Dernier acces", TOPIC_BADGE, "resultat", false, nullptr, nullptr, "mdi:shield-key");

  annoncer("binary_sensor", "porte",
           "\"name\":\"Porte\",\"stat_t\":\"" TOPIC_PORTE "\",\"dev_cla\":\"door\"");
  annoncer("button", "ouvrir",
           "\"name\":\"Ouvrir la porte\",\"cmd_t\":\"" TOPIC_PORTE_CMD "\",\"ic\":\"mdi:door-open\"");
  annoncer("number", "pwm",
           "\"name\":\"Eclairage\",\"cmd_t\":\"" TOPIC_PWM_CMD "\",\"stat_t\":\"" TOPIC_PWM "\","
           "\"min\":0,\"max\":100,\"step\":5,\"unit_of_meas\":\"%\",\"mode\":\"slider\","
           "\"ic\":\"mdi:lightbulb-on-outline\"");

  Serial.println(F("[HA] Entites annoncees a Home Assistant"));
}

// ------------------------------------------------------------ envois ----

// Une valeur invalide (capteur absent) part en "null" : Home Assistant affiche "inconnu"
static String valeur(float v, bool valide, int decimales) {
  return valide ? String(v, decimales) : String("null");
}

static void envoyerMesures() {
  float lux = bh1750_readLux();
  float t = bmp280_readTemperature();
  float p = bmp280_readPressure();
  String json = String("{\"lux\":") + valeur(lux, lux >= 0, 1) +
                ",\"temperature\":" + valeur(t, t > -900, 1) +
                ",\"pression\":" + valeur(p, p >= 0, 1) +
                ",\"gaz\":" + mq_readRaw() +
                ",\"badges\":" + acces_nbBadges() +
                ",\"tempo_jour\":\"" + tempo_nom(tempo_aujourdhui()) + "\"" +
                ",\"tempo_demain\":\"" + tempo_nom(tempo_demain()) + "\"}";
  mqtt.publish(TOPIC_ETAT, json.c_str());
}

static void envoyerPorte() {
  portePubliee = acces_porteOuverte() ? 1 : 0;
  mqtt.publish(TOPIC_PORTE, portePubliee ? "ON" : "OFF", true);
}

static void envoyerPwm() {
  pwmPublie = pwm_get();
  mqtt.publish(TOPIC_PWM, String(pwmPublie).c_str(), true);
}

// ---------------------------------------------------------- réception ----

static void recevoir(char *topic, byte *payload, unsigned int len) {
  String msg;
  for (unsigned int i = 0; i < len; i++) msg += (char)payload[i];

  if (strcmp(topic, TOPIC_PORTE_CMD) == 0) {
    Serial.println(F("[HA] Ordre : ouvrir la porte"));
    acces_ouvrirPorte();
  } else if (strcmp(topic, TOPIC_PWM_CMD) == 0) {
    Serial.println("[HA] Eclairage : " + msg + " %");
    pwm_set(msg.toInt());
  } else if (strcmp(topic, TOPIC_HA_STATUS) == 0 && msg == "online") {
    annoncerTout(); // Home Assistant a redémarré : on se présente de nouveau
  }
}

// ----------------------------------------------------------- connexion ----

static void connecterMqtt() {
  Serial.printf("[HA] Connexion MQTT a %s:%d... ", MQTT_SERVEUR, MQTT_PORT);
  String id = String(BASE "-") + String((uint32_t)ESP.getEfuseMac(), HEX);
  // Testament : si la carte disparaît, le broker publie "offline" à sa place
  if (!mqtt.connect(id.c_str(), MQTT_UTILISATEUR, MQTT_MOTDEPASSE,
                    TOPIC_DISPO, 0, true, "offline")) {
    // 4/5 = mauvais identifiants, -2 = broker injoignable (IP ? Mosquitto démarré ?)
    Serial.printf("ECHEC (code %d)\n", mqtt.state());
    return;
  }
  Serial.println(F("OK"));
  mqtt.publish(TOPIC_DISPO, "online", true);
  annoncerTout();
  mqtt.subscribe(TOPIC_PORTE_CMD);
  mqtt.subscribe(TOPIC_PWM_CMD);
  mqtt.subscribe(TOPIC_HA_STATUS);
  envoyerPorte();
  envoyerPwm();
  envoyerMesures();
  dernierEnvoi = millis();
}

// ------------------------------------------------------------- public ----

void ha_setup() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_MOTDEPASSE);
  Serial.printf("[WiFi] Connexion a \"%s\"...\n", WIFI_SSID);

  mqtt.setServer(MQTT_SERVEUR, MQTT_PORT);
  mqtt.setCallback(recevoir);
  mqtt.setBufferSize(1024); // les messages de découverte dépassent les 256 octets par défaut
  mqtt.setSocketTimeout(3); // évite de figer l'écran trop longtemps si le Pi ne répond pas
}

void ha_loop() {
  if (WiFi.status() != WL_CONNECTED) {
    if (wifiAnnonce) Serial.println(F("[WiFi] Connexion perdue"));
    wifiAnnonce = false;
    return;
  }
  if (!wifiAnnonce) {
    wifiAnnonce = true;
    Serial.print(F("[WiFi] OK, adresse IP "));
    Serial.println(WiFi.localIP());
    derniereTentative = 0; // tente MQTT tout de suite
  }

  if (!mqtt.connected()) {
    if (derniereTentative == 0 || millis() - derniereTentative > PERIODE_RECONNEXION) {
      derniereTentative = millis();
      connecterMqtt();
    }
    return;
  }

  mqtt.loop();
  // Changements faits depuis l'écran ou par un badge : on prévient tout de suite
  if (portePubliee != (acces_porteOuverte() ? 1 : 0)) envoyerPorte();
  if (pwmPublie != pwm_get()) envoyerPwm();
  if (millis() - dernierEnvoi > PERIODE_MESURES) {
    dernierEnvoi = millis();
    envoyerMesures();
  }
}

void ha_publierBadge(const String &uid, const String &resultat) {
  if (!mqtt.connected()) return;
  String json = "{\"uid\":\"" + uid + "\",\"resultat\":\"" + resultat + "\"}";
  mqtt.publish(TOPIC_BADGE, json.c_str(), true);
}

bool ha_wifiConnecte() {
  return WiFi.status() == WL_CONNECTED;
}

bool ha_estConnecte() {
  return mqtt.connected();
}
