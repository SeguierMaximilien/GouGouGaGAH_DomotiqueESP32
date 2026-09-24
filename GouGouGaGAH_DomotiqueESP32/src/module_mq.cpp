#include "module_mq.h"
#include "pinout.h"

void mq_setup() {
  pinMode(MQ_AO, INPUT);
  Serial.println(F("[MQ] OK (lecture analogique prete)"));
}

int mq_readRaw() {
  return analogRead(MQ_AO);
}

float mq_readPercent() {
  return (analogRead(MQ_AO) / 4095.0f) * 100.0f;
}
