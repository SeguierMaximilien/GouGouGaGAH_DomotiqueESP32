#include "module_tft.h"
#include "pinout.h"
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>
#include <Preferences.h>
#include <algorithm>

// Ton module n'est pas un vrai ILI9341 : sa mémoire est organisée nativement en
// 320 x 240 (le grand côté sert de lignes), et il ignore l'échange lignes/colonnes
// utilisé d'habitude pour passer en paysage. On garde donc l'adressage "portrait"
// du contrôleur, mais en lui donnant une taille logique de 320 x 240.
class EcranClone : public Adafruit_ILI9341 {
public:
  using Adafruit_ILI9341::Adafruit_ILI9341;
  void forcerTaille(int16_t w, int16_t h) {
    _width = w;
    _height = h;
  }
};

static EcranClone tft(TFT_CS, TFT_DC, TFT_RST);
static XPT2046_Touchscreen touch(TOUCH_CS); // pas d'IRQ, lecture en polling

// ---- Calibration du tactile ----
// Valeurs brutes lues aux bords gauche/droite (X) et haut/bas (Y) de l'écran.
// Un MIN plus grand que le MAX veut dire que l'axe est inversé (map() le gère).
// "echange" : l'axe X de l'écran correspond à l'axe Y brut du tactile.
// Ces valeurs par défaut sont remplacées par celles de l'écran "Calibrer",
// enregistrées en mémoire flash (elles survivent au redémarrage et au reflash).
struct CalibTactile {
  int xMin, xMax, yMin, yMax;
  bool echange;
};
static CalibTactile calib = { 300, 3800, 300, 3800, false };
static bool calibre = false;

static const int TOUCH_PRESSION_MIN = 400; // en dessous, appui trop léger : ignoré

static void tft_chargerCalibration() {
  Preferences prefs;
  prefs.begin("tactile", true);
  calibre = prefs.isKey("xMin");
  if (calibre) {
    calib.xMin = prefs.getInt("xMin");
    calib.xMax = prefs.getInt("xMax");
    calib.yMin = prefs.getInt("yMin");
    calib.yMax = prefs.getInt("yMax");
    calib.echange = prefs.getBool("echange");
    Serial.printf("[Tactile] Calibration chargee : X %d..%d  Y %d..%d  echange=%d\n",
                  calib.xMin, calib.xMax, calib.yMin, calib.yMax, calib.echange);
  } else {
    Serial.println(F("[Tactile] Pas encore calibre : menu > Calibrer"));
  }
  prefs.end();
}

static void tft_enregistrerCalibration() {
  Preferences prefs;
  prefs.begin("tactile", false);
  prefs.putInt("xMin", calib.xMin);
  prefs.putInt("xMax", calib.xMax);
  prefs.putInt("yMin", calib.yMin);
  prefs.putInt("yMax", calib.yMax);
  prefs.putBool("echange", calib.echange);
  prefs.end();
}

// Lit un registre de l'écran (lecture SPI générique, valable pour ILI9341 et ST7789).
// Le premier octet renvoyé par l'écran est un octet "fantôme", d'où index + 1.
static uint8_t tft_lireRegistre(uint8_t registre, uint8_t index) {
  return tft.Adafruit_SPITFT::readcommand8(registre, index + 1);
}

// Affiche sur le moniteur série l'identifiant de l'écran et l'orientation réellement
// appliquée, pour savoir quel contrôleur se cache derrière le module.
static void tft_diagnostic() {
  Serial.print(F("[TFT] ID ecran (0x04) : "));
  for (uint8_t i = 0; i < 3; i++) {
    Serial.print(tft_lireRegistre(0x04, i), HEX);
    Serial.print(' ');
  }
  Serial.print(F(" | ID4 (0xD3) : "));
  for (uint8_t i = 0; i < 3; i++) {
    Serial.print(tft_lireRegistre(0xD3, i), HEX);
    Serial.print(' ');
  }
  Serial.print(F(" | MADCTL lu : 0x"));
  Serial.print(tft_lireRegistre(0x0B, 0), HEX);
  Serial.println();
}

// ---- Test des orientations au démarrage ----
// Chaque réglage s'affiche 5 s avec une grande lettre. Note la lettre où le cadre blanc
// fait tout le tour de l'écran, où la lettre se lit normalement (pas en miroir) et où
// les carrés R V B sont dans cet ordre, en bas. Mets ensuite TFT_TEST_ORIENTATIONS à 0
// et TFT_CONFIG à cette lettre.
#define TFT_TEST_ORIENTATIONS 0
#define TFT_CONFIG 'H' // choisi d'après le test : rendu correct sur ce module

struct ConfigEcran {
  char lettre;
  uint8_t madctl; // valeur envoyée au registre 0x36 (RGB, sans échange lignes/colonnes)
  const char *description;
};

static const ConfigEcran configs[] = {
  { 'F', 0x00, "320x240, normal" },
  { 'G', 0x40, "320x240, miroir gauche/droite" },
  { 'H', 0x80, "320x240, miroir haut/bas" },
  { 'I', 0xC0, "320x240, retourne (180 degres)" },
};
static const int NB_CONFIGS = sizeof(configs) / sizeof(configs[0]);

static void tft_appliquerConfig(const ConfigEcran &cfg) {
  tft.setRotation(0); // remet l'adressage de base du contrôleur
  uint8_t m = cfg.madctl;
  tft.sendCommand(0x36, &m, 1);
  tft.forcerTaille(320, 240);
  tft.fillScreen(ILI9341_BLACK); // efface toute la mémoire (320 x 240)
}

static void tft_dessinerMire(char lettre) {
  int w = tft.width(), h = tft.height();
  tft.fillScreen(ILI9341_BLACK);
  tft.drawRect(0, 0, w, h, ILI9341_WHITE);
  tft.drawRect(2, 2, w - 4, h - 4, ILI9341_WHITE);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(8);
  tft.setCursor(w / 2 - 24, 20);
  tft.print(lettre);
  // Trois carrés : R doit être rouge, V vert, B bleu
  const uint16_t couleurs[3] = { ILI9341_RED, ILI9341_GREEN, ILI9341_BLUE };
  const char lettres[3] = { 'R', 'V', 'B' };
  for (int i = 0; i < 3; i++) {
    int x = w / 2 - 75 + i * 52;
    tft.fillRect(x, h - 70, 46, 46, couleurs[i]);
    tft.setTextSize(3);
    tft.setTextColor(ILI9341_WHITE);
    tft.setCursor(x + 14, h - 58);
    tft.print(lettres[i]);
  }
}

static void tft_testOrientations() {
  for (int i = 0; i < NB_CONFIGS; i++) {
    tft_appliquerConfig(configs[i]);
    tft_dessinerMire(configs[i].lettre);
    Serial.printf("[TFT] Test %c : %s\n", configs[i].lettre, configs[i].description);
    delay(5000);
  }
}

void tft_setup() {
  // 20 MHz au lieu de 40 MHz par défaut : plus fiable avec des fils sur plaque à trous
  tft.begin(20000000);
  if (TFT_TEST_ORIENTATIONS) tft_testOrientations();
  for (int i = 0; i < NB_CONFIGS; i++) {
    if (configs[i].lettre == TFT_CONFIG) tft_appliquerConfig(configs[i]);
  }
  tft_diagnostic();
  tft.fillScreen(ILI9341_BLACK);
  touch.begin();
  touch.setRotation(0); // axes bruts : la calibration gère sens et échange des axes
  tft_chargerCalibration();
  Serial.println(F("[TFT] OK (verifier visuellement l'ecran)"));
  Serial.println(F("[Tactile] OK (touche l'ecran pour tester)"));
}

void tft_showTestScreen() {
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(10, 10);
  tft.setTextColor(ILI9341_GREEN);
  tft.setTextSize(2);
  tft.println("Diagnostic OK");

  tft.setTextSize(1);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 40);
  tft.println("Touche l'ecran pour");
  tft.setCursor(10, 55);
  tft.println("tester le tactile.");

  tft.fillRect(10, 100, 40, 30, ILI9341_RED);
  tft.fillRect(60, 100, 40, 30, ILI9341_GREEN);
  tft.fillRect(110, 100, 40, 30, ILI9341_BLUE);
  tft.drawRect(160, 100, 40, 30, ILI9341_YELLOW);
}

void tft_printLine(int x, int y, const String &text) {
  tft.fillRect(x, y, 220, 12, ILI9341_BLACK); // efface la ligne précédente
  tft.setCursor(x, y);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(1);
  tft.print(text);
}

// Le premier échantillon d'un appui est souvent faux (stylet pas encore bien posé) :
// on lit plusieurs points, on garde ceux assez appuyés et on prend la médiane.
bool tft_checkTouch(int &x, int &y) {
  if (!touch.touched()) return false;
  const int N = 7;
  int xs[N], ys[N], n = 0;
  for (int i = 0; i < N; i++) {
    TS_Point p = touch.getPoint();
    if (p.z >= TOUCH_PRESSION_MIN) {
      xs[n] = p.x;
      ys[n] = p.y;
      n++;
    }
    delay(2);
  }
  if (n < 4) return false; // appui trop léger ou instable
  std::sort(xs, xs + n);
  std::sort(ys, ys + n);
  x = xs[n / 2];
  y = ys[n / 2];
  return true;
}

bool tft_checkTouchPixel(int &x, int &y) {
  int rx, ry;
  if (!tft_checkTouch(rx, ry)) return false;
  int a = calib.echange ? ry : rx;
  int b = calib.echange ? rx : ry;
  x = constrain(map(a, calib.xMin, calib.xMax, 0, tft.width() - 1), 0, tft.width() - 1);
  y = constrain(map(b, calib.yMin, calib.yMax, 0, tft.height() - 1), 0, tft.height() - 1);
  return true;
}

// ---- Écran de calibration ----

static const int CALIB_MARGE = 20; // distance des croix aux bords

static void tft_dessinerCroix(int x, int y, uint16_t couleur) {
  tft.drawFastHLine(x - 10, y, 21, couleur);
  tft.drawFastVLine(x, y - 10, 21, couleur);
  tft.drawCircle(x, y, 5, couleur);
}

// Attend un appui franc, en fait la moyenne pendant ~300 ms, puis attend qu'on relâche.
static void tft_lireAppuiCalibration(int &rx, int &ry) {
  long sx, sy;
  int n;
  do {
    while (!touch.touched()) delay(10);
    delay(80); // laisse le stylet se poser
    sx = sy = n = 0;
    unsigned long debut = millis();
    while (millis() - debut < 300) {
      int x, y;
      if (tft_checkTouch(x, y)) { sx += x; sy += y; n++; }
    }
  } while (n < 5);
  rx = sx / n;
  ry = sy / n;
  unsigned long relache = millis();
  while (millis() - relache < 200) { // relâché depuis 200 ms
    if (touch.touched()) relache = millis();
    delay(10);
  }
}

bool tft_calibrerTactile() {
  const int w = tft.width(), h = tft.height(), m = CALIB_MARGE;
  // Ordre : haut-gauche, haut-droite, bas-droite, bas-gauche
  const int cx[4] = { m, w - 1 - m, w - 1 - m, m };
  const int cy[4] = { m, m, h - 1 - m, h - 1 - m };
  int rx[4], ry[4];

  for (int i = 0; i < 4; i++) {
    tft.fillScreen(ILI9341_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_WHITE);
    tft.setCursor(40, h / 2 - 20);
    tft.print("Appuie au centre");
    tft.setCursor(40, h / 2 + 4);
    tft.printf("de la croix (%d/4)", i + 1);
    tft_dessinerCroix(cx[i], cy[i], ILI9341_YELLOW);
    tft_lireAppuiCalibration(rx[i], ry[i]);
    Serial.printf("[Calib] point %d (%d,%d) : brut x=%d y=%d\n", i + 1, cx[i], cy[i], rx[i], ry[i]);
  }

  // Quel axe brut varie quand on va de gauche à droite ?
  int dxBrutX = abs((rx[1] + rx[2]) - (rx[0] + rx[3]));
  int dxBrutY = abs((ry[1] + ry[2]) - (ry[0] + ry[3]));
  bool echange = dxBrutY > dxBrutX;
  int a[4], b[4];
  for (int i = 0; i < 4; i++) {
    a[i] = echange ? ry[i] : rx[i];
    b[i] = echange ? rx[i] : ry[i];
  }
  // Moyenne des deux croix de chaque bord, puis extrapolation jusqu'au bord de l'écran
  float gauche = (a[0] + a[3]) / 2.0f, droite = (a[1] + a[2]) / 2.0f;
  float haut = (b[0] + b[1]) / 2.0f, bas = (b[2] + b[3]) / 2.0f;
  float parPixelX = (droite - gauche) / (w - 1 - 2 * m);
  float parPixelY = (bas - haut) / (h - 1 - 2 * m);

  bool ok = fabsf(droite - gauche) > 800 && fabsf(bas - haut) > 600;
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextSize(2);
  tft.setCursor(20, h / 2 - 10);
  if (!ok) {
    Serial.println(F("[Calib] ECHEC : points trop proches, recommence en appuyant bien sur les croix"));
    tft.setTextColor(ILI9341_RED);
    tft.print("Echec, recommence");
    delay(1500);
    return false;
  }
  calib.xMin = lroundf(gauche - m * parPixelX);
  calib.xMax = lroundf(droite + m * parPixelX);
  calib.yMin = lroundf(haut - m * parPixelY);
  calib.yMax = lroundf(bas + m * parPixelY);
  calib.echange = echange;
  tft_enregistrerCalibration();
  calibre = true;
  Serial.printf("[Calib] OK : X %d..%d  Y %d..%d  echange=%d (enregistre)\n",
                calib.xMin, calib.xMax, calib.yMin, calib.yMax, calib.echange);
  tft.setTextColor(ILI9341_GREEN);
  tft.print("Calibration OK");
  delay(1000);
  return true;
}

bool tft_estCalibre() {
  return calibre;
}

Adafruit_ILI9341 &tft_get() {
  return tft;
}
