#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>

#include "pinout.h"
#include "module_bh1750.h"
#include "module_bmp280.h"
#include "module_mq.h"
#include "module_rfid.h"
#include "module_tft.h"
#include "module_menu.h"

void setup() {
  Serial.begin(115200);
  delay(1500); // laisse le temps au moniteur série de s'ouvrir
  Serial.println(F("=== Diagnostic GouGouGaGAH ==="));

  // Bus I2C : BH1750 + BMP280
  Wire.begin(I2C_SDA, I2C_SCL);
  // Bus SPI partagé : TFT + tactile + RC522
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);

  // Toutes les puces SPI désélectionnées avant d'initialiser quoi que ce soit
  pinMode(TFT_CS, OUTPUT);   digitalWrite(TFT_CS, HIGH);
  pinMode(TOUCH_CS, OUTPUT); digitalWrite(TOUCH_CS, HIGH);
  pinMode(RFID_CS, OUTPUT);  digitalWrite(RFID_CS, HIGH);

  bh1750_setup();
  bmp280_setup();
  mq_setup();
  rfid_setup();
  tft_setup();
  menu_setup(); // menu tactile : un bouton par composant
}

void loop() {
  menu_loop();

  // Récapitulatif sur le moniteur série toutes les 2 s
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 2000) {
    lastPrint = millis();
    String ligne = String("Lux ") + bh1750_readLux() +
                   "  T " + bmp280_readTemperature() + "C" +
                   "  P " + bmp280_readPressure() + "hPa" +
                   "  MQ " + mq_readPercent() + "%";
    Serial.println(ligne);
  }
}
