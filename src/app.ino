#include <DS3231.h>

DS3231 clock;

void setup()
{
  Serial.begin(9600);

  Serial.println("Initialize RTC");

  clock.begin();

  // __DATE__ and __TIME__ will be expanded at compile time.
  clock.setDateTime(__DATE__, __TIME__);    
}

void loop()
{
  RTCDateTime dt = clock.getDateTime();

  Serial.print(dt.month);  Serial.print("/");
  Serial.print(dt.day);    Serial.print("/");
  Serial.print(dt.year);   Serial.print(" ");
  Serial.print(dt.hour);   Serial.print(":");
  Serial.print(dt.minute); Serial.print(":");
  Serial.print(dt.second); Serial.println("");

  delay(1000);
}
