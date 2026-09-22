#include "TFTDisplay.h"
#include "pinout.h"
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

static Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);

void tft_init() {
  // Suppose que SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI) a déjà été appelé
  // (une seule fois, dans le programme principal).
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);
}

void tft_clear() {
  tft.fillScreen(ILI9341_BLACK);
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
  tft.println("Ecran TFT actif");

  tft.fillRect(10, 70, 40, 30, ILI9341_RED);
  tft.fillRect(60, 70, 40, 30, ILI9341_GREEN);
  tft.fillRect(110, 70, 40, 30, ILI9341_BLUE);
  tft.drawRect(160, 70, 40, 30, ILI9341_YELLOW);
}
