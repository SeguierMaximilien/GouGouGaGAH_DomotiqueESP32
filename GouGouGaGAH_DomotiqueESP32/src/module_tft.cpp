#include "module_tft.h"
#include "pinout.h"
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>

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

// ---- Calibration du tactile (valeurs brutes aux bords de l'écran) ----
// Valeurs typiques pour un ILI9341 2.8". Pour les ajuster : écran "Tactile" du menu,
// touche les 4 coins et recopie les valeurs brutes affichées.
#define TOUCH_X_MIN  300
#define TOUCH_X_MAX  3800
#define TOUCH_Y_MIN  300
#define TOUCH_Y_MAX  3800
// Passe à 1 si le point touché apparaît en miroir (gauche/droite ou haut/bas).
#define TOUCH_INVERT_X 0
#define TOUCH_INVERT_Y 0

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
#define TFT_TEST_ORIENTATIONS 1
#define TFT_CONFIG 'F'

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
  touch.setRotation(1); // paysage (à ajuster avec TOUCH_INVERT_* si besoin)
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

bool tft_checkTouch(int &x, int &y) {
  if (!touch.touched()) return false;
  TS_Point p = touch.getPoint();
  x = p.x;
  y = p.y;
  return true;
}

bool tft_checkTouchPixel(int &x, int &y) {
  int rx, ry;
  if (!tft_checkTouch(rx, ry)) return false;
  x = map(rx, TOUCH_X_MIN, TOUCH_X_MAX, 0, tft.width() - 1);
  y = map(ry, TOUCH_Y_MIN, TOUCH_Y_MAX, 0, tft.height() - 1);
  if (TOUCH_INVERT_X) x = tft.width() - 1 - x;
  if (TOUCH_INVERT_Y) y = tft.height() - 1 - y;
  x = constrain(x, 0, tft.width() - 1);
  y = constrain(y, 0, tft.height() - 1);
  return true;
}

Adafruit_ILI9341 &tft_get() {
  return tft;
}
