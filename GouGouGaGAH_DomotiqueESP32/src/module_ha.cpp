#include "module_ha.h"
#include "module_bh1750.h"
#include "module_bmp280.h"
#include "module_mq.h"
#include "module_relais.h"
#include <WiFi.h>
#include <PubSubClient.h>

#if __has_include("secrets.h")
#include "secrets.h"
#else
#warning "include/secrets.h manquant : copie secrets_exemple.h en secrets.h et remplis-le"
#include "secrets_exemple.h"
#endif

// Tous les messages de la carte commencent par ce préfixe
#define BASE            "gougougagah"
#define TOPIC_DISPO     BASE "/dispo"        // online / offline
#define TOPIC_ETAT      BASE "/etat"         // mesures en JSON
#define TOPIC_RELAIS    BASE "/relais"       // ON / OFF
#define TOPIC_RELAIS_SET BASE "/relais/set"  // ordre venant de Home Assistant
#define TOPIC_BADGE     BASE "/badge"        // dernier UID lu
#define TOPIC_HA_STATUS "homeassistant/status" // Home Assistant vient de (re)démarrer

static const unsigned long PERIODE_MESURES = 10000; // ms entre deux envois
static const unsigned long PERIODE_RECONNEXION = 10000;

static WiFiClient wifiClient;
static PubSubClient mqtt(wifiClient);
static unsigned long dernierEnvoi = 0;
static unsigned long derniereTentative = 0;
static bool wifiAnnonce = false;
static int relaisPublie = -1; // -1 = jamais envoyé

// ------------------------------------------------------ découverte HA ----

// Bloc "appareil" commun : regroupe toutes les entités sous un seul appareil
static const char *APPAREIL =
  "\"dev\":{\"ids\":[\"" BASE "\"],\"name\":\"GouGouGaGAH\","
  "\"mf\":\"Witherwatch\",\"mdl\":\"ESP32-C6\"},"
  "\"avty_t\":\"" TOPIC_DISPO "\"";

static void annoncerCapteur(const char *id, const char *nom, const char *champ,
                            const char *classe, const char *unite, const char *icone) {
  String topic = String("homeassistant/sensor/" BASE "_") + id + "/config";
  String json = String("{\"name\":\"") + nom + "\","
                "\"uniq_id\":\"" BASE "_" + id + "\","
                "\"stat_t\":\"" TOPIC_ETAT "\","
                "\"val_tpl\":\"{{ value_json." + champ + " }}\","
                "\"stat_cla\":\"measurement\",";
  if (classe) json += String("\"dev_cla\":\"") + classe + "\",";
  if (unite)  json += String("\"unit_of_meas\":\"") + unite + "\",";
  if (icone)  json += String("\"ic\":\"") + icone + "\",";
  json += APPAREIL;
  json += "}";
  mqtt.publish(topic.c_str(), json.c_str(), true);
}

static void annoncerTout() {
  annoncerCapteur("lux", "Luminosite", "lux", "illuminance", "lx", nullptr);
  annoncerCapteur("temperature", "Temperature", "temperature", "temperature", "°C", nullptr);
  annoncerCapteur("pression", "Pression", "pression", "atmospheric_pressure", "hPa", nullptr);
  annoncerCapteur("gaz", "Gaz (brut)", "gaz", nullptr, nullptr, "mdi:molecule");

  String badge = String("{\"name\":\"Badge\",\"uniq_id\":\"" BASE "_badge\","
                        "\"stat_t\":\"" TOPIC_BADGE "\",\"ic\":\"mdi:card-account-details\",") +
                 APPAREIL + "}";
  mqtt.publish("homeassistant/sensor/" BASE "_badge/config", badge.c_str(), true);

  String relais = String("{\"name\":\"Relais\",\"uniq_id\":\"" BASE "_relais\","
                         "\"stat_t\":\"" TOPIC_RELAIS "\",\"cmd_t\":\"" TOPIC_RELAIS_SET "\","
                         "\"ic\":\"mdi:electric-switch\",") +
                  APPAREIL + "}";
  mqtt.publish("homeassistant/switch/" BASE "_relais/config", relais.c_str(), true);

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
                ",\"gaz\":" + mq_readRaw() + "}";
  mqtt.publish(TOPIC_ETAT, json.c_str());
}

static void envoyerRelais() {
  relaisPublie = relais_estActif() ? 1 : 0;
  mqtt.publish(TOPIC_RELAIS, relaisPublie ? "ON" : "OFF", true);
}

// ---------------------------------------------------------- réception ----

static void recevoir(char *topic, byte *payload, unsigned int len) {
  String msg;
  for (unsigned int i = 0; i < len; i++) msg += (char)payload[i];

  if (strcmp(topic, TOPIC_RELAIS_SET) == 0) {
    Serial.println("[HA] Ordre relais : " + msg);
    if (msg == "ON") relais_set(true);
    else if (msg == "OFF") relais_set(false);
    envoyerRelais(); // confirme tout de suite l'état réel
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
    // 4 = mauvais identifiants, -2 = broker injoignable (IP ? Mosquitto démarré ?)
    Serial.printf("ECHEC (code %d)\n", mqtt.state());
    return;
  }
  Serial.println(F("OK"));
  mqtt.publish(TOPIC_DISPO, "online", true);
  annoncerTout();
  mqtt.subscribe(TOPIC_RELAIS_SET);
  mqtt.subscribe(TOPIC_HA_STATUS);
  envoyerRelais();
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
  if (relaisPublie != (relais_estActif() ? 1 : 0)) envoyerRelais(); // basculé depuis l'écran
  if (millis() - dernierEnvoi > PERIODE_MESURES) {
    dernierEnvoi = millis();
    envoyerMesures();
  }
}

void ha_publierBadge(const String &uid) {
  if (mqtt.connected()) mqtt.publish(TOPIC_BADGE, uid.c_str(), true);
}

bool ha_estConnecte() {
  return mqtt.connected();
}
