#include "exixe.h"
#include "DS3231.h"

const int TUBE_COUNT = 6;
const int MAX_BRIGHTNESS = 127;
const int DIGIT_INCREMENT_DELAY = 65;

const int H1 = 0;
const int H2 = 1;
const int M1 = 2;
const int M2 = 3;
const int S1 = 4;
const int S2 = 5;

int tubeSsPins[TUBE_COUNT] = {22, 23, 24, 25, 26, 27};
exixe *tubes[TUBE_COUNT];

DS3231 clocka;
RTCDateTime dta;

void setup() {

  for(int i = 0; i < TUBE_COUNT; i++) {
    tubes[i] = new exixe(tubeSsPins[i]);
  }

  tubes[H1]->spi_init();
  randomSeed(analogRead(0));

  Serial.begin(9600);      // open the serial port at 9600 bps:

  clocka.begin();
  clocka.setDateTime(__DATE__, __TIME__);
}

void loop() {

  DisplayTime();
  delay(1005);
  //AntiTubePoisoning();
}

void DisplayTime() {

  dta = clocka.getDateTime();
  int h = dta.hour;
  int m = dta.minute;
  int s = dta.second;

  if (h > 12) {
    h = h - 12;
  }

  int h1 = h/10;
  int h2 = h%10;
  int m1 = m/10;
  int m2 = m%10;
  int s1 = s/10;
  int s2 = s%10;
  // Serial.print(h1);
  // Serial.print(h2);
  // Serial.print(m1);
  // Serial.print(m2);
  // Serial.print(s1);
  // Serial.println(s2);

  if (h1 == 0) {
    tubes[H1]->clear();
  } else {
    tubes[H1]->show_digit(h1, 127, 0);
  }

  tubes[H2]->show_digit(h2, 127, 0);
  tubes[M1]->show_digit(m1, 127, 0);
  tubes[M2]->show_digit(m2, 127, 0);
  tubes[S1]->show_digit(s1, 127, 0);
  tubes[S2]->show_digit(s2, 127, 0);

}

void AntiTubePoisoning() {

  int loopCount = 2 * TUBE_COUNT - 1;

  for (int loopPosition = 0; loopPosition < loopCount; loopPosition++)
  {
    if (loopPosition <= loopCount / 2)
    {
      for (int tubePosition = 0; tubePosition < TUBE_COUNT; tubePosition++)
      {
        if (tubePosition <= loopPosition)
        {
          for (int hour = 0; hour <= 9; hour++) {
            tubes[tubePosition]->show_digit(hour, MAX_BRIGHTNESS/4, 0);
            delay(DIGIT_INCREMENT_DELAY);
          }
        }
        else
        {
          tubes[tubePosition]->clear();
        }
      }
    }
    else
    {
      for (int tubePosition = 0; tubePosition < TUBE_COUNT; tubePosition++)
      {
        if (tubePosition <= loopPosition - TUBE_COUNT)
        {
          tubes[tubePosition]->clear();
        }
        else
        {
          for (int hour = 0; hour <= 9; hour++) {
            tubes[tubePosition]->show_digit(hour, MAX_BRIGHTNESS/4, 0);
            delay(DIGIT_INCREMENT_DELAY);
          }
        }
      }
    }
  }
}
