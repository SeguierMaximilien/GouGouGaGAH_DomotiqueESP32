#include "MQSensor.h"
#include "pinout.h"

void mq_init() {
  pinMode(MQ_AO, INPUT);
  // ATTENTION : si le MQ est alimenté en 5V, sa sortie AO peut depasser
  // 3.3V et endommager l'ADC de l'ESP32. Alimenter le MQ en 3.3V,
  // ou utiliser un pont diviseur de tension sur AO.
}

int mq_readRaw() {
  return analogRead(MQ_AO);
}

float mq_readPercent() {
  return (analogRead(MQ_AO) / 4095.0f) * 100.0f;
}
