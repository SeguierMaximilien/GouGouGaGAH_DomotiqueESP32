#include "RFIDReader.h"
#include "pinout.h"
#include <SPI.h>
#include <MFRC522.h>

static MFRC522 rfid(RFID_CS, RFID_RST);
static bool rfid_ready = false;

bool rfid_init() {
  // Suppose que SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI) a déjà été appelé
  // (une seule fois, dans le programme principal).
  rfid.PCD_Init();
  byte v = rfid.PCD_ReadRegister(rfid.VersionReg);
  rfid_ready = (v != 0x00 && v != 0xFF);
  return rfid_ready;
}

bool rfid_checkCard(String &uidStr) {
  if (!rfid_ready) return false;
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return false;
  }

  uidStr = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uidStr += "0";
    uidStr += String(rfid.uid.uidByte[i], HEX);
    if (i < rfid.uid.size - 1) uidStr += " ";
  }
  uidStr.toUpperCase();

  rfid.PICC_HaltA();
  return true;
}
