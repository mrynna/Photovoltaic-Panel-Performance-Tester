#include <Adafruit_ADS1X15.h>

Adafruit_ADS1115 ads;

long adc;
float volts = 0.0;
float current = 0.0, current2 = 0.0;

void setup(){
   Serial.begin(9600);
   if (!ads.begin()) {
    Serial.println("Failed to initialize ADS.");
    while (1);
   }
}

void loop(){
   volts = 0.0;
   adc = 0;
   adc += ads.readADC_SingleEnded(0);
   volts = ads.computeVolts(adc);
   current = (adc - 13332) * 0.1875 / sensitivity;
   current2 = (volts - 2.5) * 100;

   Serial.print("ADC = ");
   Serial.print(adc);
   Serial.print(" | ");
   Serial.print(volts);
   Serial.print(" | ");
   Serial.print(current);
   Serial.print(" | ");
   Serial.println(current2);
   delay(1000);
}
