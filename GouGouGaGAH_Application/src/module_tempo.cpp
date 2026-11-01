#include "module_tempo.h"
#include "pinout.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

// API publique gratuite (données RTE) : renvoie {"codeJour":1,...} avec 1 bleu, 2 blanc, 3 rouge
#define URL_TEMPO "https://www.api-couleur-tempo.fr/api/jourTempo/"

static const unsigned long PERIODE_MAJ = 3600000UL;   // 1 h
static const unsigned long PERIODE_ESSAI = 300000UL;  // 5 min si la dernière requête a échoué
static const unsigned long CYCLE_LED = 2000;          // ms

static CouleurTempo aujourdhui = TEMPO_INCONNU;
static CouleurTempo demain = TEMPO_INCONNU;
static unsigned long derniereMaj = 0;
static bool dejaMaj = false;
static int phaseAffichee = -1;

// Lit /today ou /tomorrow ; TEMPO_INCONNU si la requête échoue
static CouleurTempo lireJour(const char *jour) {
  WiFiClientSecure client;
  client.setInsecure(); // pas de vérification du certificat : suffisant pour une couleur Tempo
  HTTPClient http;
  http.setTimeout(4000);
  if (!http.begin(client, String(URL_TEMPO) + jour)) return TEMPO_INCONNU;

  CouleurTempo c = TEMPO_INCONNU;
  if (http.GET() == HTTP_CODE_OK) {
    String json = http.getString();
    int pos = json.indexOf("\"codeJour\":");
    if (pos >= 0) {
      int code = json.substring(pos + 11).toInt();
      if (code >= 1 && code <= 3) c = (CouleurTempo)code;
    }
  }
  http.end();
  return c;
}

static void mettreAJour() {
  aujourdhui = lireJour("today");
  demain = lireJour("tomorrow");
  Serial.printf("[Tempo] aujourd'hui %s, demain %s\n", tempo_nom(aujourdhui), tempo_nom(demain));
}

static void allumer(CouleurTempo c) {
  switch (c) {
    case TEMPO_BLEU:  rgbLedWrite(LED_RGB, 0, 0, 60); break;
    case TEMPO_BLANC: rgbLedWrite(LED_RGB, 40, 40, 40); break;
    case TEMPO_ROUGE: rgbLedWrite(LED_RGB, 60, 0, 0); break;
    default:          rgbLedWrite(LED_RGB, 30, 15, 0); break; // orange = couleur pas encore connue
  }
}

// Sur un cycle de 2 s : couleur du jour (longue), pause, couleur du lendemain (courte), pause
static void clignoter() {
  unsigned long t = millis() % CYCLE_LED;
  int phase = t < 700 ? 0 : t < 1000 ? 1 : t < 1300 ? 2 : 3;
  if (phase == phaseAffichee) return; // on n'écrit dans la LED qu'aux changements
  phaseAffichee = phase;
  if (phase == 0) allumer(aujourdhui);
  else if (phase == 2) allumer(demain);
  else rgbLedWrite(LED_RGB, 0, 0, 0);
}

void tempo_setup() {
  rgbLedWrite(LED_RGB, 0, 0, 0);
  Serial.println(F("[Tempo] LED RGB prete"));
}

void tempo_loop() {
  clignoter();
  if (WiFi.status() != WL_CONNECTED) return;

  bool connu = aujourdhui != TEMPO_INCONNU;
  unsigned long attente = connu ? PERIODE_MAJ : PERIODE_ESSAI;
  if (!dejaMaj || millis() - derniereMaj > attente) {
    dejaMaj = true;
    derniereMaj = millis();
    mettreAJour();
  }
}

CouleurTempo tempo_aujourdhui() {
  return aujourdhui;
}

CouleurTempo tempo_demain() {
  return demain;
}

const char *tempo_nom(CouleurTempo c) {
  switch (c) {
    case TEMPO_BLEU:  return "bleu";
    case TEMPO_BLANC: return "blanc";
    case TEMPO_ROUGE: return "rouge";
    default:          return "inconnu";
  }
}
