#include <Stepper.h>

int potPin = A0;

int stepsPerRevolution = 2048;

int rpm = 10;

Stepper myStepper (stepsPerRevolution, 8,9,10,11);

int previousVal =0;
int currentVal; 

void setup() {
  Serial.begin(9600);
  myStepper.setSpeed(rpm);


}

void loop() {
  int val = analogRead(A0);
  Serial.println(val);
  delay(200);
  // put your main code here, to run repeatedly:
  currentVal = analogRead(potPin);

  Serial.println(currentVal);

  myStepper.step(currentVal-previousVal);

  previousVal = currentVal;  

}
