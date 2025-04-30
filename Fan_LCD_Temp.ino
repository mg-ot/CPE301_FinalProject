#include <LiquidCrystal.h>
#include <DHT.h>

LiquidCrystal lcd(22, 23, 24, 25, 26, 27);

DHT dht(7, DHT11); // DHT11 on pin 7


const float tempHighThreshold = 28.0; //Fan Threshold to turn on

void setup() {
    dht.begin(); // Start DHT
    lcd.begin(16, 2); // Start LCD
    lcd.clear();

    
    DDRE |= (1 << PE3); //D5 fan enableas output
    DDRG |= (1 << PG5); //D4 motor input 1 as output
    DDRE |= (1 << PE5); //D3 motor input 2 as output

   //motor direction as forward
    PORTG |= (1 << PG5);  //D4 HIGH:motor input 1)
    PORTE &= ~(1 << PE5); //D3 LOW:motor input 2)

    analogWrite(5, 0); // Fan off initially
}
void loop() {
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    if (temperature >= 0 && temperature <= 100 && humidity >= 0 && humidity <= 100) {
        
        if (temperature > tempHighThreshold) {
            analogWrite(5, 255); // D5: fan on at full speed
        } else {
            analogWrite(5, 0);   // D5: Fan off
        }

        //Display temperature and humidity 
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Temp: ");
        lcd.print(temperature, 1);
        lcd.print((char)223);
        lcd.print("C");

        lcd.setCursor(0, 1);
        lcd.print("Hum: ");
        lcd.print(humidity, 1);
        lcd.print("%");
    }

    delay(2000); // Temporary, replace with millis() later
}