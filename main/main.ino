#include <Adafruit_ADS1X15.h>

Adafruit_ADS1115 ads;

// Voltage Divider
long adc0 = 0.0; 
float R1 = 9752.0; //Resistor 1, 
float R2 = 206000.0; //Resistor 2
float volts0, voc; 
//endOf Voltage Divider

//ACS758
long adc1 = 0.0;
float volts1 = 0.0;
float current = 0.0, current2 = 0.0;
//endOf ACS758

void setup(){
   Serial.begin(9600);
   if (!ads.begin()) {
    Serial.println("Failed to initialize ADS."); // Print in Serial Monitor if ADS1115 fail to initialize
    while (1);
   }
}

void loop(){

   readVoltage(0);
   readCurrent(1);
   Serial.println("");
   delay(1000);

}

void readVoltage(int inputPin){
   volts0 = 0; // reset volts0 value
   // Make samples 10x, and take the average
   for (int i = 0; i < 10; i++){
      adc0 = ads.readADC_SingleEnded(inputPin); // read ADC from inputPin of ADS1115
      volts0 += ads.computeVolts(adc0); // convert ADS1115 ADC value to Voltage
      delay(1);
   }
   volts0 /= 10; // Take average from above samples
   voc = ((R1 + R2)/R1) * volts0; // convert volts0 (input Voltage) to actual Voltage (VCC)
   if(voc < 0.05){
      voc = 0; // if voc < 0.05, make voc = 0
   }else{
      // do nothing
   }
   
   Serial.print("Sensor Tegangan, ADC : "); 
   Serial.print(adc0); 
   Serial.print(" | "); 
   Serial.print(volts0);
   Serial.print(" V | "); 
   Serial.print(voc); 
   Serial.println(" V");
   delay(1);
}

void readCurrent(int inputPin){
   current = 0.0;
   volts1 = 0.0;
   for(int i = 0; i < 10; i++){
      adc1 = ads.readADC_SingleEnded(inputPin); // read ADC from inputPin of ADS1115
      current += (adc1 - 13333) * 0.1875 / 10; // convert ADS1115 ADC to Current
      delay(1);
   }
   volts1 = ads.computeVolts(adc1); // convert ADS1115 A0 ADC to Voltage
   current /= 10;
   current2 = (volts1 - 2.5) * 100; // convert Voltage (volts variable) to Current
   if (current < 0){
      current = 0; // If current value < 0, make current = 0
   }else{
      // Do nothing
   }

   if (current2 < 0){
      current2 = 0; // If current2 value < 0, make current2 = 0
   }else{
      // Do nothing
   }

   Serial.print("Sensor Arus,     ADC : ");
   Serial.print(adc1); 
   Serial.print(" | ");
   Serial.print(volts1);
   Serial.print(" V | ");
   Serial.print(current);
   Serial.print(" A | ");
   Serial.print(current2);
   Serial.println(" A");
   delay(1);
}