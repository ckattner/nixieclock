#include "exixe.h"

const int TUBE_COUNT = 6;
const int MAX_BRIGHTNESS = 127;

int tubeSsPins[TUBE_COUNT] = {22, 23, 24, 25, 26, 27};
exixe *tubes[TUBE_COUNT];

void setup() {

  for(int i = 0; i < TUBE_COUNT; i++) {
    tubes[i] = new exixe(tubeSsPins[i]);
  }

  tubes[0]->spi_init();
  randomSeed(analogRead(0));
}

void loop() {

  for (int j = 0; j < 10; j++) {

    for (int i = 0; i < TUBE_COUNT; i++) {

      tubes[i]->show_digit(j, MAX_BRIGHTNESS, 0);
      tubes[i]->set_led(random(0, 128), random(0, 128), random(0, 128));
      delay(100);

    }

    delay(500);
  }
}
