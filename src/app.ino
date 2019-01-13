#include "exixe.h"

int cs1 = 22;
int cs2 = 23;
int cs3 = 24;
int cs4 = 25;
int cs5 = 26;
int cs6 = 27;

exixe tube1 = exixe(cs1);
exixe tube2 = exixe(cs2);
exixe tube3 = exixe(cs3);
exixe tube4 = exixe(cs4);
exixe tube5 = exixe(cs5);
exixe tube6 = exixe(cs6);

void setup()
{
  tube1.spi_init();

  tube1.clear();
  tube2.clear();
  tube3.clear();
  tube4.clear();
  tube5.clear();
  tube6.clear();
}

int color = 127;

void loop()
{
  tube1.set_led(0, color, 0);
  tube2.set_led(0, 0, color);
  tube3.set_led(color, 0, 0);
  tube4.set_led(0, color, 0);
  tube5.set_led(0, 0, color);
  tube6.set_led(color, 0, 0);

  delay(1000);
}
