#include <Arduino.h>

// ---- Bus I2C (BH1750 + BMP280) ----
#define I2C_SDA     6
#define I2C_SCL     7
 
// ---- Bus SPI partagé (TFT + Tactile + RC522) ----
#define SPI_SCK     18
#define SPI_MOSI    19
#define SPI_MISO    20
 
// ---- Ecran TFT ----
#define TFT_CS      21
#define TFT_DC      22
#define TFT_RST     23
// LED (retroeclairage) -> 3V3 direct, pas de GPIO
 
// ---- Tactile (partage SPI_SCK / SPI_MOSI / SPI_MISO) ----
#define TOUCH_CS    4
 
// ---- RFID RC522 (partage SPI_SCK / SPI_MOSI / SPI_MISO) ----
#define RFID_CS     11
#define RFID_RST    0
 
// ---- Capteur MQ (analogique) ----
#define MQ_AO

void setup() {
  
}

void loop() {
  
}
//six seven