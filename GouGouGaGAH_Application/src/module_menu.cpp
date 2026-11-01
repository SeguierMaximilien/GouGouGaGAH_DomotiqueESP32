#include "module_menu.h"
#include "module_tft.h"
#include "module_bh1750.h"
#include "module_bmp280.h"
#include "module_mq.h"
#include "module_acces.h"
#include "module_pwm.h"
#include "module_tempo.h"
#include "module_ha.h"
#include <WiFi.h>

// Pas d'accents à l'écran : la police par défaut d'Adafruit_GFX ne les affiche pas.

enum Ecran { MENU, CAPTEURS, PORTE, BADGES, ECLAIRAGE, INFOS, CALIBRER };

struct Bouton {
  int x, y, w, h;
  const char *texte;
  uint16_t couleur;
};

static const int NB_BOUTONS = 6;
static Bouton boutons[NB_BOUTONS];
static const Ecran cibles[NB_BOUTONS] = { CAPTEURS, PORTE, BADGES, ECLAIRAGE, INFOS, CALIBRER };
static Bouton retour;
static Bouton btnOuvrir;                               // écran Porte
static Bouton btnAjouter, btnSupprimer, btnEffacer;    // écran Badges
static Bouton btnMoins, btnPlus;                       // écran Eclairage

static Ecran ecranActuel = MENU;
static unsigned long dernierRafraichissement = 0;
static bool doigtPose = false;          // pour ne réagir qu'au moment où on pose le doigt
static unsigned long dernierContact = 0; // anti-rebond : relâché seulement après 150 ms sans contact
static unsigned long effacerArme = 0;   // "Tout effacer" demande un 2e appui dans les 3 s

// ---------------------------------------------------------------- dessin ----

static void dessinerBouton(const Bouton &b) {
  Adafruit_ILI9341 &tft = tft_get();
  tft.fillRoundRect(b.x, b.y, b.w, b.h, 8, b.couleur);
  tft.drawRoundRect(b.x, b.y, b.w, b.h, 8, ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE);
  int largeurTexte = strlen(b.texte) * 12; // 6 px par caractère x taille 2
  tft.setCursor(b.x + (b.w - largeurTexte) / 2, b.y + (b.h - 16) / 2);
  tft.print(b.texte);
}

static bool estDans(const Bouton &b, int x, int y) {
  return x >= b.x && x < b.x + b.w && y >= b.y && y < b.y + b.h;
}

static void dessinerTitre(const char *titre) {
  Adafruit_ILI9341 &tft = tft_get();
  tft.fillScreen(ILI9341_BLACK);
  tft.fillRect(0, 0, tft.width(), 40, ILI9341_NAVY);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(retour.x + retour.w + 10, 12);
  tft.print(titre);
  dessinerBouton(retour);
}

// Efface une zone puis y écrit une valeur (évite le scintillement d'un fillScreen).
static void ecrireValeur(int y, const String &texte, uint8_t taille, uint16_t couleur) {
  Adafruit_ILI9341 &tft = tft_get();
  tft.fillRect(0, y, tft.width(), 8 * taille + 4, ILI9341_BLACK);
  tft.setTextSize(taille);
  tft.setTextColor(couleur);
  tft.setCursor(10, y);
  tft.print(texte);
}

static uint16_t couleurTempo(CouleurTempo c) {
  switch (c) {
    case TEMPO_BLEU:  return ILI9341_BLUE;
    case TEMPO_BLANC: return ILI9341_WHITE;
    case TEMPO_ROUGE: return ILI9341_RED;
    default:          return ILI9341_DARKGREY;
  }
}

static void afficherMenu() {
  Adafruit_ILI9341 &tft = tft_get();
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_GREEN);
  tft.setCursor(10, 12);
  tft.print("Assistant domotique");
  for (int i = 0; i < NB_BOUTONS; i++) dessinerBouton(boutons[i]);
}

static void afficherEcran(Ecran e) {
  ecranActuel = e;
  dernierRafraichissement = 0; // force un rafraîchissement immédiat

  switch (e) {
    case MENU:
      afficherMenu();
      break;
    case CAPTEURS:
      dessinerTitre("Capteurs");
      break;
    case PORTE:
      dessinerTitre("Porte");
      dessinerBouton(btnOuvrir);
      break;
    case BADGES:
      dessinerTitre("Badges");
      dessinerBouton(btnAjouter);
      dessinerBouton(btnSupprimer);
      dessinerBouton(btnEffacer);
      break;
    case ECLAIRAGE:
      dessinerTitre("Eclairage (PWM)");
      dessinerBouton(btnMoins);
      dessinerBouton(btnPlus);
      break;
    case INFOS:
      dessinerTitre("Infos");
      break;
    case CALIBRER:
      tft_calibrerTactile();
      doigtPose = true; // le stylet vient d'être levé : pas de clic fantôme
      afficherEcran(MENU);
      break;
  }
}

// --------------------------------------------------- mise à jour des écrans ----

static String texteLux() {
  float lux = bh1750_readLux();
  return lux < 0 ? String("non detecte") : String(lux, 1) + " lux";
}

static String texteTemperature() {
  float t = bmp280_readTemperature();
  return t < -900 ? String("non detecte") : String(t, 1) + " C";
}

static String textePression() {
  float p = bmp280_readPressure();
  return p < 0 ? String("non detecte") : String(p, 1) + " hPa";
}

static void dessinerJauge(int pourcent) {
  Adafruit_ILI9341 &tft = tft_get();
  int x = 20, y = 150, w = tft.width() - 40, h = 30;
  tft.drawRect(x, y, w, h, ILI9341_WHITE);
  int plein = (w - 4) * pourcent / 100;
  tft.fillRect(x + 2, y + 2, plein, h - 4, ILI9341_YELLOW);
  tft.fillRect(x + 2 + plein, y + 2, w - 4 - plein, h - 4, ILI9341_BLACK);
}

static void rafraichir() {
  switch (ecranActuel) {
    case CAPTEURS:
      ecrireValeur(55,  "Lumiere : " + texteLux(), 2, ILI9341_YELLOW);
      ecrireValeur(85,  "Temp.   : " + texteTemperature(), 2, ILI9341_ORANGE);
      ecrireValeur(115, "Pression: " + textePression(), 2, ILI9341_CYAN);
      ecrireValeur(145, "Gaz     : " + String(mq_readRaw()) + " / 4095", 2, ILI9341_GREEN);
      ecrireValeur(175, "Porte   : " + String(acces_porteOuverte() ? "ouverte" : "fermee"), 2, ILI9341_WHITE);
      ecrireValeur(205, "Eclair. : " + String(pwm_get()) + " %", 2, ILI9341_WHITE);
      break;
    case PORTE:
      ecrireValeur(160, acces_porteOuverte() ? "Porte OUVERTE" : "Porte fermee", 3,
                   acces_porteOuverte() ? ILI9341_GREEN : ILI9341_WHITE);
      ecrireValeur(205, acces_dernierMessage(), 1, ILI9341_LIGHTGREY);
      break;
    case BADGES: {
      ecrireValeur(55, String("Badges autorises : ") + acces_nbBadges(), 2, ILI9341_WHITE);
      String etat;
      uint16_t couleur = ILI9341_YELLOW;
      if (acces_mode() == ACCES_VALIDATION) etat = "Passe un badge autorise";
      else if (acces_mode() == ACCES_AJOUT) etat = "Passe le NOUVEAU badge";
      else if (acces_mode() == ACCES_SUPPRESSION) etat = "Passe le badge a enlever";
      else if (effacerArme && millis() - effacerArme < 3000) {
        etat = "Re-appuie : tout effacer";
        couleur = ILI9341_RED;
      } else {
        etat = acces_dernierMessage();
        couleur = ILI9341_LIGHTGREY;
      }
      ecrireValeur(150, etat, 2, couleur);
      break;
    }
    case ECLAIRAGE:
      ecrireValeur(110, String(pwm_get()) + " %", 3, ILI9341_YELLOW);
      dessinerJauge(pwm_get());
      break;
    case INFOS:
      ecrireValeur(50, ha_wifiConnecte() ? "WiFi : " + WiFi.localIP().toString() : String("WiFi : connexion..."),
                   2, ha_wifiConnecte() ? ILI9341_GREEN : ILI9341_ORANGE);
      ecrireValeur(80, ha_estConnecte() ? "Home Assistant : OK" : "Home Assistant : --",
                   2, ha_estConnecte() ? ILI9341_GREEN : ILI9341_ORANGE);
      ecrireValeur(120, String("Tempo jour  : ") + tempo_nom(tempo_aujourdhui()), 2,
                   couleurTempo(tempo_aujourdhui()));
      ecrireValeur(150, String("Tempo demain: ") + tempo_nom(tempo_demain()), 2,
                   couleurTempo(tempo_demain()));
      break;
    default:
      break;
  }
}

// Un appui (doigt qui vient de se poser) sur l'écran en cours
static void toucher(int x, int y) {
  if (ecranActuel == MENU) {
    for (int i = 0; i < NB_BOUTONS; i++) {
      if (estDans(boutons[i], x, y)) { afficherEcran(cibles[i]); return; }
    }
    return;
  }
  if (y < 40) { // toute la barre de titre sert de bouton retour
    acces_annulerMode();
    afficherEcran(MENU);
    return;
  }

  switch (ecranActuel) {
    case PORTE:
      if (estDans(btnOuvrir, x, y)) acces_ouvrirPorte();
      break;
    case BADGES:
      if (estDans(btnAjouter, x, y)) acces_demanderAjout();
      else if (estDans(btnSupprimer, x, y)) acces_demanderSuppression();
      else if (estDans(btnEffacer, x, y)) {
        if (effacerArme && millis() - effacerArme < 3000) {
          acces_toutEffacer();
          effacerArme = 0;
        } else {
          effacerArme = millis();
        }
      }
      break;
    case ECLAIRAGE:
      if (estDans(btnMoins, x, y)) pwm_set(pwm_get() - 10);
      else if (estDans(btnPlus, x, y)) pwm_set(pwm_get() + 10);
      break;
    default:
      break;
  }
  dernierRafraichissement = 0;
}

// ------------------------------------------------------------------ API ----

void menu_setup() {
  Adafruit_ILI9341 &tft = tft_get();
  const int W = tft.width(), marge = 8, haut = 40;

  // Menu : grille de 3 x 2 boutons
  int w = (W - 4 * marge) / 3;
  int h = (tft.height() - haut - 3 * marge) / 2;
  const char *textes[NB_BOUTONS] = { "Capteurs", "Porte", "Badges", "Eclair.", "Infos", "Calib." };
  uint16_t couleurs[NB_BOUTONS] = { ILI9341_DARKGREEN, ILI9341_ORANGE, ILI9341_PURPLE,
                                    ILI9341_OLIVE, ILI9341_NAVY, ILI9341_DARKGREY };
  for (int i = 0; i < NB_BOUTONS; i++) {
    int col = i % 3, ligne = i / 3;
    boutons[i] = { marge + col * (w + marge), haut + marge + ligne * (h + marge), w, h,
                   textes[i], couleurs[i] };
  }
  retour = { 4, 4, 70, 32, "<", ILI9341_DARKGREY };

  btnOuvrir    = { W / 2 - 100, 55, 200, 90, "OUVRIR", ILI9341_DARKGREEN };
  int wb = (W - 4 * marge) / 3;
  btnAjouter   = { marge,              85, wb, 50, "Ajouter", ILI9341_DARKGREEN };
  btnSupprimer = { 2 * marge + wb,     85, wb, 50, "Suppr.", ILI9341_MAROON };
  btnEffacer   = { 3 * marge + 2 * wb, 85, wb, 50, "Tout", ILI9341_DARKGREY };
  btnMoins     = { 20,          55, 80, 50, "-10", ILI9341_DARKGREY };
  btnPlus      = { W - 100,     55, 80, 50, "+10", ILI9341_DARKGREY };

  // Sans calibration, les boutons ne tombent pas sous le stylet : on calibre d'abord
  if (!tft_estCalibre()) {
    while (!tft_calibrerTactile()) {}
  }
  afficherEcran(MENU);
}

void menu_loop() {
  int x, y;
  bool touche = tft_checkTouchPixel(x, y);
  if (touche && !doigtPose) toucher(x, y);
  if (touche) {
    doigtPose = true;
    dernierContact = millis();
  } else if (millis() - dernierContact > 150) {
    doigtPose = false;
  }

  if (ecranActuel != MENU && millis() - dernierRafraichissement > 500) {
    dernierRafraichissement = millis();
    rafraichir();
  }
}
