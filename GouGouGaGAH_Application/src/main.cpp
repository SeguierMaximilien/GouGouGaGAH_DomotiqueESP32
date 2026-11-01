#include <WiFi.h>
// ========================================================
// Assistant domotique GouGouGaGAH - programme final
// (le programme de test des composants est dans GouGouGaGAH_DomotiqueESP32)
// ========================================================
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>

#include "pinout.h"
#include "module_bh1750.h"
#include "module_bmp280.h"
#include "module_mq.h"
#include "module_rfid.h"
#include "module_relais.h"
#include "module_tft.h"
#include "module_menu.h"
#include "module_acces.h"
#include "module_pwm.h"
#include "module_tempo.h"
#include "module_ha.h"

void setup() {
  Serial.begin(115200);
  delay(1500); // laisse le temps au moniteur série de s'ouvrir
  Serial.println(F("=== Assistant domotique GouGouGaGAH ==="));

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
  relais_setup();
  pwm_setup();
  tempo_setup();
  acces_setup();  // badges autorisés lus en mémoire NVS
  tft_setup();
  menu_setup();   // écran tactile
  ha_setup();     // WiFi + MQTT vers Home Assistant
}

void loop() {
  acces_loop();   // badges + gâche de porte
  menu_loop();    // écran tactile
  ha_loop();      // Home Assistant
  tempo_loop();   // LED RGB Tempo (clignote en permanence)

  // Récapitulatif sur le moniteur série toutes les 5 s
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 5000) {
    lastPrint = millis();
    Serial.println(String("Lux ") + bh1750_readLux() +
                   "  T " + bmp280_readTemperature() + "C" +
                   "  P " + bmp280_readPressure() + "hPa" +
                   "  MQ " + mq_readRaw() +
                   "  PWM " + pwm_get() + "%" +
                   "  MQTT " + (ha_estConnecte() ? "OK" : "--") +
                   "  WiFi " + (ha_wifiConnecte() ? String(WiFi.RSSI()) + "dBm" : String("--")));
  }
}
