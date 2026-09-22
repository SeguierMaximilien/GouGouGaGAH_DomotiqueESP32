#include "TouchScreen.h"
#include "pinout.h"
#include <SPI.h>
#include <XPT2046_Touchscreen.h>

static XPT2046_Touchscreen touch(TOUCH_CS, TOUCH_IRQ);

void touch_init() {
  // Suppose que SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI) a déjà été appelé
  // (une seule fois, dans le programme principal).
  touch.begin();
}

bool touch_getPoint(int &x, int &y) {
  if (!touch.touched()) return false;
  TS_Point p = touch.getPoint();
  x = p.x;
  y = p.y;
  return true;
}
