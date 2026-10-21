#include "module_menu.h"
#include "module_tft.h"
#include "module_bh1750.h"
#include "module_bmp280.h"
#include "module_mq.h"
#include "module_rfid.h"
#include "module_relais.h"

// Pas d'accents à l'écran : la police par défaut d'Adafruit_GFX ne les affiche pas.

enum Ecran { MENU, LUMIERE, TEMPERATURE, GAZ, BADGE, TACTILE, TOUT, RELAIS };

struct Bouton {
  int x, y, w, h;
  const char *texte;
  uint16_t couleur;
  Ecran cible;
};

static const int NB_BOUTONS = 7;
static Bouton boutons[NB_BOUTONS];
static Bouton retour;
static Bouton boutonRelais; // gros bouton ON/OFF de l'écran Relais

static Ecran ecranActuel = MENU;
static unsigned long dernierRafraichissement = 0;
static bool doigtPose = false; // pour ne réagir qu'au moment où on pose le doigt
static String dernierBadge = "";

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

static void afficherMenu() {
  Adafruit_ILI9341 &tft = tft_get();
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_GREEN);
  tft.setCursor(10, 10);
  tft.print("Test des composants");
  for (int i = 0; i < NB_BOUTONS; i++) dessinerBouton(boutons[i]);
}

static void dessinerEtatRelais() {
  bool actif = relais_estActif();
  boutonRelais.texte = actif ? "ON" : "OFF";
  boutonRelais.couleur = actif ? ILI9341_DARKGREEN : ILI9341_MAROON;
  dessinerBouton(boutonRelais);
  ecrireValeur(boutonRelais.y + boutonRelais.h + 10, "Touche pour basculer", 2, ILI9341_WHITE);
}

static void afficherEcran(Ecran e) {
  ecranActuel = e;
  dernierRafraichissement = 0; // force un rafraîchissement immédiat
  Adafruit_ILI9341 &tft = tft_get();

  switch (e) {
    case MENU:        afficherMenu(); return;
    case LUMIERE:     dessinerTitre("Lumiere BH1750"); break;
    case TEMPERATURE: dessinerTitre("BMP280"); break;
    case GAZ:         dessinerTitre("Gaz MQ"); break;
    case BADGE:
      dessinerTitre("Badge RC522");
      ecrireValeur(60, "Approche un badge...", 2, ILI9341_WHITE);
      if (dernierBadge.length() > 0) ecrireValeur(110, dernierBadge, 3, ILI9341_YELLOW);
      break;
    case TACTILE:
      dessinerTitre("Tactile");
      tft.setTextSize(1);
      tft.setTextColor(ILI9341_LIGHTGREY);
      tft.setCursor(10, tft.height() - 12);
      tft.print("Touche les 4 coins et note les valeurs brutes");
      break;
    case TOUT:        dessinerTitre("Tous les capteurs"); break;
    case RELAIS:
      dessinerTitre("Relais");
      dessinerEtatRelais();
      break;
  }
  dessinerBouton(retour);
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

static String texteGaz() {
  return String(mq_readRaw()) + " / 4095";
}

static void rafraichir() {
  switch (ecranActuel) {
    case LUMIERE:
      ecrireValeur(80, texteLux(), 3, ILI9341_YELLOW);
      break;
    case TEMPERATURE:
      ecrireValeur(70, texteTemperature(), 3, ILI9341_ORANGE);
      ecrireValeur(120, textePression(), 3, ILI9341_CYAN);
      break;
    case GAZ:
      ecrireValeur(80, texteGaz(), 2, ILI9341_GREEN);
      break;
    case TOUT:
      ecrireValeur(55,  "Lumiere : " + texteLux(), 2, ILI9341_YELLOW);
      ecrireValeur(85,  "Temp.   : " + texteTemperature(), 2, ILI9341_ORANGE);
      ecrireValeur(115, "Pression: " + textePression(), 2, ILI9341_CYAN);
      ecrireValeur(145, "Gaz     : " + String(mq_readRaw()), 2, ILI9341_GREEN);
      ecrireValeur(175, "Badge   : " + (dernierBadge.length() ? dernierBadge : String("-")), 2, ILI9341_WHITE);
      ecrireValeur(205, String("Relais  : ") + (relais_estActif() ? "ON" : "OFF"), 2, ILI9341_WHITE);
      break;
    default:
      break;
  }
}

// Le tactile en écran "Tactile" : affiche les coordonnées et dessine un point.
static void testerTactile(int x, int y) {
  Adafruit_ILI9341 &tft = tft_get();
  int rx = -1, ry = -1;
  tft_checkTouch(rx, ry); // relit les valeurs brutes pour la calibration
  ecrireValeur(50, String("Brut  x=") + rx + " y=" + ry, 2, ILI9341_WHITE);
  ecrireValeur(75, String("Pixel x=") + x + " y=" + y, 2, ILI9341_GREEN);
  if (y > 100) tft.fillCircle(x, y, 4, ILI9341_RED); // pas dans la zone de texte
}

// ------------------------------------------------------------------ API ----

void menu_setup() {
  Adafruit_ILI9341 &tft = tft_get();
  // Grille de 3 x 2 boutons qui s'adapte à la taille (paysage 320x240 ou portrait 240x320)
  const int marge = 8, haut = 40;
  int w = (tft.width() - 4 * marge) / 3;
  int h = (tft.height() - haut - 4 * marge) / 3; // 3 lignes de 3 boutons
  const char *textes[NB_BOUTONS] = { "Lumiere", "Temp", "Gaz", "Badge", "Tactile", "Tout", "Relais" };
  uint16_t couleurs[NB_BOUTONS] = { ILI9341_OLIVE, ILI9341_MAROON, ILI9341_DARKGREEN,
                                    ILI9341_PURPLE, ILI9341_DARKCYAN, ILI9341_NAVY,
                                    ILI9341_ORANGE };
  Ecran cibles[NB_BOUTONS] = { LUMIERE, TEMPERATURE, GAZ, BADGE, TACTILE, TOUT, RELAIS };
  for (int i = 0; i < NB_BOUTONS; i++) {
    int col = i % 3, ligne = i / 3;
    boutons[i] = { marge + col * (w + marge), haut + marge + ligne * (h + marge), w, h,
                   textes[i], couleurs[i], cibles[i] };
  }
  retour = { 4, 4, 70, 32, "<", ILI9341_DARKGREY, MENU };
  boutonRelais = { tft.width() / 2 - 80, 70, 160, 90, "OFF", ILI9341_MAROON, RELAIS };

  afficherEcran(MENU);
}

void menu_loop() {
  // Lecture de badge en permanence, pour ne rater aucun passage
  String uid;
  if (rfid_checkCard(uid)) {
    dernierBadge = uid;
    Serial.println("[RC522] Carte : " + uid);
    if (ecranActuel == BADGE) ecrireValeur(110, uid, 3, ILI9341_YELLOW);
  }

  int x, y;
  bool touche = tft_checkTouchPixel(x, y);
  if (touche && ecranActuel == TACTILE) {
    testerTactile(x, y);
  }
  if (touche && !doigtPose) {
    // On vient de poser le doigt : on l'affiche sur le moniteur série pour la calibration
    int rx = -1, ry = -1;
    tft_checkTouch(rx, ry);
    Serial.printf("[Tactile] brut x=%d y=%d -> pixel x=%d y=%d\n", rx, ry, x, y);
    if (ecranActuel == MENU) {
      for (int i = 0; i < NB_BOUTONS; i++) {
        if (estDans(boutons[i], x, y)) { afficherEcran(boutons[i].cible); break; }
      }
    } else if (y < 40) {
      // Toute la barre de titre sert de bouton retour (plus facile à toucher
      // tant que le tactile n'est pas calibré)
      afficherEcran(MENU);
    } else if (ecranActuel == RELAIS && estDans(boutonRelais, x, y)) {
      relais_basculer();
      dessinerEtatRelais();
    }
  }
  doigtPose = touche;

  if (ecranActuel != MENU && millis() - dernierRafraichissement > 500) {
    dernierRafraichissement = millis();
    rafraichir();
  }
}
