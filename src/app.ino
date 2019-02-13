#include "exixe.h"
#include "DS3231.h"
#include "NonBlockingDelay.h"
#include "Encoder.h"
#include "limits.h"

const byte TUBE_COUNT = 6;
const byte ENCODER_COUNT = 3;
const byte MAX_BRIGHTNESS = 127;
const byte DIGIT_INCREMENT_DELAY = 65;

const byte H1 = 0;
const byte H2 = 1;
const byte M1 = 2;
const byte M2 = 3;
const byte S1 = 4;
const byte S2 = 5;

byte tubeSsPins[TUBE_COUNT] = {22, 23, 24, 25, 26, 27};
exixe *tubes[TUBE_COUNT];

DS3231 rtc;
NonBlockingDelay *displayTimeDelay;

byte encoderPins[ENCODER_COUNT][2] = {{46, 47}, {42, 43}, {38, 39}};
Encoder *encoders[ENCODER_COUNT];
long oldEncoderPositions[ENCODER_COUNT] = {0, 0, 0};

struct EncoderRanges {
  byte upper_roll;
  byte lower_roll;
  int  timeToAdd;  // the number of seconds to add to unixtime to increment by the unit of time
};

const EncoderRanges encoderRanges[ENCODER_COUNT] = {
  {12, 1, 3600}, {59, 0, 60}, {59, 0, 1}
};

void setup() {

  for(int i = 0; i < TUBE_COUNT; i++) {
    tubes[i] = new exixe(tubeSsPins[i]);
  }

  for (int i = 0; i < ENCODER_COUNT; i++) {
    encoders[i] = new Encoder(encoderPins[i][0], encoderPins[i][1]);
  }

  tubes[H1]->spi_init();
  randomSeed(analogRead(0));

  Serial.begin(9600);

  rtc.begin();

  displayTimeDelay = new NonBlockingDelay(1000);
}

void loop() {

  for (byte i = 0; i < ENCODER_COUNT; i++) {
    Encoder *encoder = encoders[i];
    EncoderRanges encoderRange = encoderRanges[i];
    oldEncoderPositions[i] = UpdateTime(encoder, encoderRange, oldEncoderPositions[i]);
  }

  if (displayTimeDelay->hasElapsed()) {
    DisplayTime();
    displayTimeDelay->reset();
  }
  //AntiTubePoisoning();
}

long UpdateTime(Encoder *encoder, EncoderRanges &encoderRange, long oldPosition) {
  long newPosition = encoder->read() / 4;

  if (newPosition != oldPosition) {

    RTCDateTime dt = rtc.getDateTime();
    
    bool increasing = false;

    if (newPosition > oldPosition) {
      increasing = true;
    }

    uint32_t newTime = GetNewTime(dt.unixtime, encoderRange.timeToAdd, increasing);
    rtc.setDateTime(newTime);
    DisplayTime();
    //Serial.println(newTime);
  }

  return newPosition;
}

uint32_t GetNewTime(uint32_t currentTime, int offset, bool increasing) {

  if (increasing == true) {
    return currentTime + offset;
  }

  return currentTime - offset;
}

void DisplayTime() {

  RTCDateTime dt = rtc.getDateTime();
  byte h = dt.hour;
  byte m = dt.minute;
  byte s = dt.second;

  // if (h > 12) {
  //   h = h - 12;
  // }

  tubes[H1]->show_digit(h/10, MAX_BRIGHTNESS, 0);
  tubes[H2]->show_digit(h%10, MAX_BRIGHTNESS, 0);
  tubes[M1]->show_digit(m/10, MAX_BRIGHTNESS, 0);
  tubes[M2]->show_digit(m%10, MAX_BRIGHTNESS, 0);
  tubes[S1]->show_digit(s/10, MAX_BRIGHTNESS, 0);
  tubes[S2]->show_digit(s%10, MAX_BRIGHTNESS, 0);
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
