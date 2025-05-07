//Authors: Manuel Gonzalez, Tirth Patel, Vanessa Medina, Luis Matheus



#include <LiquidCrystal.h>
#include <DHT.h>
#include <Stepper.h>
#include <RTClib.h>

LiquidCrystal lcd(22, 23, 24, 25, 26, 27);
DHT dht(7, DHT11);
Stepper ventStepper(2048, 13, 11, 12, 10);
RTC_DS1307 rtc;

enum SystemState { DISABLED = 0, IDLE, RUNNING, ERROR };
volatile SystemState state = DISABLED;
SystemState lastLoggedState = DISABLED;
unsigned long lastLCD = 0;
int previousStep = -1;
int lastMotorState = 0;


const int fanPWM = 5;
const int tempThreshold = 28;
const int waterThreshold = 175;

//Reset button on D50 (Must be D50:PB3)
#define RESET_BUTTON 3

//LEDs
#define LED_DISABLED (1 << 7) // D30
#define LED_IDLE     (1 << 6) // D31
#define LED_ERROR    (1 << 5) // D32
#define LED_RUNNING  (1 << 4) // D33

//uART
#define TBE 0x20
volatile unsigned char* myUCSR0A = (unsigned char*) 0x00C0;
volatile unsigned char* myUCSR0B = (unsigned char*) 0x00C1;
volatile unsigned char* myUCSR0C = (unsigned char*) 0x00C2;
volatile unsigned int*  myUBRR0  = (unsigned int*)  0x00C4;
volatile unsigned char* myUDR0   = (unsigned char*) 0x00C6;

//ADC regsite
volatile unsigned char* my_ADMUX  = (unsigned char*) 0x7C;
volatile unsigned char* my_ADCSRA = (unsigned char*) 0x7A;
volatile unsigned char* my_ADCSRB = (unsigned char*) 0x7B;
volatile unsigned int*  my_ADC_DATA = (unsigned int*) 0x78;

//ISR
volatile unsigned char* portE     = (unsigned char*) 0x2E;
volatile unsigned char* portDDRE  = (unsigned char*) 0x2D;
volatile unsigned char* myEICRB   = (unsigned char*) 0x6A;
volatile unsigned char* myEIMSK   = (unsigned char*) 0x3D;
volatile unsigned char* myTCCR1A  = (unsigned char*) 0x80;
volatile unsigned char* myTCCR1B  = (unsigned char*) 0x81;
volatile unsigned char* myTCCR1C  = (unsigned char*) 0x82;
volatile unsigned char* myTIMSK1  = (unsigned char*) 0x6F;
volatile unsigned int*  myTCNT1   = (unsigned int*)  0x84;

//code Functions
void updateLEDs(SystemState s) {
  PORTC &= ~(LED_DISABLED | LED_IDLE | LED_RUNNING | LED_ERROR);
  if (s == DISABLED) PORTC |= LED_DISABLED;
  if (s == IDLE)      PORTC |= LED_IDLE;
  if (s == RUNNING)   PORTC |= LED_RUNNING;
  if (s == ERROR)     PORTC |= LED_ERROR;
}

void U0Init(unsigned long baud) {
  unsigned int tbaud = (16000000 / (16 * baud)) - 1;
  *myUCSR0A = 0x20;
  *myUCSR0B = 0x18;
  *myUCSR0C = 0x06;
  *myUBRR0 = tbaud;
}
void U0putchar(unsigned char data) {
  while (!(*myUCSR0A & TBE)) {}
  *myUDR0 = data;
}
void U0puts(const char* s) {
  while (*s) U0putchar(*s++);
}
void logRTCEvent(const char* label, DateTime now) {
  char buffer[50];
  int hour = now.hour();
  if (hour == 0) hour = 12;
  else if (hour > 12) hour -= 12;
  sprintf(buffer, "%02d/%02d/%04d %d:%02d ", now.month(), now.day(), now.year(), hour, now.minute());
  U0puts(buffer);
  U0puts(label);
  U0putchar('\r');
  U0putchar('\n');
}

void adc_init() {
  *my_ADCSRA |= 0x80;
  *my_ADCSRA &= ~(0x40 | 0x20);
  *my_ADCSRA |= 0x07;
  *my_ADCSRB &= ~(0x08 | 0x07);
  *my_ADMUX |= 0x40;
  *my_ADMUX &= ~(0x20);
}
unsigned int adc_read(unsigned char channel) {
  *my_ADMUX = (*my_ADMUX & 0xF0) | (channel & 0x0F);
  *my_ADCSRA |= 0x40;
  while (*my_ADCSRA & 0x40);
  return *my_ADC_DATA;
}

void setupButtonISR() {
  *portDDRE &= ~(1 << 4); 
  *portE    |=  (1 << 4); 
  *myEICRB  &= ~(0x0F);
  *myEICRB  |=  (1 << 2);
  *myEIMSK  |=  (1 << 4);
  *myTCCR1A = 0x00;
  *myTCCR1B = 0x00;
  *myTCCR1C = 0x00;
  *myTIMSK1 |= (1 << 0);
  *myTCNT1 = 0;
}

ISR(INT4_vect) {
  *myTCNT1 = 0;
  *myTCCR1B |= (1 << 1);
}
ISR(TIMER1_OVF_vect) {
  *myTCCR1B = 0x00;
  if (state != DISABLED) state = DISABLED;
  else state = IDLE;
  *myTCNT1 = 0;
}

void setup() {
  dht.begin();
  lcd.begin(16, 2);
  rtc.begin();
  U0Init(9600);
  adc_init();
  ventStepper.setSpeed(10);

  //***NOTE*** only uncomment this to change time. must be set again if using new RTC module
  //rtc.adjust(DateTime(2025, 5, 6, 21, 18, 0));  //This currently sets time to May 6th, 2025 9:18

  //Fan
  DDRE |= (1 << PE3);
  DDRG |= (1 << PG5);
  DDRE |= (1 << PE5);
  PORTG |= (1 << PG5);
  PORTE &= ~(1 << PE5);
  analogWrite(fanPWM, 0);

  //LEDs
  DDRC |= LED_DISABLED | LED_IDLE | LED_RUNNING | LED_ERROR;
  updateLEDs(state);

  //Reset button (d50, other pins didnt work?)
  DDRB &= ~(1 << RESET_BUTTON);
  PORTB |= (1 << RESET_BUTTON);

  state = DISABLED;
  lastLoggedState = DISABLED;
  lastLCD = millis() - 60000;

  setupButtonISR();
  sei();
}
void loop() {
  if (state != lastLoggedState) {
    DateTime now = rtc.now();
    logRTCEvent((state == DISABLED) ? "DISABLED" :
                (state == IDLE)     ? "IDLE" :
                (state == RUNNING)  ? "RUNNING" : "ERROR", now);
    updateLEDs(state);
    lastLoggedState = state;

    if (state == IDLE || state == RUNNING) {
      lastLCD = millis() - 60000; // LCD needs to refresh when going from disabled to idle, not wait a minute
    }
  }

  if (state == DISABLED) {
    analogWrite(fanPWM, 0);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("System OFF");
    return;
  }

  if (state == ERROR) {
    analogWrite(fanPWM, 0);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ERROR: Water low");

    if ((PINB & (1 << 3)) == 0) { 
      unsigned int waterVal = adc_read(1);
      if (waterVal >= waterThreshold) {
        state = IDLE;
      }
    }
    return;
  }

  DateTime now = rtc.now();
  float temp = dht.readTemperature();
  float humid = dht.readHumidity();
  if (temp < 0 || temp > 100 || humid < 0 || humid > 100) return;

  if (millis() - lastLCD >= 60000) {
    lastLCD = millis();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Temp: ");
    lcd.print(temp, 1);
    lcd.print((char)223);
    lcd.print("C");

    lcd.setCursor(0, 1);
    lcd.print("Hum: ");
    lcd.print(humid, 1);
    lcd.print("%");
  }

  unsigned int waterVal = adc_read(1);
  if (waterVal < waterThreshold && state != ERROR) {
    state = ERROR;
    return;
  }

  if (state == IDLE && temp > tempThreshold) {
    state = RUNNING;
  } else if (state == RUNNING && temp <= tempThreshold) {
    state = IDLE;
  }

  int motorIsOn = (state == RUNNING);
  analogWrite(fanPWM, motorIsOn ? 255 : 0);
  if (motorIsOn != lastMotorState) {
    logRTCEvent(motorIsOn ? "Fan ON" : "Fan OFF", now);
    lastMotorState = motorIsOn;
  }

  unsigned int potVal = adc_read(0);
  int targetStep = map(potVal, 0, 1023, 0, 2048);
  int stepsToMove = targetStep - previousStep;
  if (abs(stepsToMove) >= 5) {
    ventStepper.step(stepsToMove);
    previousStep = targetStep;
    logRTCEvent("Vent Moved", now);
  }
}
