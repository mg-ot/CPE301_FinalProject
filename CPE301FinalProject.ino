#include <RTClib.h>
RTC_DS3231 rtc;

//UART
volatile unsigned char *myUCSR0A = (unsigned char *) 0x00C0;
volatile unsigned char *myUCSR0B = (unsigned char *) 0x00C1;
volatile unsigned char *myUCSR0C = (unsigned char *) 0x00C2;
volatile unsigned int  *myUBRR0  = (unsigned int  *) 0x00C4;
volatile unsigned char *myUDR0   = (unsigned char *) 0x00C6;

void setup() {
  // put your setup code here, to run once:
  U0Init(9600);
  rtc.begin();
  
}

void loop() {
  // put your main code here, to run repeatedly:
  void clock(*motorState){
    if(motorState == 1){
      DateTime now = rtc.now();
      char buffer[25];
      sprintf(buffer, "%d/%d/%d %d:%d:%d", now.day(), now.month(), now.day(), now.hour, now.minute(), now.second());

      U0puts(buffer);

      U0putchar('\r');
      U0putchar('\n');
    motorState = 0; //Update motorState to 1 in code which turn on or off motor
   }
  }

}

void U0Init(unsigned long U0baud) {
unsigned int tbaud = (16000000 / (16 * U0baud)) - 1;

*myUCSR0A = 0x20;
*myUCSR0B = 0x18;
*myUCSR0C = 0x06;
*myUBRR0 = tbaud;
}

unsigned char U0kbhit() {
if (*myUCSR0A & RDA)
  return 1;
else
  return 0;
}

unsigned char U0getchar() {
while (!U0kbhit()) {}
  return *myUDR0;
}

void U0putchar(unsigned char U0pdata) {
while (!(*myUCSR0A & TBE)) {}
  *myUDR0 = U0pdata;
}

unsigned char U0getchar() {
while (!U0kbhit()) {}
  return *myUDR0;
}

void U0putchar(unsigned char U0pdata) {
while (!(*myUCSR0A & TBE)) {}
  *myUDR0 = U0pdata;
}

void U0puts(const char *s) {
    while (*s) { // Loop until null terminator is found
        U0putchar(*s);
        s++;
    }
}
