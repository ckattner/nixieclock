#include "exixe.h"
#include "DS3231.h"
#include "NonBlockingDelay.h"
#include "Encoder.h"
#include "limits.h"

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

DS3231 rtc;
NonBlockingDelay *displayTimeDelay;
Encoder hoursEnc(40, 41);
long oldPos = 0;

void setup() {

  for(int i = 0; i < TUBE_COUNT; i++) {
    tubes[i] = new exixe(tubeSsPins[i]);
  }

  tubes[H1]->spi_init();
  randomSeed(analogRead(0));

  Serial.begin(9600);      // open the serial port at 9600 bps:

  rtc.begin();
  rtc.setDateTime(__DATE__, __TIME__);

  displayTimeDelay = new NonBlockingDelay(1000);
}

void loop() {

  UpdateTime();

  if (displayTimeDelay->hasElapsed()) {
    DisplayTime();
    displayTimeDelay->reset();
  }
  //AntiTubePoisoning();
}

void UpdateTime() {

  long newPos = hoursEnc.read() / 4;
  
  if (newPos != oldPos) {

    RTCDateTime dt = rtc.getDateTime();
    byte mins;
    if (newPos > oldPos) {
      mins = GetNewMinute(dt.minute, true);
      rtc.setDateTime(dt.year, dt.month, dt.day, dt.hour, mins, dt.second);
    } else {
      mins = GetNewMinute(dt.minute, false);
      rtc.setDateTime(dt.year, dt.month, dt.day, dt.hour, mins, dt.second);
    }
    Serial.print(" ");
    Serial.println(mins);
    oldPos = newPos;
    DisplayTime();
  }
}

uint8_t GetNewMinute(uint8_t minute, bool up) {

  if (up == true) {
    if (minute == 59) {
      return 0;
    } else {
      return minute + 1;
    }
  } else {
    if (minute == 0) {
      return 59;
    } else {
      return minute - 1;
    }
  }
}

void DisplayTime() {

  RTCDateTime dt = rtc.getDateTime();
  byte h = dt.hour;
  byte m = dt.minute;
  byte s = dt.second;

  if (h > 12) {
    h = h - 12;
  }

  tubes[H1]->show_digit(h/10, 127, 0);
  tubes[H2]->show_digit(h%10, 127, 0);
  tubes[M1]->show_digit(m/10, 127, 0);
  tubes[M2]->show_digit(m%10, 127, 0);
  tubes[S1]->show_digit(s/10, 127, 0);
  tubes[S2]->show_digit(s%10, 127, 0);
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
