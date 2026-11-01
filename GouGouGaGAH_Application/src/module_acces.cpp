#include "module_acces.h"
#include "module_rfid.h"
#include "module_relais.h"
#include "module_ha.h"
#include <Preferences.h>

static const int MAX_BADGES = 20;
static const unsigned long DUREE_OUVERTURE = 3000; // ms de relais collé
static const unsigned long DUREE_MODE = 15000;     // ms pour présenter le badge à ajouter/supprimer

static Preferences nvs;
static String badges[MAX_BADGES];
static int nbBadges = 0;

static ModeAcces mode = ACCES_NORMAL;
static unsigned long debutMode = 0;
static unsigned long debutOuverture = 0;
static bool porteOuverte = false;
static String dernierMessage = "-";

// ------------------------------------------------------------- NVS ----

// Les badges sont rangés dans une seule chaîne : "04 A3 F1 2B;DE AD BE EF;..."
static void sauvegarder() {
  String liste;
  for (int i = 0; i < nbBadges; i++) {
    if (i) liste += ";";
    liste += badges[i];
  }
  nvs.putString("liste", liste);
}

static void charger() {
  String liste = nvs.getString("liste", "");
  nbBadges = 0;
  int debut = 0;
  while (debut < (int)liste.length() && nbBadges < MAX_BADGES) {
    int fin = liste.indexOf(';', debut);
    if (fin < 0) fin = liste.length();
    badges[nbBadges++] = liste.substring(debut, fin);
    debut = fin + 1;
  }
}

static int chercher(const String &uid) {
  for (int i = 0; i < nbBadges; i++) {
    if (badges[i] == uid) return i;
  }
  return -1;
}

// -------------------------------------------------------- traitement ----

static void signaler(const String &uid, const String &resultat) {
  dernierMessage = uid + " : " + resultat;
  Serial.println("[Acces] " + dernierMessage);
  ha_publierBadge(uid, resultat);
}

static void traiterBadge(const String &uid) {
  int pos = chercher(uid);
  switch (mode) {
    case ACCES_NORMAL:
      if (pos >= 0) {
        signaler(uid, "autorise");
        acces_ouvrirPorte();
      } else {
        signaler(uid, "refuse");
      }
      break;
    case ACCES_AJOUT:
      if (pos >= 0) signaler(uid, "deja connu");
      else if (nbBadges >= MAX_BADGES) signaler(uid, "liste pleine");
      else {
        badges[nbBadges++] = uid;
        sauvegarder();
        signaler(uid, "ajoute");
      }
      mode = ACCES_NORMAL;
      break;
    case ACCES_SUPPRESSION:
      if (pos < 0) signaler(uid, "inconnu");
      else {
        badges[pos] = badges[--nbBadges]; // le dernier prend sa place
        sauvegarder();
        signaler(uid, "supprime");
      }
      mode = ACCES_NORMAL;
      break;
  }
}

// ------------------------------------------------------------- public ----

void acces_setup() {
  nvs.begin("badges", false);
  charger();
  Serial.printf("[Acces] %d badge(s) autorise(s) en memoire\n", nbBadges);
}

void acces_loop() {
  String uid;
  if (rfid_checkCard(uid)) traiterBadge(uid);

  if (mode != ACCES_NORMAL && millis() - debutMode > DUREE_MODE) {
    mode = ACCES_NORMAL;
    dernierMessage = "Temps depasse";
  }
  if (porteOuverte && millis() - debutOuverture > DUREE_OUVERTURE) {
    porteOuverte = false;
    relais_set(false);
  }
}

void acces_ouvrirPorte() {
  porteOuverte = true;
  debutOuverture = millis();
  relais_set(true);
}

bool acces_porteOuverte() {
  return porteOuverte;
}

void acces_demanderAjout() {
  mode = ACCES_AJOUT;
  debutMode = millis();
}

void acces_demanderSuppression() {
  mode = ACCES_SUPPRESSION;
  debutMode = millis();
}

void acces_annulerMode() {
  mode = ACCES_NORMAL;
}

ModeAcces acces_mode() {
  return mode;
}

void acces_toutEffacer() {
  nbBadges = 0;
  sauvegarder();
  dernierMessage = "Tous les badges effaces";
}

int acces_nbBadges() {
  return nbBadges;
}

String acces_dernierMessage() {
  return dernierMessage;
}
