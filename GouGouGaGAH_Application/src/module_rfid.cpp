#include "module_rfid.h"
#include "pinout.h"
#include <SPI.h>
#include <MFRC522.h>

static MFRC522 rfid(RFID_CS, RFID_RST);
static bool rfid_ready = false;

bool rfid_setup() {
  rfid.PCD_Init();
  byte version = rfid.PCD_ReadRegister(rfid.VersionReg);
  rfid_ready = (version != 0x00 && version != 0xFF);

  if (rfid_ready) {
    Serial.print(F("[RC522] OK (version registre 0x"));
    Serial.print(version, HEX);
    Serial.println(F(")"));
  } else {
    Serial.println(F("[RC522] ECHEC (verifier SPI et alim 3.3V, jamais 5V !)"));
  }
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
