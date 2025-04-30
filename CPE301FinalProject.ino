#include <RTClib.h>
RTC_DS3231 rtc;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  rtc.begin();
  
}

void loop() {
  // put your main code here, to run repeatedly:
  void clock(*motorState){
    if(motorState == 1){
      DateTime now = rtc.now();

      Serial.print(now.month(), DEC);
      Serial.print("/");
      Serial.print(now.day(), DEC);
      Serial.print("/");
      Serial.print(now.year(), DEC);
      Serial.print(" ");
      Serial.print(now.hour(), DEC);
      Serial.print(":");
      Serial.print(now.minute(), DEC);
      Serial.print(":");
      Serial.print(now.second(), DEC);
      Serial.printIn();

    motorState = 0; //Update motorState to 1 in code which turn on or off motor
   }
  }

}
