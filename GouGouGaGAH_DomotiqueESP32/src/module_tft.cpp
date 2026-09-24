#include "module_tft.h"
#include "pinout.h"
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>

static Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);
static XPT2046_Touchscreen touch(TOUCH_CS); // pas d'IRQ, lecture en polling

void tft_setup() {
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);
  touch.begin();
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
