#include <UTFT.h> 
#include <URTouch.h>
#include <UTFT_Buttons.h>
#include <Adafruit_ADS1X15.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <BH1750.h>
#include <MAX6675.h>
#include <RTClib.h>
#include <SPI.h>
#include <SD.h>

void backButton(String *, void (*)());
void drawMultimeter();
void drawPerformance();
void drawHomeScreen();
void drawSetting();

//=======================ESP Data Transfer=========================
String kirim = "";
bool otomatis, statusKirim = false, statusKirimM = false;
//=================================================================

//=======================RTC SD Card================================
RTC_DS1307 rtc;
File file;
char namaHari[7][12] = {"Minggu", "Senin", "Selasa", "Rabu", "Kamis", "Jumat", "Sabtu"};
String hari = "";
String waktu = "";
//==================================================================

// ================================= LCD TFT 3.2 ===================================
// Creating Objects
UTFT    myGLCD(ILI9341_16,38,39,40,41); 
URTouch  myTouch( 6, 5, 4, 3, 2);

// Fonts
extern uint8_t SmallFont[];
extern uint8_t BigFont[];
extern uint8_t SevenSegNumFont[];
extern uint8_t SevenSegmentFull[];
extern uint8_t SixteenSegment16x24[];
float estimasi = 0;

// Touch coordinate
int x, y;

// Keypad
char stCurrent[20]="";
int stCurrentLen=0;
char stLast1[20]="";
char stLast2[20]="";

// Menu
String currentPage;
char selectedUnit = '0',selectedUnitV = '0', selectedUnitW = '0', selectedUnitL = '0'; 

// Time Slider
const int valueSlider = 1;
int xR=38;
//======================================================================================

//===========Relay===========
int relay1 = 8;
int relay2 = 9;
//===========================

//=========Push Button=========
int pb = 10;
int pbState = 1;
//=============================

// ===== Creating Object ADS1115 =====
Adafruit_ADS1115 ads; // 
//====================================

//==========Voltage Divider===========
long adc0 = 0.0;
float R1 = 9752.0; //Resistor 1, 
float R2 = 206000.0; //Resistor 2
float volts0, voc = 0.0, volt = 0.0; 
//====================================

//==============ACS758===============
long adc1 = 0.0;
float volts1 = 0.0; 
float current = 0.0, current2 = 0.0;
//===================================

//============Suhu Termocouple===============
byte SOs = 11;
byte CSs = 12;
byte SCKs = 13;
MAX6675 suhuCel(CSs,SOs,SCKs,1);
MAX6675 suhuFah(CSs,SOs,SCKs,2);
//============================================

//===============Suhu DS18B20==================
OneWire pin_DS18B20(A0);
DallasTemperature DS18B20(&pin_DS18B20);
//=============================================

//==================millis==================
unsigned long millis1, millis2, millis3;
unsigned long startMillis;
unsigned long interval = 5000;
unsigned long manualStop = 15000;
unsigned long intervalOtomatis = 3600000;

byte fase = 0;
//==========================================


// ========Lux Meter========
int LUX;
BH1750 lightMeter;
//==========================

//======================================Data Variables===================================
float arusA, arusmA, voltV, voltmV, lux, calLux, iradiasi, dayaW, dayamW, performa;
float suhuPanelC, suhuPanelF, suhuLingkunganC, suhuLingkunganF;
int adc = 13300, calValue;
//=======================================================================================

void setup() {
  Serial.begin(9600);
  Serial3.begin(115200);
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  pinMode(pb, INPUT_PULLUP);
  digitalWrite(relay1, HIGH);
  digitalWrite(relay2, HIGH);
  Wire.begin();
  DS18B20.begin();
  lightMeter.begin();
  if (! rtc.begin()) {
    Serial.println("RTC TIDAK TERBACA");
  }
  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));//update rtc dari waktu komputer
  }
  if (!SD.begin(53)) {
    Serial.println("sd card gagal");
  }
  file = SD.open("OTOMATIS.txt", FILE_WRITE);
  file.close();
  file = SD.open("MANUAL.txt", FILE_WRITE);
  file.close();
  myGLCD.InitLCD();
  myGLCD.clrScr();
  myTouch.InitTouch();
  myTouch.setPrecision(PREC_MEDIUM);
  while (!ads.begin()) {
    drawBoot();
    Serial.println("Failed to initialize ADS.");
    delay(10);
  }
  myGLCD.clrScr();
  // Defining Pin Modes
  drawHomeScreen(); 
  currentPage = "0";
}

void loop() { 
  getTime();
  // Home Screen
  if (currentPage == "0") {
    if (myTouch.dataAvailable()) {
      myTouch.read();
      x=myTouch.getX(); // X coordinate where the screen has been pressed
      y=myTouch.getY(); // Y coordinates where the screen has been pressed

      // If we press the Performance Button 
      if ((x>=35) && (x<=285) && (y>=90) && (y<=130)) {
        drawFrame(35, 90, 285, 130);
        currentPage = "10";
        myGLCD.clrScr();
        drawPerformance();
      }
      // If we press the Multimeter Button 
      if ((x>=35) && (x<=285) && (y>=140) && (y<=180)) {
        drawFrame(35, 140, 285, 180);
        currentPage = "11";
        fase = 0;
        myGLCD.clrScr();
        drawMultimeter();
      }  
      // If we press Setting
      if ((x>=35) && (x<=285) && (y>=190) && (y<=230)) {
        drawFrame(35, 190, 285, 230);
        currentPage = "3";
        myGLCD.clrScr();
        drawSetting();
      }
    }
  }
// MANUAL
  if (currentPage == "1") {    
    if(digitalRead(pb) == 0){ 
      pbState = 0; // make pbstate to 0, to reset measurment from the start
    }
    if(pbState == 0){
      printManual(); // calling function to display sens value
      kirim = "";
      if(statusKirimM == true){
        // kirimManual();
        sendESP('M');
      }
    }
    if (myTouch.dataAvailable()) {
      myTouch.read();
      x=myTouch.getX();
      y=myTouch.getY();
      backButton("10", drawPerformance);
    }
  }
  
  // Otomatis
  if (currentPage == "2") {
    SliderOtomatis();
    if (myTouch.dataAvailable()) {
        myTouch.read();
        x=myTouch.getX();
        y=myTouch.getY();
              
      //Back button
      backButton("10", drawPerformance);
      // Mulai
      if ((x>=100) && (x<=216) && (y>=180) && (y<=215)) {
        drawFrame(100, 180, 216, 215); //drawframemulai
        currentPage = "4";
        myGLCD.clrScr();
        StopPengukuran();
      }
    }
  }
  // Setting
  if (currentPage =="3") {
    if (myTouch.dataAvailable()) {
      myTouch.read();
      x=myTouch.getX();
      y=myTouch.getY();
      backButton("0", drawHomeScreen);
      if ((x>=30) && (x<=290) && (y>=90) && (y<=130)) {
        drawFrame(30, 90, 290, 130);
        currentPage = "7";
        myGLCD.clrScr();
        SelectOption();
      }
      if ((x>=30) && (x<=290) && (y>=140) && (y<=180)) {
        drawFrame(30, 140, 290, 180);
        currentPage = "5";
        myGLCD.clrScr();
        drawAdcCal();
      }
    }    
  }
  // Hal otomatis mengukur
  if (currentPage == "4") {
    intervalOtomatis = estimasi * 16;
    intervalOtomatis *= 1000;
    otomatis = true;
    millis3 = millis();
    while (otomatis == true){
      getValueAll();
      kirim = "";
      if(statusKirim == true){
        // kirimOtomatis();
        sendESP('O');
      }
      if(millis() - millis3 > intervalOtomatis){ 
        otomatis = false;
        myGLCD.clrScr();
        currentPage = "0";
        drawHomeScreen();
      }
      if (myTouch.dataAvailable()) {
        myTouch.read();
        x=myTouch.getX();
        y=myTouch.getY();
            
      //Stop button
      
        if ((x>=35) && (x<=285) &&(y>=90) && (y<=130)) {
          drawFrame(35, 90, 285, 130);
          otomatis = false;
          currentPage = "0";
          // kirimOtomatis();
          sendESP('O');
          myGLCD.clrScr();
          drawHomeScreen();
        }
      }
    }
  }
  // Menu kalibrasi
  if (currentPage == "5") {
    digitalWrite(relay1, HIGH);
    digitalWrite(relay2, LOW);
    adcCal();
    if (myTouch.dataAvailable()) {
      myTouch.read();
      x=myTouch.getX();
      y=myTouch.getY();
      //Back button  
      backButton("3", drawSetting);
      // if we press simpan
      if ((x>=100) && (x<=216) && (y>=180) && (y<=215)) {
        drawFrame(100, 180, 216, 215); //drawframesimpan
        currentPage = "3";
        digitalWrite(relay2, HIGH);
        myGLCD.clrScr();
        drawSetting();
      }
    }
  }
  if (currentPage == "6") {
    if (myTouch.dataAvailable()) {
      myTouch.read();
      x=myTouch.getX();
      y=myTouch.getY();
      //Back button
      backButton("0", drawHomeScreen);
    }
  }
  if (currentPage == "7") {
    if (myTouch.dataAvailable()) {
      myTouch.read();
      x=myTouch.getX();
      y=myTouch.getY();

      if ((x>=250) && (x<=290) && (y>=120) && (y<=140)) {
        drawFrame(250, 120, 290, 140);
        currentPage = "8";
        myGLCD.clrScr();
        myGLCD.setBackColor(12, 171, 182);
        drawButtons();  
      }
    
      if ((x>=250) && (x<=290) && (y>=190) && (y<=210)) {
        drawFrame(250, 190, 290, 210);
        currentPage = "9";
        myGLCD.clrScr();
        myGLCD.setBackColor(12, 171, 182);
        drawButtons();  
      }
      backButton("3", drawSetting);
    }
  }
  if (currentPage == "8") {
    delay(50);
    if (myTouch.dataAvailable()){
      myTouch.read();
      x=myTouch.getX();
      y=myTouch.getY();
      
      if ((y>=60) && (y<=110))  // Upper row
      {
        if ((x>=10) && (x<=60))  // Button: 1
        {
          waitForIt(10, 60, 60, 110);
          updateStr('1');
        }
        if ((x>=70) && (x<=120))  // Button: 2
        {
          waitForIt(70, 60, 120, 110);
          updateStr('2');
        }
        if ((x>=130) && (x<=180))  // Button: 3
        {
          waitForIt(130, 60, 180, 110);
          updateStr('3');
        }
        if ((x>=190) && (x<=240))  // Button: 4
        {
          waitForIt(190, 60, 240, 110);
          updateStr('4');
        }
        if ((x>=250) && (x<=300))  // Button: 5
        {
          waitForIt(250, 60, 300, 110);
          updateStr('5');
        }
      }

      if ((y>=120) && (y<=170))  // Center row
      {
        if ((x>=10) && (x<=60))  // Button: 6
        {
          waitForIt(10, 120, 60, 170);
          updateStr('6');
        }
        if ((x>=70) && (x<=120))  // Button: 7
        {
          waitForIt(70, 120, 120, 170);
          updateStr('7');
        }
        if ((x>=130) && (x<=180))  // Button: 8
        {
          waitForIt(130, 120, 180, 170);
          updateStr('8');
        }
        if ((x>=190) && (x<=240))  // Button: 9
        {
          waitForIt(190, 120, 240, 170);
          updateStr('9');
        }
        if ((x>=250) && (x<=300))  // Button: 0
        {
          waitForIt(250, 120, 300, 170);
          updateStr('0');
        }
      }

      if ((y>=180) && (y<=230))  // Upper row
      {
        if ((x>=130) && (x<=180))  // Button: .
        {
          waitForIt(130, 180, 180, 230);
          updateStr('.');
        }
        if ((x>=10) && (x<=120))  // Button: Clear
        {
          waitForIt(10, 180, 120, 230);
          stCurrent[0]='\0';
          stCurrentLen=0;
          myGLCD.setColor(0, 0, 0);
          myGLCD.fillRect(0, 25, 245, 50);
        }
        if ((x>=190) && (x<=300))  // Button: Enter
        {
          waitForIt(190, 180, 300, 230);
          if (stCurrentLen>0)
          {
            for (x=0; x<stCurrentLen+1; x++)
            {
              stLast1[x]=stCurrent[x];
            }
            stCurrent[0]='\0';
            stCurrentLen=0;
            myGLCD.setColor(0, 0, 0);
            myGLCD.fillRect(0, 5, 245, 25);
            myGLCD.setColor(0, 255, 0);
            myGLCD.setFont(BigFont);
            myGLCD.print(stLast1, LEFT, 8);
            currentPage = "7";
            myGLCD.clrScr();
            SelectOption();
          }
          else
          {
            myGLCD.setFont(SmallFont);
            myGLCD.setColor(255, 0, 0);
            myGLCD.print("BUFFER EMPTY", CENTER, 10);
            delay(500);
            myGLCD.print("            ", CENTER, 10);
            delay(500);
            myGLCD.print("BUFFER EMPTY", CENTER, 10);
            delay(500);
            myGLCD.print("            ", CENTER, 10);
            myGLCD.setColor(0, 255, 0);
          }
        }
      }
      if ((x>=250) && (x<=300) &&(y>=10) && (y<=60)) {
        drawFrame(250, 10, 300, 36);
        currentPage = "7";
        myGLCD.clrScr();
        SelectOption();
         
      }
    }
  }
  if (currentPage == "9") {
    delay(50);
    if (myTouch.dataAvailable()){
      myTouch.read();
      x=myTouch.getX();
      y=myTouch.getY();
      if ((y>=60) && (y<=110))  // Upper row
      {
        if ((x>=10) && (x<=60))  // Button: 1
        {
          waitForIt(10, 60, 60, 110);
          updateStr('1');
        }
        if ((x>=70) && (x<=120))  // Button: 2
        {
          waitForIt(70, 60, 120, 110);
          updateStr('2');
        }
        if ((x>=130) && (x<=180))  // Button: 3
        {
          waitForIt(130, 60, 180, 110);
          updateStr('3');
        }
        if ((x>=190) && (x<=240))  // Button: 4
        {
          waitForIt(190, 60, 240, 110);
          updateStr('4');
        }
        if ((x>=250) && (x<=300))  // Button: 5
        {
          waitForIt(250, 60, 300, 110);
          updateStr('5');
        }
      }

      if ((y>=120) && (y<=170))  // Center row
      {
        if ((x>=10) && (x<=60))  // Button: 6
        {
          waitForIt(10, 120, 60, 170);
          updateStr('6');
        }
        if ((x>=70) && (x<=120))  // Button: 7
        {
          waitForIt(70, 120, 120, 170);
          updateStr('7');
        }
        if ((x>=130) && (x<=180))  // Button: 8
        {
          waitForIt(130, 120, 180, 170);
          updateStr('8');
        }
        if ((x>=190) && (x<=240))  // Button: 9
        {
          waitForIt(190, 120, 240, 170);
          updateStr('9');
        }
        if ((x>=250) && (x<=300))  // Button: 0
        {
          waitForIt(250, 120, 300, 170);
          updateStr('0');
        }
      }

      if ((y>=180) && (y<=230))  // Upper row
      {
        if ((x>=130) && (x<=180))  // Button: .
        {
          waitForIt(130, 180, 180, 230);
          updateStr('.');
        }
        if ((x>=10) && (x<=120))  // Button: Clear
        {
          waitForIt(10, 180, 120, 230);
          stCurrent[0]='\0';
          stCurrentLen=0;
          myGLCD.setColor(0, 0, 0);
          myGLCD.fillRect(0, 25, 245, 50);
        }
        if ((x>=190) && (x<=300))  // Button: Enter
        {
          waitForIt(190, 180, 300, 230);
          if (stCurrentLen>0)
          {
            for (x=0; x<stCurrentLen+1; x++)
            {
              stLast2[x]=stCurrent[x];
            }
            stCurrent[0]='\0';
            stCurrentLen=0;
            myGLCD.setColor(0, 0, 0);
            myGLCD.fillRect(0, 5, 245, 25);
            myGLCD.setColor(0, 255, 0);
            myGLCD.setFont(BigFont);
            myGLCD.print(stLast2, LEFT, 8);
            currentPage = "7";
            myGLCD.clrScr();
            SelectOption();
          }
          else{
            myGLCD.setColor(255, 0, 0);
            myGLCD.setFont(SmallFont);
            myGLCD.print("BUFFER EMPTY", CENTER, 10);
            delay(500);
            myGLCD.print("            ", CENTER, 10);
            delay(500);
            myGLCD.print("BUFFER EMPTY", CENTER, 10);
            delay(500);
            myGLCD.print("            ", CENTER, 10);
            myGLCD.setColor(0, 255, 0);
          }
        }
      }

      if ((x>=250) && (x<=300) &&(y>=10) && (y<=60)) {
        drawFrame(250, 10, 300, 36);
        currentPage = "7";
        myGLCD.clrScr();
        SelectOption();
      }
    }
  }

  // Performance
  if (currentPage == "10") {    
    if (myTouch.dataAvailable()) {
      myTouch.read();
      x=myTouch.getX();
      y=myTouch.getY();
      // If we press the Manual Button 
      if ((x>=35) && (x<=285) && (y>=90) && (y<=130)) {
        drawFrame(35, 90, 285, 130);
        currentPage = "1"; 
        myGLCD.clrScr(); 
        drawManual(); 
      }
      // If we press the Otomatis Button 
      if ((x>=35) && (x<=285) && (y>=140) && (y<=180)) {
        drawFrame(35, 140, 285, 180);
        currentPage = "2";
        myGLCD.clrScr();
        drawOtomatis();
      }
      backButton("0", drawHomeScreen);
    }
  }
  // Multimeter
  if (currentPage == "11") {  
    digitalWrite(relay1,HIGH);
    digitalWrite(relay2,LOW);
    getNilaiMulti(); // calling function to display sens value
    if (myTouch.dataAvailable()) {
      myTouch.read();
      x=myTouch.getX();
      y=myTouch.getY();
      
      // If we press Arus
      if ((x>=5) && (x<=57) &&(y>=70) && (y<=100)) {
        drawFrame(5, 70, 57, 100);
        currentPage = "12";
        myGLCD.clrScr();
        drawArus(); // Draws the Arus Menu
      }
        // If we press Tegangan
      if ((x>=62) && (x<=134) &&(y>=70) && (y<=100)) {
        drawFrame(62, 70, 134, 100);
        currentPage = "13"; 
        myGLCD.clrScr();
        drawTegangan(); // Draws the Tegangan Menu
      }
          // If we press Daya
      if ((x>=139) && (x<=212) &&(y>=70) && (y<=100)) {
        drawFrame(139, 70, 212, 100);
        currentPage = "14";
        myGLCD.clrScr();
        drawDaya(); // Draws the daya Menu
      }
        // If we press Lux
      if ((x>=217) && (x<=270) &&(y>=70) && (y<=100)) {
        drawFrame(217, 70, 270, 100);
        currentPage = "15";
        myGLCD.clrScr();
        drawLux(); // Draws the Lux Menu
      }
      // If we press Suhu
      if ((x>=275) && (x<=315) &&(y>=70) && (y<=100)) {
        drawFrame(275, 70, 315, 100);
        currentPage = "16";
        myGLCD.clrScr();
        drawSuhu(); // Draws the Suhu Menu
      }
      backButton("0", drawHomeScreen);
    }
  }
  //Multiimeter Arus
  if (currentPage == "12") {    
    digitalWrite(relay1,HIGH);
    digitalWrite(relay2,LOW);
    getArus(); 
    Serial.println(adc1);
    Serial.println(arusA);
    if (myTouch.dataAvailable()) {
      myTouch.read();
      x=myTouch.getX();
      y=myTouch.getY();
      
      // If we press A
      if ((x>=10) && (x<=90) &&(y>=135) && (y<=163)) {
        drawFrame(10, 135, 90, 163);          
        selectedUnit = '0';
        myGLCD.setColor(0, 0, 0);
        myGLCD.fillRect(110, 140, 310, 210);
      }
      // If we press mA
      if ((x>=10) && (x<=90) &&(y>=173) && (y<=201)) {
        drawFrame(10, 173, 90, 201);
        selectedUnit = '1';
        myGLCD.setColor(0, 0, 0);
        myGLCD.fillRect(110, 140, 310, 210);
      }
      backButton("11", drawMultimeter);
    }
  }
  //Multiimeter Tegangan
  if (currentPage == "13") {    
    digitalWrite(relay2,HIGH);
    digitalWrite(relay1,LOW);
    getTegangan();
    if (myTouch.dataAvailable()) {
      myTouch.read();
      x=myTouch.getX();
      y=myTouch.getY();
      
      // If we press V
      if ((x>=10) && (x<=90) &&(y>=135) && (y<=163)) {
        drawFrame(10, 135, 90, 163);
        selectedUnitV = '0';
          myGLCD.setColor(0, 0, 0);
        myGLCD.fillRect(110, 140, 310, 210);
      }
      // If we press mV
      if ((x>=10) && (x<=90) &&(y>=173) && (y<=201)) {
          drawFrame(10, 173, 90, 201);
        selectedUnitV = '1';
          myGLCD.setColor(0, 0, 0);
        myGLCD.fillRect(110, 140, 310, 210);
      }
      // If we press the Back Button
      backButton("11", drawMultimeter);
    }
  }
  //Multiimeter Daya
  if (currentPage == "14") {
    digitalWrite(relay1,HIGH);
    digitalWrite(relay2,LOW);
    getDaya();
    if (myTouch.dataAvailable()) {
      myTouch.read();
      x=myTouch.getX();
      y=myTouch.getY();
      
      // If we press W
      if ((x>=10) && (x<=90) &&(y>=135) && (y<=163)) {
        drawFrame(10, 135, 90, 163);
        selectedUnitW = '0';
          myGLCD.setColor(0, 0, 0);
        myGLCD.fillRect(110, 140, 310, 210);
          
      }
      // If we press mW
      if ((x>=10) && (x<=90) &&(y>=173) && (y<=201)) {
          drawFrame(10, 173, 90, 201);
        selectedUnitW = '1';
          myGLCD.setColor(0, 0, 0);
        myGLCD.fillRect(110, 140, 310, 210);
      }
      // If we press the Back Button
      backButton("11", drawMultimeter);
    }
  }
  //Multiimeter Lux
  if (currentPage == "15") {    
    getLux();
    if (myTouch.dataAvailable()) {
      myTouch.read();
      x=myTouch.getX();
      y=myTouch.getY();
      
      // If we press lux
      if ((x>=10) && (x<=90) &&(y>=135) && (y<=163)) {
        drawFrame(10, 135, 90, 163);
        selectedUnitL = '0';
        myGLCD.setColor(0, 0, 0);
        myGLCD.fillRect(110, 140, 315, 230);
          
      }
      // If we press w/m2
      if ((x>=10) && (x<=90) &&(y>=173) && (y<=201)) {
        drawFrame(10, 173, 90, 201);
        selectedUnitL = '1';
        myGLCD.setColor(0, 0, 0);
        myGLCD.fillRect(110, 140, 315, 230);
      }
      // If we press the Back Button
      backButton("11", drawMultimeter);
    }
  }
  //Multiimeter Suhu
  if (currentPage == "16") {    
    getSuhu();
    if (myTouch.dataAvailable()) {
      myTouch.read();
      x=myTouch.getX();
      y=myTouch.getY();
      
      // If we press C
      if ((x>=10) && (x<=90) &&(y>=135) && (y<=163)) {
        drawFrame(10, 135, 90, 163);
        selectedUnitL = '0';
        myGLCD.setColor(0, 0, 0);
        myGLCD.fillRect(118, 135, 315, 162);
        myGLCD.fillRect(118, 183, 315, 220);
          
      }
      // If we press F
      if ((x>=10) && (x<=90) &&(y>=173) && (y<=201)) {
        drawFrame(10, 173, 90, 201);
        selectedUnitL = '1';
        myGLCD.setColor(0, 0, 0);
        myGLCD.fillRect(118, 135, 315, 162);
        myGLCD.fillRect(118, 183, 315, 220);
      }
      // If we press the Back Button
      backButton("11", drawMultimeter);
    }
  }
}

// ====== Funtions ======
// ==================================================== Menu =========================================================
// Boot =======================================================================================================
void drawBoot() {
  myGLCD.setBackColor(0,0,0);
  myGLCD.setColor(255, 255, 255);
  myGLCD.setFont(BigFont);
  myGLCD.print("LOADING SETUP", CENTER, 120);
}

// Home Screen ===============================================================================================
void drawHomeScreen() {
  otomatis = false;
  digitalWrite(relay1, HIGH);
  digitalWrite(relay2, HIGH);
  pbState = 1;
  if(fase == 1 || fase == 2 || fase == 3){
    fase = 0;
  }
  // Title
  myGLCD.setBackColor(0,0,0);
  myGLCD.setColor(255, 255, 255);
  myGLCD.setFont(BigFont);
  myGLCD.print("PERFORMANCE TESTER", CENTER, 10);
  myGLCD.setColor(102, 255, 255);
  myGLCD.drawLine(0,32,319,32);
  myGLCD.setColor(255, 255, 255);
  myGLCD.setFont(SmallFont);
  myGLCD.print("Panel Photovoltaic", CENTER, 41);
  myGLCD.setFont(BigFont);
  myGLCD.print("Select Mode", CENTER, 64);

  // Button - Performance Tester 
  myGLCD.setColor(12, 171, 182);
  myGLCD.fillRoundRect (35, 90, 285, 130); 
  myGLCD.setColor(255, 255, 255); 
  myGLCD.drawRoundRect (35, 90, 285, 130); 
  myGLCD.setFont(BigFont); 
  myGLCD.setBackColor(12, 171, 182); 
  myGLCD.print("PERFORMANCE", CENTER, 102);
  
// Button - MULTIMETER
  myGLCD.setColor(12, 171, 182);
  myGLCD.fillRoundRect (35, 140, 285, 180);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (35, 140, 285, 180);
  myGLCD.setFont(BigFont);
  myGLCD.setBackColor(12, 171, 182);
  myGLCD.print("MULTIMETER", CENTER, 152);
  
  // Button - Setting
  myGLCD.setColor(12, 171, 182);
  myGLCD.fillRoundRect (35, 190, 285, 230);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (35, 190, 285, 230);
  myGLCD.setFont(BigFont);
  myGLCD.setBackColor(12, 171, 182);
  myGLCD.print("PENGATURAN", CENTER, 202);
  
}


// Menu Performance Tester =====================================================================================
void drawPerformance() {
  myGLCD.setColor(100, 155, 203);
  myGLCD.fillRoundRect (10, 10, 60, 36);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (10, 10, 60, 36);
  myGLCD.setFont(BigFont);
  myGLCD.setBackColor(100, 155, 203);
  myGLCD.print("<-", 18, 15);
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setFont(SmallFont);
  myGLCD.print("Back to Main Menu", 70, 18);
  
  // Button - Manual 
  myGLCD.setColor(12, 171, 182);
  myGLCD.fillRoundRect (35, 90, 285, 130);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (35, 90, 285, 130);
  myGLCD.setFont(BigFont);
  myGLCD.setBackColor(12, 171, 182);
  myGLCD.print("MANUAL", CENTER, 102);
  
  // Button - Otomatis
  myGLCD.setColor(12, 171, 182);
  myGLCD.fillRoundRect (35, 140, 285, 180);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (35, 140, 285, 180);
  myGLCD.setFont(BigFont);
  myGLCD.setBackColor(12, 171, 182);
  myGLCD.print("OTOMATIS", CENTER, 152);
}

// Menu Manual ======================================================================================
void drawManual() {
  myGLCD.setColor(100, 155, 203);
  myGLCD.fillRoundRect (10, 10, 60, 36);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (10, 10, 60, 36);
  myGLCD.setFont(BigFont);
  myGLCD.setBackColor(100, 155, 203);
  myGLCD.print("<-", 18, 15);
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setFont(SmallFont);
  myGLCD.print("Back to Performance Menu", 70, 18);
  myGLCD.setFont(BigFont);
  myGLCD.print("PENGUKURAN MANUAL", CENTER, 50);
  myGLCD.setColor(102, 255, 255);
  myGLCD.drawLine(0,70,319,70);
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setColor(255, 255, 255);
  myGLCD.setFont(SmallFont);
  myGLCD.print("TEGANGAN (Voc)  :", 10, 80);
  myGLCD.print("V", 293, 80);
  myGLCD.print("ARUS (Isc)      :", 10, 105);
  myGLCD.print("A", 293, 105);
  myGLCD.print("SUHU LINGKUNGAN :", 10, 130);
  myGLCD.print("C", 293, 130);
  myGLCD.print("SUHU PANEL      :", 10, 155);
  myGLCD.print("C", 293, 155);
  myGLCD.print("IRADIASI        :", 10, 180);
  myGLCD.print("W/M^2", 275, 180);
  myGLCD.print("PERFORMA PANEL  :", 10, 205);
  myGLCD.print("%", 293, 205);
}

// Menu Otomatis =====================================================================================
void drawOtomatis() {
  myGLCD.setColor(100, 155, 203);
  myGLCD.fillRoundRect (10, 10, 60, 36);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (10, 10, 60, 36);
  myGLCD.setFont(BigFont);
  myGLCD.setBackColor(100, 155, 203);
  myGLCD.print("<-", 18, 15);
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setFont(SmallFont);
  myGLCD.print("Back to Performance Menu", 70, 18);
  myGLCD.setFont(BigFont);
  myGLCD.print("Pengukuran Otomatis", CENTER, 50);
 
  myGLCD.print("Waktu :", 50, 95);
  myGLCD.print("Jam ", 220, 95);
  myGLCD.print("T", 10, 135);
  myGLCD.drawLine(0,75,319,75); 
  myGLCD.drawRect(30, 138, 310, 148);
  
  myGLCD.setColor(100, 155, 203);
  myGLCD.fillRoundRect (100, 180, 216, 215);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (100, 180, 216, 215);
  myGLCD.setBackColor(100, 155, 203);
  myGLCD.setFont(BigFont);
  myGLCD.print("MULAI", 120, 190);
}


// Menu Pengaturan =================================================================================
void drawSetting(){
  myGLCD.setColor(100, 155, 203);
  myGLCD.fillRoundRect (10, 10, 60, 36);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (10, 10, 60, 36);
  myGLCD.setFont(BigFont);
  myGLCD.setBackColor(100, 155, 203);
  myGLCD.print("<-", 18, 15);
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setFont(SmallFont);
  myGLCD.print("Back to Main Menu", 70, 18);
  myGLCD.setFont(BigFont);
  myGLCD.print("Pengaturan", CENTER, 50);
  myGLCD.setColor(102, 255, 255);
  myGLCD.drawLine(0,75,319,75); 
  myGLCD.setColor(12, 171, 182);
  myGLCD.fillRoundRect (30, 90, 290, 130);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (30, 90, 290, 130);
  myGLCD.setFont(BigFont);
  myGLCD.setBackColor(12, 171, 182);
  myGLCD.print("Data Panel", CENTER, 100);
  myGLCD.setColor(12, 171, 182);
  myGLCD.fillRoundRect (30, 140, 290, 180);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (30, 140, 290, 180);
  myGLCD.setFont(BigFont);
  myGLCD.setBackColor(12, 171, 182);
  myGLCD.print("Kalibrasi", CENTER, 150);
}

// Slider Waktu Otomatis ================================================================================
void SliderOtomatis(){
  if (myTouch.dataAvailable()) {
    myTouch.read();
    x=myTouch.getX();
    y=myTouch.getY();        
    if( (y>=130) && (y<=156)) {
      xR=x;
      if (xR<=38) {
        xR=38;
      }
      if (xR>=303){
        xR=303;
      }
    }
  }
  int xRC = map(xR,38,310,0,255);

  estimasi = (float (xRC)/25.0 ) + 1; //convert 0 - 255, to 1 - 10
  estimasi = int (estimasi);
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setColor(255, 255, 255);
  myGLCD.setFont(BigFont);
  myGLCD.printNumI(estimasi,165, 95, 2);
  myGLCD.setColor(255, 255, 255);
  myGLCD.fillRect(xR,139,(xR+4),147); // Positioner
  myGLCD.setColor(0, 0, 0);
  myGLCD.fillRect(31, 139, (xR-1), 147);
  myGLCD.setColor(0, 0, 0);
  myGLCD.fillRect((xR+5), 139, 309, 147);

 }

// Button Stop Pengukuran Otomatis ==========================================================
void StopPengukuran(){
  myGLCD.setColor(12, 171, 182); 
  myGLCD.fillRoundRect (35, 90, 285, 130); 
  myGLCD.setColor(255, 255, 255); 
  myGLCD.drawRoundRect (35, 90, 285, 130); 
  myGLCD.setFont(BigFont); 
  myGLCD.setBackColor(12, 171, 182); 
  myGLCD.print("STOP PENGUKURAN", CENTER, 102); 
}

// Menu Input Data Panel ====================================================
void SelectOption(){
  myGLCD.setColor(100, 155, 203);
  myGLCD.fillRoundRect (10, 10, 60, 36);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (10, 10, 60, 36);
  myGLCD.setFont(BigFont);
  myGLCD.setBackColor(100, 155, 203);
  myGLCD.print("<-", 18, 15);
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setFont(BigFont);
  myGLCD.print("INPUT DATA PANEL", CENTER, 50);
  myGLCD.setColor(102, 255, 255);
  myGLCD.drawLine(0,70,319,70);
  myGLCD.drawLine(10,110,138,110);
  myGLCD.drawLine(10,110,138,110); //linetegangan
  myGLCD.drawLine(10,180,74,180); //linearus
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setColor(255, 255, 255);
  myGLCD.setFont(BigFont);
  myGLCD.print("TEGANGAN", 10, 90);
  myGLCD.print("Voc   :", 10, 120);
  myGLCD.print(stLast1, 130, 120);
  myGLCD.print("V", 220, 120);
  myGLCD.print("ARUS", 10, 160);
  myGLCD.print("Isc   :", 10,190);
  myGLCD.print(stLast2, 130, 190);
  myGLCD.print("A", 220, 190);
  
  myGLCD.setFont(SmallFont);
  myGLCD.setColor(100, 155, 203);
  myGLCD.fillRoundRect (250, 120, 290, 140);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (250, 120, 290, 140);
  myGLCD.setFont(SmallFont);
  myGLCD.setBackColor(100, 155, 203);
  myGLCD.print("...", 260, 125);

  myGLCD.setColor(100, 155, 203);
  myGLCD.fillRoundRect (250, 190, 290, 210);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (250, 190, 290, 210);
  myGLCD.setFont(SmallFont);
  myGLCD.setBackColor(100, 155, 203);
  myGLCD.print("...", 260, 195);
}

// Menu Multimeter ==========================================================
void drawMultimeter() {
  myGLCD.setColor(100, 155, 203);
  myGLCD.fillRoundRect (10, 10, 60, 36);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (10, 10, 60, 36);
  myGLCD.setFont(BigFont);
  myGLCD.setBackColor(100, 155, 203);
  myGLCD.print("<-", 18, 15);
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setFont(SmallFont);
  myGLCD.print("Back to Main Menu", 70, 18);

  myGLCD.setFont(BigFont);
  myGLCD.print("Multimeter", CENTER, 45);
  myGLCD.setColor(102, 255, 255);
  myGLCD.drawLine(0,110,319,110);
  myGLCD.setBackColor(0, 0, 0);
 

 //Arus
  myGLCD.setColor(100, 155, 203);
  myGLCD.fillRoundRect (5, 70, 57, 100);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (5, 70, 57, 100);
  myGLCD.setBackColor(100, 155, 203);
  myGLCD.print("Amp", 7, 77);

  //Tegangan
  myGLCD.setColor(100, 155, 203);
  myGLCD.fillRoundRect (62, 70, 134, 100);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (62, 70, 134, 100);
  myGLCD.print("Volt",65, 77);

  //Daya
  myGLCD.setColor(100, 155, 203);
  myGLCD.fillRoundRect (139, 70, 212, 100);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (139, 70, 212, 100);
  myGLCD.print("Watt", 142, 77);
  
  //Lux
  myGLCD.setColor(100, 155, 203);
  myGLCD.fillRoundRect (217, 70, 270, 100);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (217, 70, 270, 100);
  myGLCD.print("Lux", 219, 77);

  //Suhu
  myGLCD.setColor(100, 155, 203);
  myGLCD.fillRoundRect (275, 70, 315, 100);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (275, 70, 315, 100);
  myGLCD.print("T", 285, 77);
  
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setColor(255, 255, 255);
  myGLCD.setFont(SmallFont);
  myGLCD.print("Arus (I)      :", 10, 120);
  myGLCD.print("Tegangan (V)  :", 10, 145);
  myGLCD.print("Daya (W)      :", 10, 170);
  myGLCD.print("I.Cahaya (Lux):", 10, 195);
  myGLCD.print("S.Lingkungan  :", 10, 220);
}
// ========================================================================================================

// ======================================= Keypad ========================================
void updateStr(int val)
{
  if (stCurrentLen<5)
  {
    stCurrent[stCurrentLen]=val;
    stCurrent[stCurrentLen+1]='\0';
    stCurrentLen++;
    myGLCD.setFont(BigFont);
    myGLCD.setColor(255, 255, 255);
    myGLCD.print(stCurrent, LEFT, 30);
    
  }
  else
  {
    myGLCD.setFont(SmallFont);
    myGLCD.setColor(255, 0, 0);
    myGLCD.print("BUFFER FULL!", CENTER, 10);
    delay(500);
    myGLCD.print("            ", CENTER, 10);
    delay(500);
    myGLCD.print("BUFFER FULL!", CENTER, 10);
    delay(500);
    myGLCD.print("            ", CENTER, 10);
    myGLCD.setColor(0, 255, 0);
  }
}

// Draw a red frame while a button is touched
void waitForIt(int x1, int y1, int x2, int y2)
{
  myGLCD.setColor(255, 0, 0);
  myGLCD.drawRoundRect (x1, y1, x2, y2);
  while (myTouch.dataAvailable())
    myTouch.read();
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (x1, y1, x2, y2);
}

void drawButtons(){
   myGLCD.setColor(12, 171, 182);
   myGLCD.fillRoundRect (250, 10, 300, 36);
   myGLCD.setColor(255, 255, 255);
   myGLCD.drawRoundRect (250, 10, 300, 36);
   myGLCD.setFont(SmallFont);
   myGLCD.print("BACK", 259, 18);
   // Draw the upper row of buttons
   for (x=0; x<5; x++){
      myGLCD.setColor(12, 171, 182);
      myGLCD.fillRoundRect (10+(x*60), 60, 60+(x*60), 110);
      myGLCD.setColor(255, 255, 255);
      myGLCD.drawRoundRect (10+(x*60), 60, 60+(x*60), 110);
      myGLCD.setFont(BigFont);
      myGLCD.printNumI(x+1, 27+(x*60), 77);
   }
   // Draw the center row of buttons
   for (x=0; x<5; x++){
      myGLCD.setColor(12, 171, 182);
      myGLCD.fillRoundRect (10+(x*60), 120, 60+(x*60), 170);
      myGLCD.setColor(255, 255, 255);
      myGLCD.drawRoundRect (10+(x*60), 120, 60+(x*60), 170);
      if (x<4)
      myGLCD.setFont(BigFont);
      myGLCD.printNumI(x+6, 27+(x*60), 137);
   }   
   myGLCD.setColor(12, 171, 182);
   myGLCD.fillRoundRect (250, 120, 300, 170) ;
   myGLCD.setColor(255, 255, 255);
   myGLCD.drawRoundRect (250, 120, 300, 170);
   myGLCD.print("0", 267, 137);
   // Draw the lower row of buttons
   myGLCD.setColor(12, 171, 182);
   myGLCD.fillRoundRect (10, 180, 120, 230);
   myGLCD.setColor(255, 255, 255);
   myGLCD.drawRoundRect (10, 180, 120, 230);
   myGLCD.setFont(BigFont);
   myGLCD.print("Clear", 30, 200);

   myGLCD.setColor(12, 171, 182);
   myGLCD.fillRoundRect (130, 180, 180, 230);
   myGLCD.setColor(255, 255, 255);
   myGLCD.drawRoundRect (130, 180, 180, 230);
   myGLCD.setFont(BigFont);
   myGLCD.print(".", 148, 200);
      
   myGLCD.setColor(12, 171, 182);
   myGLCD.fillRoundRect (190, 180, 300, 230);
   myGLCD.setColor(255, 255, 255);
   myGLCD.drawRoundRect (190, 180, 300, 230);
   myGLCD.setFont(BigFont);
   myGLCD.print("Enter", 200, 200);
   myGLCD.setBackColor (0, 0, 0);
}
// ===============================================================================================================

// ===================================== Menampilkan niilai Hasil Pengukuran =====================================
void printManual() {
  getValueAll();
  myGLCD.setFont(SmallFont);
  myGLCD.setColor(0, 255, 0);
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.printNumF(voltV, 2, 150, 80,'.', 8);
  myGLCD.printNumF(arusA, 2, 150, 105,'.', 8);
  myGLCD.printNumF(suhuLingkunganC, 2, 150, 130,'.', 8);
  myGLCD.printNumF(suhuPanelC, 2, 150, 155,'.', 8);
  myGLCD.printNumF(iradiasi, 2, 150, 180,'.', 8);
  myGLCD.printNumF(performa, 2, 150, 205,'.', 8);
}

void drawArus (){
  myGLCD.setColor(100, 155, 203);
  myGLCD.fillRoundRect (10, 10, 60, 36);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (10, 10, 60, 36);
  myGLCD.setFont(BigFont);
  myGLCD.setBackColor(100, 155, 203);
  myGLCD.print("<-", 18, 15);
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setFont(SmallFont);
  myGLCD.print("Back to Multimeter Menu", 70, 18);
  myGLCD.setFont(BigFont);
  myGLCD.print("Multimeter", CENTER, 45);
  myGLCD.print("Arus", CENTER, 76);
  myGLCD.setColor(102, 255, 255);
  myGLCD.drawLine(0,100,319,100);
  myGLCD.setBackColor(0, 0, 0);
  
  myGLCD.setColor(255, 255, 255);
  myGLCD.setFont(SmallFont);
  myGLCD.print("Select Unit", 10, 114);
  myGLCD.setFont(BigFont);
  myGLCD.print("Hasil:", 120, 120);
  myGLCD.setColor(223, 77, 55);
  myGLCD.fillRoundRect (10, 135, 90, 163);
  myGLCD.setColor(225, 255, 255);
  myGLCD.drawRoundRect (10, 135, 90, 163);
  myGLCD.setBackColor(223, 77, 55);
  myGLCD.setColor(255, 255, 255);
  myGLCD.print("A", 40, 140);
  myGLCD.setColor(223, 77, 55);
  myGLCD.fillRoundRect (10, 173, 90, 201);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (10, 173, 90, 201);
  myGLCD.setBackColor(223, 77, 55);
  myGLCD.setColor(255, 255, 255);
  myGLCD.print("mA", 24, 180);
}

void drawTegangan (){
  myGLCD.setColor(100, 155, 203);
  myGLCD.fillRoundRect (10, 10, 60, 36);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (10, 10, 60, 36);
  myGLCD.setFont(BigFont);
  myGLCD.setBackColor(100, 155, 203);
  myGLCD.print("<-", 18, 15);
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setFont(SmallFont);
  myGLCD.print("Back to Multimeter Menu", 70, 18);
  myGLCD.setFont(BigFont);
  myGLCD.print("Multimeter", CENTER, 45);
  myGLCD.print("Tegangan", CENTER, 76);
  myGLCD.setColor(102, 255, 255);
  myGLCD.drawLine(0,100,319,100);
  myGLCD.setBackColor(0, 0, 0);
  
  myGLCD.setColor(255, 255, 255);
  myGLCD.setFont(SmallFont);
  myGLCD.print("Select Unit", 10, 114);
  myGLCD.setFont(BigFont);
  myGLCD.print("Hasil:", 120, 120);
  myGLCD.setColor(223, 77, 55);
  myGLCD.fillRoundRect (10, 135, 90, 163);
  myGLCD.setColor(225, 255, 255);
  myGLCD.drawRoundRect (10, 135, 90, 163);
  myGLCD.setBackColor(223, 77, 55);
  myGLCD.setColor(255, 255, 255);
  myGLCD.print("V", 40, 140);
  myGLCD.setColor(223, 77, 55);
  myGLCD.fillRoundRect (10, 173, 90, 201);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (10, 173, 90, 201);
  myGLCD.setBackColor(223, 77, 55);
  myGLCD.setColor(255, 255, 255);
  myGLCD.print("mV", 24, 180);
  
}

void drawDaya (){
  myGLCD.setColor(100, 155, 203);
  myGLCD.fillRoundRect (10, 10, 60, 36);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (10, 10, 60, 36);
  myGLCD.setFont(BigFont);
  myGLCD.setBackColor(100, 155, 203);
  myGLCD.print("<-", 18, 15);
   
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setFont(SmallFont);
  myGLCD.print("Back to Multimeter Menu", 70, 18);
  myGLCD.setFont(BigFont);
  myGLCD.print("Multimeter", CENTER, 45);
  myGLCD.print("Daya", CENTER, 76);
  myGLCD.setColor(102, 255, 255);
  myGLCD.drawLine(0,100,319,100);
  myGLCD.setBackColor(0, 0, 0);
  
  myGLCD.setColor(255, 255, 255);
  myGLCD.setFont(SmallFont);
  myGLCD.print("Select Unit", 10, 114);
  myGLCD.setFont(BigFont);
  myGLCD.print("Hasil:", 120, 120);
  myGLCD.setColor(223, 77, 55);
  myGLCD.fillRoundRect (10, 135, 90, 163);
  myGLCD.setColor(225, 255, 255);
  myGLCD.drawRoundRect (10, 135, 90, 163);
  myGLCD.setBackColor(223, 77, 55);
  myGLCD.setColor(255, 255, 255);
  myGLCD.print("W", 40, 140);
  myGLCD.setColor(223, 77, 55);
  myGLCD.fillRoundRect (10, 173, 90, 201);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (10, 173, 90, 201);
  myGLCD.setBackColor(223, 77, 55);
  myGLCD.setColor(255, 255, 255);
  myGLCD.print("mW", 24, 180);
   
}

void drawLux (){
  myGLCD.setColor(100, 155, 203);
  myGLCD.fillRoundRect (10, 10, 60, 36);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (10, 10, 60, 36);
  myGLCD.setFont(BigFont);
  myGLCD.setBackColor(100, 155, 203);
  myGLCD.print("<-", 18, 15);
   
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setFont(SmallFont);
  myGLCD.print("Back to Multimeter Menu", 70, 18);
  myGLCD.setFont(BigFont);
  myGLCD.print("Multimeter", CENTER, 45);
  myGLCD.print("Intensitas Cahaya", CENTER, 76);
  myGLCD.setColor(102, 255, 255);
  myGLCD.drawLine(0,100,319,100);
  myGLCD.setBackColor(0, 0, 0);
  
  myGLCD.setColor(255, 255, 255);
  myGLCD.setFont(SmallFont);
  myGLCD.print("Select Unit", 10, 114);
  myGLCD.setFont(BigFont);
  myGLCD.print("Hasil:", 120, 120);
  myGLCD.setColor(223, 77, 55);
  myGLCD.fillRoundRect (10, 135, 90, 163);
  myGLCD.setColor(225, 255, 255);
  myGLCD.drawRoundRect (10, 135, 90, 163);
  myGLCD.setBackColor(223, 77, 55);
  myGLCD.setColor(255, 255, 255);
  myGLCD.print("Lux", 20, 140);
  myGLCD.setColor(223, 77, 55);
  myGLCD.fillRoundRect (10, 173, 90, 201);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (10, 173, 90, 201);
  myGLCD.setBackColor(223, 77, 55);
  myGLCD.setColor(255, 255, 255);
  myGLCD.print("W/M^2", 12, 180);
   
}
void drawSuhu (){
  myGLCD.setColor(100, 155, 203);
  myGLCD.fillRoundRect (10, 10, 60, 36);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (10, 10, 60, 36);
  myGLCD.setFont(BigFont);
  myGLCD.setBackColor(100, 155, 203);
  myGLCD.print("<-", 18, 15);
   
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setFont(SmallFont);
  myGLCD.print("Back to Multimeter Menu", 70, 18);
  myGLCD.setFont(BigFont);
  myGLCD.print("Multimeter", CENTER, 45);
  myGLCD.print("Suhu", CENTER, 76);
  myGLCD.setColor(102, 255, 255);
  myGLCD.drawLine(0,100,319,100);
  myGLCD.setBackColor(0, 0, 0);
  
  myGLCD.setColor(255, 255, 255);
  myGLCD.setFont(SmallFont);
  myGLCD.print("Select Unit", 10, 114);
  myGLCD.setFont(SmallFont);
  myGLCD.print("Suhu Panel:", 115, 120);
  myGLCD.print("Suhu Lingkungan:", 120, 170);
  myGLCD.setColor(223, 77, 55);
  myGLCD.fillRoundRect (10, 135, 90, 163);
  myGLCD.setColor(225, 255, 255);
  myGLCD.drawRoundRect (10, 135, 90, 163);
  myGLCD.setBackColor(223, 77, 55);
  myGLCD.setColor(255, 255, 255);
  myGLCD.setFont(BigFont);
  myGLCD.print(" C", 22, 140);
  myGLCD.setColor(223, 77, 55);
  myGLCD.fillRoundRect (10, 173, 90, 201);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (10, 173, 90, 201);
  myGLCD.setBackColor(223, 77, 55);
  myGLCD.setColor(255, 255, 255);
  myGLCD.print(" F", 22, 180);
   
}

void drawAdcCal() {
  myGLCD.setColor(100, 155, 203);
  myGLCD.fillRoundRect (10, 10, 60, 36);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (10, 10, 60, 36);
  myGLCD.setFont(BigFont);
  myGLCD.setBackColor(100, 155, 203);
  myGLCD.print("<-", 18, 15);
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setFont(BigFont);
  myGLCD.print("Kalibrasi", CENTER, 50);
  myGLCD.print("ADC :", 50, 95);
  myGLCD.setColor(100, 155, 203);
  myGLCD.fillRoundRect (100, 180, 216, 215);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (100, 180, 216, 215);
  myGLCD.setBackColor(100, 155, 203);
  myGLCD.setFont(BigFont);
  myGLCD.print("SIMPAN", 110, 190);
  myGLCD.setColor(102, 255, 255);
  myGLCD.drawLine(0,80,319,80);
}

void getNilaiMulti(){ // Get Sens Value for Multimeter
  arusA = getCurrent(1, adc);
  Serial.println(adc);
  voltV = getVoltage(0);
  if(voltV < 0){
    voltV *= -1;
  }
  LUX = getLuxVal();
  dayaW = getWatt(voltV, arusA);
  getTemp();
  
  myGLCD.setFont(SmallFont);
  myGLCD.setColor(0, 255, 0);
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.printNumF(arusA ,2 ,135, 120, '.', 6);
  myGLCD.printNumF(voltV ,2 ,135, 145, '.', 6);
  myGLCD.printNumF(dayaW ,2 ,135, 170, '.', 6);
  myGLCD.printNumI(LUX ,135, 195, 6);
  myGLCD.printNumF(suhuLingkunganC ,2, 135, 220,'.', 6);
  myGLCD.print("/", 185, 220);
  myGLCD.printNumF(suhuLingkunganF ,2, 190, 220,'.', 7);
  myGLCD.setColor(255, 255, 255);
  myGLCD.print("A", 265, 120);
  myGLCD.print("V", 265, 145);
  myGLCD.print("W", 265, 170);
  myGLCD.print("Lux", 260, 195);
  myGLCD.print("C/F", 260, 220);
}

void getArus() {
  arusA = getCurrent(1, adc);
  Serial.println(adc);
  Serial.print(arusA);
  arusmA= arusA*1000.0;
  // Prints the value in A
  if (selectedUnit == '0') {
    myGLCD.setFont(SevenSegmentFull);
    myGLCD.setColor(0, 255, 0);
    myGLCD.setBackColor(0, 0, 0);
    myGLCD.printNumF(arusA ,2 ,110, 145);
    myGLCD.setFont(BigFont);
    myGLCD.print(" A", 285, 178);
    }
    // Prints the value in mA
  if (selectedUnit == '1') {
    myGLCD.setFont(SevenSegmentFull);
    myGLCD.setColor(0, 255, 0);
    myGLCD.setBackColor(0, 0, 0);
    myGLCD.printNumI(arusmA, 110, 145);
    myGLCD.setFont(BigFont);
    myGLCD.print("mA", 285, 178);
  } 
}

void getTegangan() {
  voltV = getVoltage(0);
  if(voltV < 0){
    voltV *= -1;
  }
  float voltmV= voltV*1000.0;
 
  // Prints the value in V
  if (selectedUnitV == '0') {
    myGLCD.setFont(SevenSegmentFull);
    myGLCD.setColor(0, 255, 0);
    myGLCD.setBackColor(0, 0, 0);
    myGLCD.printNumF(voltV, 2 ,110, 145);
    myGLCD.setFont(BigFont);
    myGLCD.print(" V", 285, 178);
    }
    // Prints the value in mV
  if (selectedUnitV == '1' ) {
    myGLCD.setFont(SevenSegmentFull);
    myGLCD.setColor(0, 255, 0);
    myGLCD.setBackColor(0, 0, 0);
    myGLCD.printNumF(voltmV , 1, 110, 145);
    myGLCD.setFont(BigFont);
    myGLCD.print("mV", 285, 178);
  } 
}

void getDaya() {
  dayaW = getWatt(voltV, arusA);
  float dayamW= dayaW*1000.0;
 
  // Prints the value in W
  if (selectedUnitW == '0') {
    myGLCD.setFont(SevenSegmentFull);
    myGLCD.setColor(0, 255, 0);
    myGLCD.setBackColor(0, 0, 0);
    myGLCD.printNumF(dayaW , 2, 110, 145);
    myGLCD.setFont(BigFont);
    myGLCD.print(" W", 285, 178);
    }
    // Prints the value in mW
  if (selectedUnitW == '1' ) {
    myGLCD.setFont(SevenSegmentFull);
    myGLCD.setColor(0, 255, 0);
    myGLCD.setBackColor(0, 0, 0);
    myGLCD.printNumF(dayamW ,1 ,110, 145);
    myGLCD.setFont(BigFont);
    myGLCD.print("mW", 285, 178);
  } 
}

void getLux() {
  LUX = getLuxVal();
  iradiasi = LUX*0.0079;
  
  // Prints the value in Lux
  if (selectedUnitL == '0') { 
    myGLCD.setFont(SixteenSegment16x24);
    myGLCD.setColor(0, 255, 0);
    myGLCD.setBackColor(0, 0, 0);
    myGLCD.printNumI(LUX ,120, 155);
    myGLCD.setFont(BigFont);
    myGLCD.print("lux", 255, 165);
  }
  // Prints the value in irradiance
  if (selectedUnitL == '1') {
    myGLCD.setFont(SixteenSegment16x24);
    myGLCD.setColor(0, 255, 0);
    myGLCD.setBackColor(0, 0, 0);
    myGLCD.printNumF(iradiasi ,2 ,120, 155);
    myGLCD.setFont(BigFont);
    myGLCD.print("W/M2", 255, 165);
  } 
}

void getSuhu() {
  getTemp();
  // Prints the temp in C
  if (selectedUnitL == '0') { 
    myGLCD.setFont(SixteenSegment16x24);
    myGLCD.setColor(0, 255, 0);
    myGLCD.setBackColor(0, 0, 0);
    myGLCD.printNumF(suhuPanelC, 2 , 120, 140, '.', 6);
    myGLCD.printNumF(suhuLingkunganC, 2, 120, 190, '.', 6);
    myGLCD.print("C", 255, 140);
    myGLCD.print("C", 255, 190);
  }
  // Prints the temp in F
  if (selectedUnitL == '1') {
    myGLCD.setFont(SixteenSegment16x24);
    myGLCD.setColor(0, 255, 0);
    myGLCD.setBackColor(0, 0, 0);
    myGLCD.printNumF(suhuPanelF, 2, 120, 140, '.', 6);
    myGLCD.printNumF(suhuLingkunganF, 2, 120, 190, '.', 6);
    myGLCD.print("F", 255, 140);
    myGLCD.print("F", 255, 190);
  } 
}

// Highlights the button when pressed
void drawFrame(int x1, int y1, int x2, int y2) {
  myGLCD.setColor(255, 0, 0);
  myGLCD.drawRoundRect (x1, y1, x2, y2);
  while (myTouch.dataAvailable())
    myTouch.read();
    myGLCD.setColor(255, 255, 255);
    myGLCD.drawRoundRect (x1, y1, x2, y2);
}

void backButton(String currentpage, void(*function)()){
  if ((x>=10) && (x<=60) &&(y>=10) && (y<=60)) {
    drawFrame(10, 10, 60, 36);
    otomatis = false;
    currentPage = currentpage;
    myGLCD.clrScr();
    (*function)();
  }
}
//==============================================================================================================================

//============================================ Mendapatkan nilai dari sensor =================================================== 
float getCurrent(int inputPin, int adc){
  current = 0.0;
  for(int i = 0; i < 10; i++){
    adc1 = ads.readADC_SingleEnded(inputPin) - adc; // read ADC from inputPin of ADS1115
    current += ads.computeVolts(adc1) * 100; // convert ADS1115 ADC to Voltage, 10 (ACS758 200B sensitivity) equals to 100 multiplier
    delay(1);
  }
  //  volts1 = ads.computeVolts(adc1); // convert ADS1115 A0 ADC to Voltage
  current /= 10;
  //  current2 = (volts1 - 2.5) * 100; // convert Voltage (volts variable) to Current
  if (current < 0){
    current = 0; // If current value < 0, make current = 0
  }else{
    // Do nothing
  }
  return current;
}

float getVoltage(int inputPin){
   volts0 = 0; // reset volts0 value
   // Make samples 10x, and take the average
   for (int i = 0; i < 10; i++){
      adc0 = ads.readADC_SingleEnded(inputPin); // read ADC from inputPin of ADS1115
      volts0 += ads.computeVolts(adc0); // convert ADS1115 ADC to Voltage
      delay(1);
   }
   volts0 /= 10; // Take average from above samples
   voc = ((R1 + R2)/R1) * volts0; // convert volts0 (input Voltage) to actual Voltage (VCC)
   if(voc < 0.05){
      voc = 0; // if voc < 0.05, make voc = 0
   }else{
      // do nothing
   }
  return voc;
}

int getLuxVal(){
  lux = lightMeter.readLightLevel();
  if (lux < 20.0){
    calLux = lux * 3.72;
  }else if(lux >= 20.0 && lux <= 50){
    calLux = lux * 3.53;
  }else if(lux > 50.0 && lux < 120){
    calLux = lux * 3.3;
  }else if(lux >= 120.0){
    calLux = lux * 3.12;
  }
  else{
    calLux = lux;
  }
  return calLux;
}

float getWatt(float volt, float arus){
  float watt = volt * arus;
  return watt;
}

void getValueAll(){
  getRelayVA();
  getTemp();
  LUX = getLuxVal();
  iradiasi = LUX * 0.0079;
  performa = getPerformance(voltV, arusA, atof(stLast1), atof(stLast2));
}

void getRelayVA() {
  startMillis = millis();
  if(fase == 0){
    interval = 2000;
  }else if (fase == 1){
    interval = 5000;
  }else if (fase ==2){
    interval = 2000;
  }else if (fase == 3){
    interval = 5000;
  }
  if(startMillis - millis1 > interval){ 
    if(fase == 0){
      digitalWrite(relay1, LOW);
      for(int i = 0; i < 10; i++){
        voltV = getVoltage(0);
        }
      fase = 1;
      statusKirim = false;
      statusKirimM = false;
    }
    else if(fase == 1){
      digitalWrite(relay1, HIGH);
      fase = 2;
      statusKirim = false;
      statusKirimM = false;
    }
    else if(fase == 2){
      digitalWrite(relay2, LOW);
      for(int i = 0; i < 10; i++){
        arusA = getCurrent(1, adc);
      }
      fase = 3;
      statusKirim = false;
      statusKirimM = false;
    }
    else if(fase = 3){
      digitalWrite(relay2, HIGH);
      statusKirim = true;
      statusKirimM = true;
      fase = 0;
      pbState = 1;
    }
    else{
      statusKirim = false;
      statusKirimM = false;
      pbState = 0;
      fase = 0;
    }
    millis1 = startMillis;
  }
}

float getPerformance(float voc, float isc, float vocNamePlate, float iscNamePlate){
  float pVoc = (voc/vocNamePlate) * 100;
  float pIsc = (isc/iscNamePlate) * 100;
  float performance = (pVoc + pIsc) / 2;
  return performance;
}

void adcCal(){
  calValue = ads.readADC_SingleEnded(1) - adc;
  if(calValue < 0){
    adc--;
  }else if(calValue > 0){
    adc++;
  }else{
    myGLCD.print("TERKALIBRASI", CENTER, 135);
  }
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setColor(255, 255, 255);
  myGLCD.setFont(BigFont);
  myGLCD.printNumI(calValue,150, 95, 6);
}

void getTemp(){
  DS18B20.requestTemperatures();
  suhuPanelC = suhuCel.read_temp();
  suhuPanelF = suhuFah.read_temp();
  suhuLingkunganC = DS18B20.getTempCByIndex(0);
  suhuLingkunganF = DS18B20.getTempFByIndex(0);
}

void sendESP(char mode){
  // mode = 'O' untuk Otomatis
  // mode = 'M' untuk Manual
  kirim = "";
  kirim += waktu;
  kirim += ";";
  kirim += voltV;
  kirim += ";";
  kirim += arusA;
  kirim += ";";
  kirim += LUX;
  kirim += ";";
  kirim += suhuPanelC;
  kirim += ";";
  kirim += suhuLingkunganC;
  kirim += ";";
  if(mode == 'O'){
    kirim += "110011";
    file = SD.open("OTOMATIS.txt", FILE_WRITE);
  }else if(mode == 'M'){
    kirim += "101101";
    file = SD.open("MANUAL.txt", FILE_WRITE);
  }
  Serial3.println(kirim);
  if(file){
    file.println(kirim);
    file.close();
    Serial.print("Berhasil > ");
    Serial.println(kirim);
  }
  statusKirim = false;
  statusKirimM = false;
}

void getTime(){
  DateTime now = rtc.now();
  hari = namaHari[now.dayOfTheWeek()];
  waktu = hari + ";" + now.day() + "/" + now.month() + "/" + now.year() + ";" + now.hour() + ":" + now.minute() + ":" + now.second();
}
// void kirimManual(){
//   kirim = "";
//   kirim += voltV;
//   kirim += ";";
//   kirim += arusA;
//   kirim += ";";
//   kirim += LUX;
//   kirim += ";";
//   kirim += suhuPanelC;
//   kirim += ";";
//   kirim += suhuLingkunganC;
//   kirim += ";";
//   Serial3.println(kirim);
// }