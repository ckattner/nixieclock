#include <exixe.h>     // https://github.com/dekuNukem/exixe
#include <DS3231.h>    // https://github.com/jarzebski/Arduino-DS3231
#include <NonBlockingDelay.h>
#include <Encoder.h>   // https://github.com/PaulStoffregen/Encoder
#include <limits.h>

const byte TUBE_COUNT = 6;
const byte ENCODER_COUNT = 3;
const byte MAX_BRIGHTNESS = 127;
const byte DIGIT_INCREMENT_DELAY = 65;
const byte ANTI_POISONING_FRAME_DELAY = 200;

const byte H1 = 0;
const byte H2 = 1;
const byte M1 = 2;
const byte M2 = 3;
const byte S1 = 4;
const byte S2 = 5;

const uint32_t SECONDS_PER_YEAR = 31536000;
const uint32_t SECONDS_PER_LEAP_YEAR = 31622400;

// 31 28 31   30 31 30   31 31 30   31 30 31
const uint32_t seconds_per_month[] = {
    2678400, 2419200, 2678400, 2592000, 2678400, 2592000,
    2678400, 2678400, 2592000, 2678400, 2592000, 2678400
};

const byte tubeSsPins[TUBE_COUNT] = {22, 23, 24, 25, 26, 27};
exixe *tubes[TUBE_COUNT];

DS3231 rtc;
RTCDateTime dt;
NonBlockingDelay *displayDateDelay, *buttonDebounceTimer, *backlightFadeTimer, *antiPoisoningTimer;

enum direction_t : byte {
    NONE,
    UP,
    DOWN
};

const byte encoderPins[ENCODER_COUNT][2] = {{46, 47}, {42, 43}, {38, 39}};
const byte encoderButtonPins[ENCODER_COUNT] = {48, 44, 40};
Encoder *encoders[ENCODER_COUNT];
long oldEncoderPositions[ENCODER_COUNT] = {0, 0, 0};
direction_t encoderDirections[ENCODER_COUNT] = { NONE, NONE, NONE };
const uint16_t timeOffsets[ENCODER_COUNT] = {3600, 60, 1};

unsigned char red = 0, green = 0, blue = 60;

uint8_t loop_count = 0;

enum display_state_t : byte {
    TIME,
    DATE,
    ANTIPOISONING
};

display_state_t display_state = TIME;

bool button_clicked = false;

void setup() {
  Serial.begin(9600);

  InitializeTubes();
  InitialzieEncoders();
  InitializeEncoderButtons();
  InitializeSPI();
  InitializeRTC();
  InitializeDelayTimers();
}

void loop() {
    ReadClock();
    ReadButtons();
    ReadEncoders();

    UpdateDisplayState();

    UpdateDisplay();

    UpdateDateTime();

    UpdateBacklight();
}

void DisplayAntiTubePoisoning() {

  const int frames[15][6] = {
    { 0, 10, 10, 10, 10, 10},
    { 1,  0, 10, 10, 10, 10},
    { 2,  1,  0, 10, 10, 10},
    { 3,  2,  1,  0, 10, 10},
    { 4,  3,  2,  1,  0, 10},
    { 5,  4,  3,  2,  1,  0},
    { 6,  5,  4,  3,  2,  1},
    { 7,  6,  5,  4,  3,  2},
    { 8,  7,  6,  5,  4,  3},
    { 9,  8,  7,  6,  5,  4},
    {10,  9,  8,  7,  6,  5},
    {10, 10,  9,  8,  7,  6},
    {10, 10, 10,  9,  8,  7},
    {10, 10, 10, 10,  9,  8},
    {10, 10, 10, 10, 10,  9}
  };

  if (antiPoisoningTimer->hasElapsed()) {

    if (loop_count == 15) {
        loop_count = 0;
    }

    for (uint8_t tube_index = 0; tube_index < TUBE_COUNT; tube_index++) {

      tubes[tube_index]->clear();
      tubes[tube_index]->set_led(red, green, blue); // do this because clear() also unsets leds

      if (frames[loop_count][tube_index] != 10) {
        tubes[tube_index]->show_digit(frames[loop_count][tube_index], 127, 0);
      }
    }

    loop_count++;
    antiPoisoningTimer->reset();
  }
}

void UpdateBacklight() {

    if (backlightFadeTimer->hasElapsed()) {
        switch (display_state) {
            case DATE:
                red = (red == 60) ? 60 : red + 5;
                blue = (blue == 0) ? 0 : blue - 5;
                green = (green == 0) ? 0 : green - 5;
                break;
            case TIME:
                blue = (blue == 60) ? 60 : blue + 5;
                red = (red == 0) ? 0 : red - 5;
                green = (green == 0) ? 0 : green - 5;
                break;
            case  ANTIPOISONING:
                green = (green == 60) ? 60 : green + 5;
                red = (red == 0) ? 0 : red - 5;
                blue = (blue == 0) ? 0 : blue - 5;
                break;
        }

        for (uint8_t i = 0; i < TUBE_COUNT; i++) {
            tubes[i]->set_led(red, green, blue);
        }

        backlightFadeTimer->reset();
    }
}

void UpdateDateTime() {
    if (display_state == DATE) {
        UpdateDate();
    } else if (display_state == TIME) {
        UpdateTime();
    }
}

void UpdateDate() {

    for (uint8_t i = 0; i < ENCODER_COUNT; i++) {

        if (encoderDirections[i] != NONE) {
            uint32_t offset = 0;

            switch (i) {
              case 0:
                offset = seconds_per_month[dt.month];
                break;
              case 1:
                offset = 86400;
                break;
              case 2:
                offset = (dt.year % 4 == 0) ? SECONDS_PER_LEAP_YEAR : SECONDS_PER_YEAR;
                break;
            }

            offset = (encoderDirections[i] == DOWN) ? -1 * offset : offset;
            uint32_t newTime = dt.unixtime + offset;
            rtc.setDateTime(newTime);
            encoderDirections[i] = NONE;
        }
    }
}

void UpdateTime() {

    for (uint8_t i = 0; i < ENCODER_COUNT; i++) {

        if (encoderDirections[i] != NONE) {

            uint32_t offset = (encoderDirections[i] == DOWN) ? (-1 * timeOffsets[i]) : (timeOffsets[i]);
            uint32_t newTime = dt.unixtime + offset;
            rtc.setDateTime(newTime);
            encoderDirections[i] = NONE;
        }
    }
}

void ReadClock() {

    dt = rtc.getDateTime();
}

void ReadEncoders() {

    for (byte i = 0; i < ENCODER_COUNT; i++) {
        Encoder *encoder = encoders[i];

        long encoderPosition = encoder->read() / 4;

        if (encoderPosition > oldEncoderPositions[i]) {
            encoderDirections[i] = UP;
        }

        if (encoderPosition < oldEncoderPositions[i]) {
            encoderDirections[i] = DOWN;
        }

        if (encoderPosition == oldEncoderPositions[i]) {
            encoderDirections[i] = NONE;
        }

        oldEncoderPositions[i] = encoderPosition;
    }
}

void ReadButtons() {

    if (buttonDebounceTimer->hasElapsed()) {

        for (byte i = 0; i < ENCODER_COUNT; i++) {

            if (digitalRead(encoderButtonPins[i]) == LOW) {

                button_clicked = !button_clicked;
                buttonDebounceTimer->reset();
                return;
            }
        }
    }
}

void UpdateDisplay() {

    switch (display_state) {
        case TIME:
            DisplayTime();
            break;
        case DATE:
            DisplayDate();
            break;
        case ANTIPOISONING:
            DisplayAntiTubePoisoning();
            break;
    }
}

void UpdateDisplayState() {

    bool elapsed = displayDateDelay->hasElapsed();

    if (display_state == DATE && !elapsed) {
        display_state = DATE;
    } else {
        if (dt.second == 0 || button_clicked) {
            display_state = DATE;
            displayDateDelay->reset();
            button_clicked = false;
        } else {
            if ((dt.minute % 15 == 0 && dt.second == 30) ||
                (display_state == ANTIPOISONING && loop_count < 15)) {
                display_state = ANTIPOISONING;
            } else {
                display_state = TIME;
            }
        }
    }

    if (display_state == DATE) {
        for (uint8_t i = 0; i < ENCODER_COUNT; i++) {
            if (encoderDirections[i] != NONE) {
                displayDateDelay->reset();
                return;
            }
        }
    }
}

void DisplayTime() {

    uint8_t h = dt.hour;
    uint8_t m = dt.minute;
    uint8_t s = dt.second;

    UpdateTubes(h, m, s);
}

void DisplayDate() {

    uint8_t m = dt.month;
    uint8_t d = dt.day;
    uint16_t y = dt.year;

    UpdateTubes(m, d, y);
}

void UpdateTubes(uint8_t l, uint8_t c, uint16_t r) {

    tubes[H1]->show_digit(l/10, MAX_BRIGHTNESS, 0);
    tubes[H2]->show_digit(l%10, MAX_BRIGHTNESS, 0);
    tubes[M1]->show_digit(c/10, MAX_BRIGHTNESS, 0);
    tubes[M2]->show_digit(c%10, MAX_BRIGHTNESS, 0);
    tubes[S1]->show_digit(r/10, MAX_BRIGHTNESS, 0);
    tubes[S2]->show_digit(r%10, MAX_BRIGHTNESS, 0);
}

void InitializeTubes() {
    for(int i = 0; i < TUBE_COUNT; i++) {
      tubes[i] = new exixe(tubeSsPins[i]);
    }
}

void InitialzieEncoders() {
    for (int i = 0; i < ENCODER_COUNT; i++) {
        encoders[i] = new Encoder(encoderPins[i][0], encoderPins[i][1]);
    }
}

void InitializeEncoderButtons() {
    for (byte i = 0; i < ENCODER_COUNT; i++) {
        pinMode(encoderButtonPins[i], INPUT_PULLUP);
    }
}

void InitializeSPI() {
    // initialize SPI.  Only do this _once_ not once per tube.
    tubes[H1]->spi_init();
}

void InitializeRTC() {
    // setup the real time clock.
    rtc.begin();
//    rtc.setDateTime(__DATE__, __TIME__);
}

void InitializeDelayTimers() {
    displayDateDelay = new NonBlockingDelay(5000);
    buttonDebounceTimer = new NonBlockingDelay(250);
    backlightFadeTimer = new NonBlockingDelay(65);
    antiPoisoningTimer = new NonBlockingDelay(ANTI_POISONING_FRAME_DELAY);
}
