#include "module_relais.h"
#include "pinout.h"

static bool relais_actif = false;

void relais_setup() {
  pinMode(RELAIS_PIN, OUTPUT);
  relais_set(false);
  Serial.println(F("[Relais] OK (coupe au demarrage)"));
}

void relais_set(bool actif) {
  relais_actif = actif;
  digitalWrite(RELAIS_PIN, actif ? HIGH : LOW);
  Serial.println(actif ? F("[Relais] ON") : F("[Relais] OFF"));
}

void relais_basculer() {
  relais_set(!relais_actif);
}

bool relais_estActif() {
  return relais_actif;
}
