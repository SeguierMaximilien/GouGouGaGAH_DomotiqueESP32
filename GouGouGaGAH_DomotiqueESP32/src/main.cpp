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
#include "module_relais.h"

// Sert de multimètre : lit l'état des lignes SDA/SCL avant de démarrer l'I2C.
// Au repos, les résistances de tirage des modules doivent les maintenir à 1 (3,3 V).
static void i2c_testerLignes() {
  pinMode(I2C_SDA, INPUT);
  pinMode(I2C_SCL, INPUT);
  delay(5);
  int sda = digitalRead(I2C_SDA), scl = digitalRead(I2C_SCL);
  pinMode(I2C_SDA, INPUT_PULLUP);
  pinMode(I2C_SCL, INPUT_PULLUP);
  delay(5);
  int sdaPU = digitalRead(I2C_SDA), sclPU = digitalRead(I2C_SCL);

  const char *noms[2] = { "SDA (GPIO6)", "SCL (GPIO7)" };
  int seul[2] = { sda, scl }, avecTirage[2] = { sdaPU, sclPU };
  for (int i = 0; i < 2; i++) {
    Serial.printf("[I2C] %s : ", noms[i]);
    if (seul[i]) Serial.println(F("OK (a 3,3 V au repos)"));
    else if (avecTirage[i]) Serial.println(F("FLOTTANTE : aucun module ne la tire a 3,3 V (fil debranche ou module non alimente ?)"));
    else Serial.println(F("BLOQUEE A 0 V : module non alimente, court-circuit a la masse ou fil mal place"));
  }

  // SDA bloquée mais SCL libre : un capteur est peut-être resté coincé au milieu
  // d'un échange. 9 coups d'horloge suffisent en général à le libérer.
  if (!sdaPU && sclPU) {
    Serial.println(F("[I2C] Tentative de deblocage du bus (9 impulsions SCL)"));
    pinMode(I2C_SCL, OUTPUT_OPEN_DRAIN);
    for (int i = 0; i < 9; i++) {
      digitalWrite(I2C_SCL, LOW);  delayMicroseconds(5);
      digitalWrite(I2C_SCL, HIGH); delayMicroseconds(5);
    }
    pinMode(I2C_SCL, INPUT_PULLUP);
  }
}

// Liste les adresses qui répondent : 0x23 ou 0x5C = BH1750, 0x76 ou 0x77 = BMP280.
static void i2c_scanner() {
  int trouves = 0;
  for (uint8_t adr = 1; adr < 127; adr++) {
    Wire.beginTransmission(adr);
    uint8_t err = Wire.endTransmission();
    if (err == 0) {
      Serial.printf("[I2C] Peripherique trouve a 0x%02X\n", adr);
      trouves++;
    } else if (err >= 4) {
      Serial.println(F("[I2C] Bus bloque, scan interrompu (voir l'etat SDA/SCL ci-dessus)"));
      return;
    }
  }
  if (trouves == 0) Serial.println(F("[I2C] Aucun peripherique ne repond"));
}

void setup() {
  Serial.begin(115200);
  delay(1500); // laisse le temps au moniteur série de s'ouvrir
  Serial.println(F("=== Diagnostic GouGouGaGAH ==="));

  // Bus I2C : BH1750 + BMP280
  i2c_testerLignes();
  Wire.begin(I2C_SDA, I2C_SCL);
  i2c_scanner();
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
                   "  MQ " + mq_readRaw();
    Serial.println(ligne);
  }
}
