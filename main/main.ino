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

void backButton(int, void (*)(), bool = true);
void touchButton(int, int, int, int, int, void (*)(), int = 0, int = 1, int = 1, bool = false, bool = false, bool = false, bool = false, bool = true);
void selectUnit(int, int, int, int, char, bool istemp = false);
void drawHomeScreen();
void drawMultimeter();
void drawPerformance();
void drawManual();
void drawOtomatis();
void drawOffline();
void drawSetting();
void SelectOption();
void drawAdcCal();
void drawArus();
void drawTegangan();
void drawDaya();
void drawLux();
void drawSuhu();
void StopPengukuran();

//=======================ESP Data Transfer=========================
String kirim = "";
bool otomatis, statusKirim = false, statusKirimM = false;

//Upload Data
int signal = 404;
String msg = "";
String line = "";
int i = 0;
bool uploadState = false;
bool offlineState = false;
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
String vocData = "";
String iscData = "";
char stLast1[20] = "";
char stLast2[20] = "";

// Menu
String currentPage;
char selectedUnit = '0',selectedUnitV = '0', selectedUnitW = '0', selectedUnitL = '0'; 
bool fromMenu = true;

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
float R1 = 975.0; //Resistor 1, 
float R2 = 203000.0; //Resistor 2
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
OneWire pin_DS18B20(16);
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
unsigned long LUX;
BH1750 lightMeter;
//==========================

//======================================Data Variables===================================
float arusA, arusmA, voltV, voltmV, iradiasi, dayaW, dayamW, performa;
float suhuPanelC, suhuPanelF, suhuLingkunganC, suhuLingkunganF;
unsigned long lux, calLux;
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
  file = SD.open("SETTING.txt", FILE_READ);
  if (file) {
    String line = file.readStringUntil('\n');
    file.close();
    vocData = splitString(line, ';', 0);
    iscData = splitString(line, ';', 1);
    adc = splitString(line, ';', 2).toInt();
  } else {
    Serial.println("Gagal membuka file");
  }
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
  Serial.print("offlineState = ");
  Serial.println(offlineState);
  getTime(); // real time clock for data logger
  calValue = ads.readADC_SingleEnded(1) - adc; // to get calValue value for calibration icon warning
  // Home Screen
  if (currentPage == "0") {
    if(calValue == 0 || (calValue <= 265 && calValue >= (-2))){
      myGLCD.setColor(12, 171, 182);
      myGLCD.fillRect(240, 200, 275, 220);
    }else{
      warnCal(250, 202);
    }
    if (myTouch.dataAvailable()) {
      myTouch.read();
      x=myTouch.getX(); // X coordinate where the screen has been pressed
      y=myTouch.getY(); // Y coordinates where the screen has been pressed

      // If we press the Performance Button 
      touchButton(35, 285, 90, 130, 10, drawPerformance);
      // If we press the Multimeter Button 
      touchButton(35, 285, 140, 180, 11, drawMultimeter);
      // If we press Setting
      touchButton(35, 285, 190, 230, 3, drawSetting);
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

      //Back button
      if(offlineState == true){
        backButton(18, drawPerformance, false);
      }else{
      backButton(10, drawPerformance);
      }
    }
  }
  
  // Otomatis
  if (currentPage == "2") {
    SliderOtomatis();
    if (myTouch.dataAvailable()) {
        myTouch.read();
        x=myTouch.getX();
        y=myTouch.getY();

      // Mulai
      
      //Back button
      if(offlineState == true){
        touchButton(100, 216, 180, 215, 4, StopPengukuran, 0, 1, 1, false, false, false, true, false); 
        backButton(18, drawPerformance, false);
        offlineState == true;
      }else{
        backButton(10, drawPerformance);
        touchButton(100, 216, 180, 215, 4, StopPengukuran);
      }
    }
  }
  // Setting
  if (currentPage =="3") {
    uploadState = false;
    if(calValue == 0 || (calValue <= 265 && calValue >= (-2))){
      myGLCD.setColor(12, 171, 182);
      myGLCD.fillRect(240, 145, 275, 165);
    }else{
      warnCal(250, 150);
    }
    if (myTouch.dataAvailable()) {
      myTouch.read();
      x=myTouch.getX();
      y=myTouch.getY();
      backButton(0, drawHomeScreen);
      // input data panel
      touchButton(30, 290, 90, 130, 7, SelectOption);
      //Kalibrasi
      touchButton(30, 290, 140, 180, 5, drawAdcCal);
      //Upload data
      touchButton(30, 290, 190, 230, 6, drawUploadData, 0, 1, 1, false, true);
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
      backButton(3, drawSetting);
      // if we press simpan
      touchButton(100, 216, 180, 215, 3, drawSetting, 0, 1, 1, true);
    }
  }

  //Menu Upload Data
  if (currentPage == "6") {
    if (uploadState == true){
      uploadData();
    }
    if (myTouch.dataAvailable()) {
      myTouch.read();
      x=myTouch.getX();
      y=myTouch.getY();
      //Stop Button
      touchButton(35, 285, 140, 180, 3, drawSetting, 0, 1, 1, false, false);
    }
  }

  //Menu Selesai Upload Data
  if (currentPage == "17") {
    if (myTouch.dataAvailable()) {
      myTouch.read();
      x=myTouch.getX();
      y=myTouch.getY();
      //Back button
      backButton(3, drawSetting);
      //Stop Button
      touchButton(35, 285, 140, 180, 3, drawSetting, 0, 1, 1, false, false, true);
    }
  }

  if (currentPage == "7") {
    if (myTouch.dataAvailable()) {
      myTouch.read();
      x=myTouch.getX();
      y=myTouch.getY();
      touchButton(250, 290, 120, 140, 8, drawButtons);
      touchButton(250, 290, 120, 140, 9, drawButtons);
      backButton(3, drawSetting);
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
            vocData = String(stLast1);
            saveSetting();
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
      touchButton(250, 300, 10, 60, 7, SelectOption);
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
            iscData = String(stLast2);
            saveSetting();
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
      touchButton(250, 300, 10, 60, 7, SelectOption);
    }
  }

  // Performance
  if (currentPage == "10") {    
    if (myTouch.dataAvailable()) {
      myTouch.read();
      x=myTouch.getX();
      y=myTouch.getY();
      // If we press the Manual Button 
      touchButton(35, 285, 80, 120, 1, drawManual, 0, 1, 1, false, false, false, false);
      // If we press the Otomatis Button 
      touchButton(35, 285, 130, 170, 2, drawOtomatis, 0, 1, 1, false, false, false, false);
      // If we press the Offline Mode button 
      touchButton(35, 285, 180, 220, 18, drawPerformance, 0, 1, 1, false, false, false, false, false);
      // Back Button
      backButton(0, drawHomeScreen);
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
      touchButton(5, 57, 70, 100, 12, drawArus, 0, 1, 0);
        // If we press Tegangan
      touchButton(62, 134, 70, 100, 13, drawTegangan, 0, 0, 1);
      // If we press Daya
      touchButton(139, 212, 70, 100, 14, drawDaya, 0, 1, 0);
      // If we press Lux
      touchButton(217, 270, 70, 100, 15, drawLux, 0, 1, 0);
      // If we press Suhu
      touchButton(275, 315, 70, 100, 16, drawSuhu, 0, 1, 0);
      //Back Button
      backButton(0, drawHomeScreen);
    }
  }
  //Multiimeter Arus
  if (currentPage == "12") {    
    digitalWrite(relay1,HIGH);
    digitalWrite(relay2,LOW);
    getArus(); 
    if (myTouch.dataAvailable()) {
      myTouch.read();
      x=myTouch.getX();
      y=myTouch.getY();
      
      // If we press A
      selectUnit(10, 90, 135, 163, '0');
      // If we press mA
      selectUnit(10, 90, 173, 201, '1');
      backButton(11, drawMultimeter);
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
      selectUnit(10, 90, 135, 163, '0');
      // If we press mV
      selectUnit(10, 90, 173, 201, '1');
      // If we press the Back Button
      backButton(11, drawMultimeter);
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
      selectUnit(10, 90, 135, 163, '0');
      // If we press mW
      selectUnit(10, 90, 173, 201, '1');
      // If we press the Back Button
      backButton(11, drawMultimeter);
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
      selectUnit(10, 90, 135, 163, '0');
      // If we press w/m2
      selectUnit(10, 90, 173, 201, '1');
      // If we press the Back Button
      backButton(11, drawMultimeter);
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
      selectUnit(10, 90, 135, 163, '0', true);
      // If we press F
      selectUnit(10, 90, 173, 201, '1', true);
      // If we press the Back Button
      backButton(11, drawMultimeter);
    }
  }
  if (currentPage == "18") {    
    if (myTouch.dataAvailable()) {
      myTouch.read();
      x=myTouch.getX();
      y=myTouch.getY();
      // If we press the Manual Button 
      touchButton(35, 285, 80, 120, 1, drawManual, 0, 1, 1, false, false, false, true);
      // If we press the Otomatis Button 
      touchButton(35, 285, 130, 170, 2, drawOtomatis, 0, 1, 1, false, false, false, true);
      // Back Button
      backButton(10, drawPerformance);
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
  
  // Button - Manual 
  myGLCD.setColor(12, 171, 182);
  myGLCD.fillRoundRect (35, 80, 285, 120);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (35, 80, 285, 120);
  myGLCD.setFont(BigFont);
  myGLCD.setBackColor(12, 171, 182);
  myGLCD.print("MANUAL", CENTER, 92);
  
  // Button - Otomatis
  myGLCD.setColor(12, 171, 182);
  myGLCD.fillRoundRect (35, 130, 285, 170);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (35, 130, 285, 170);
  myGLCD.print("OTOMATIS", CENTER, 142);
  
  // Button - Offline Mode
  if(fromMenu == true){
    myGLCD.setColor(12, 171, 182);
    myGLCD.fillRoundRect (35, 180, 285, 220);
    myGLCD.setColor(255, 255, 255);
    myGLCD.drawRoundRect (35, 180, 285, 220);
    myGLCD.print("MODE OFFLINE", CENTER, 192);
    myGLCD.setBackColor(0, 0, 0);
    myGLCD.setColor(255, 255, 255);
    myGLCD.setFont(SmallFont);
    myGLCD.print("Back to Main Menu", 70, 18);
  }else{
    myGLCD.setBackColor(0, 0, 0);
    myGLCD.setColor(255, 255, 255);
    myGLCD.setFont(SmallFont);
    myGLCD.print("Back to Performance Menu", 70, 18);
  }
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
  myGLCD.print("Kalibrasi", CENTER, 150);
  myGLCD.setColor(12, 171, 182);
  myGLCD.fillRoundRect (30, 190, 290, 230);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (30, 190, 290, 230);
  myGLCD.print("Upload Data", CENTER, 200);
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

void drawUploadData(){
  myGLCD.setFont(BigFont); 
  myGLCD.setColor(255, 255, 255); 
  myGLCD.setBackColor(0, 0, 0); 
  myGLCD.print("MENGUPLOAD DATA", CENTER, 105);
  myGLCD.setColor(12, 171, 182); 
  myGLCD.fillRoundRect (35, 130, 285, 170); 
  myGLCD.setColor(255, 255, 255); 
  myGLCD.drawRoundRect (35, 130, 285, 170); 
  myGLCD.setBackColor(12, 171, 182); 
  myGLCD.print("STOP UPLOAD", CENTER, 140); 
}

void drawWaitUpload(){
  myGLCD.setFont(BigFont); 
  myGLCD.setColor(255, 255, 255); 
  myGLCD.setBackColor(0, 0, 0); 
  myGLCD.print("MEMBATALKAN ...", CENTER, 110);
}

void drawDoneUpload(){
  myGLCD.setColor(100, 155, 203);
  myGLCD.fillRoundRect (10, 10, 60, 36);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (10, 10, 60, 36);
  myGLCD.setFont(BigFont);
  myGLCD.setBackColor(100, 155, 203);
  myGLCD.print("<-", 18, 15);
  myGLCD.setFont(BigFont); 
  myGLCD.setColor(255, 255, 255); 
  myGLCD.setBackColor(0, 0, 0); 
  myGLCD.print("DATA TELAH", CENTER, 90);
  myGLCD.print("TERUPLOAD", CENTER, 115);
  myGLCD.setFont(SmallFont); 
  myGLCD.setColor(12, 171, 182); 
  myGLCD.fillRoundRect (35, 140, 285, 170); 
  myGLCD.setColor(255, 255, 255); 
  myGLCD.drawRoundRect (35, 140, 285, 170); 
  myGLCD.setBackColor(12, 171, 182); 
  myGLCD.print("SELESAI (HAPUS DATA)", CENTER, 150); 
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
  myGLCD.print(vocData, 130, 120);
  myGLCD.print("V", 220, 120);
  myGLCD.print("ARUS", 10, 160);
  myGLCD.print("Isc   :", 10,190);
  myGLCD.print(iscData, 130, 190);
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
  myGLCD.print("Arus (I)      :", 10, 125);
  myGLCD.print("Tegangan (V)  :", 10, 155);
  myGLCD.print("Daya (P)      :", 10, 185);
  myGLCD.print("I.Cahaya (Lux):", 10, 215);
  myGLCD.print("A", 295, 125);
  myGLCD.print("V", 295, 155);
  myGLCD.print("W", 295, 185);
  myGLCD.print("Lux", 290, 215);
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
   myGLCD.setBackColor(12, 171, 182);
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
  voltV = getVoltage(0);
  if(voltV < 0){
    voltV *= -1;
  }
  LUX = getLuxVal();
  dayaW = getWatt(voltV, arusA);
  
  myGLCD.setFont(BigFont);
  myGLCD.setColor(0, 255, 0);
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.printNumF(arusA ,2 ,155, 125, '.', 6);
  myGLCD.printNumF(voltV ,2 ,155, 155, '.', 6);
  myGLCD.printNumF(dayaW ,2 ,155, 185, '.', 6);
  myGLCD.printNumI(LUX ,155, 215, 6);
}

void getArus() {
  arusA = getCurrent(1, adc);
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
    myGLCD.printNumF(voltV, 2 ,110, 145, '.', 5);
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
  Serial.print(lux);
  Serial.print(" | ");
  Serial.println(LUX);
  LUX = getLuxVal();
  iradiasi = LUX*0.0079;
  
  // Prints the value in Lux
  if (selectedUnitL == '0') { 
    myGLCD.setFont(SixteenSegment16x24);
    myGLCD.setColor(0, 255, 0);
    myGLCD.setBackColor(0, 0, 0);
    myGLCD.printNumI(LUX, 125, 155, 7);
    myGLCD.printNumI(lux, 125, 190, 7);
    myGLCD.setFont(BigFont);
    myGLCD.print("lux", 255, 165);
  }
  // Prints the value in irradiance
  if (selectedUnitL == '1') {
    myGLCD.setFont(SixteenSegment16x24);
    myGLCD.setColor(0, 255, 0);
    myGLCD.setBackColor(0, 0, 0);
    myGLCD.printNumF(iradiasi ,2 ,120, 155, '.', 4);
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

// Tombol Back
void backButton(int page, void(*function)(), bool frommenu = true){
  fromMenu = frommenu;
  if ((x>=10) && (x<=60) &&(y>=10) && (y<=60)) {
    drawFrame(10, 10, 60, 36);
    uploadState = false;
    otomatis = false;
    currentPage = String(page);
    myGLCD.clrScr();
    (*function)();
  }
}

void touchButton(int x1, int x2, int y1, int y2, int page, void(*function)(), int fase = 0, int relay1State = 1, int relay2State = 1, bool savesetting = false, bool upload = false, bool delData = false, bool offlinestate = false, bool frommenu = true){
  fromMenu = frommenu;
  offlineState = offlinestate;
  uploadState = upload;
  if ((x>=x1) && (x<=x2) &&(y>=y1) && (y<=y2)) {
    drawFrame(x1, y1, x2, y2);
    currentPage = String(page);
    digitalWrite(relay1, relay1State);
    digitalWrite(relay2, relay2State);
    if(savesetting == true){
      saveSetting();
    }else{}
    if(delData == true){
      file = SD.open("OFFLINE.txt", FILE_WRITE | O_TRUNC); // Menghapus isi file txt
      file.close(); // Menutup file txt
    }else{}
    myGLCD.clrScr();
    (*function)();
  }
}

void selectUnit(int x1, int x2, int y1, int y2, char selectedunit, bool istemp = false){
  if ((x>=x1) && (x<=x2) &&(y>=y1) && (y<=y2)) {
    drawFrame(x1, y1, x2, y2);
    selectedUnit = selectedunit;
    selectedUnitW = selectedunit;
    selectedUnitV = selectedunit;
    selectedUnitL = selectedunit;
    myGLCD.setColor(0, 0, 0);
    if (istemp == true){
      myGLCD.fillRect(118, 135, 315, 162);
      myGLCD.fillRect(118, 183, 315, 220);
    }else{
      myGLCD.fillRect(110, 140, 310, 210);
    }
  }
}

// Icon peringatan kalibrasi
void warnCal(int xPlace, int yPlace){
  myGLCD.setBackColor(VGA_TRANSPARENT);
  myGLCD.setColor(VGA_YELLOW);
  myGLCD.setFont(BigFont);
  myGLCD.print("!!", xPlace, yPlace);
  myGLCD.setColor(VGA_TRANSPARENT);
  myGLCD.print("  ", xPlace, yPlace);
  myGLCD.print("  ", xPlace, yPlace);
  myGLCD.print("  ", xPlace, yPlace);
  myGLCD.print("  ", xPlace, yPlace);
  myGLCD.setColor(255, 0, 0);
  myGLCD.print("!!", xPlace, yPlace);
}

//==============================================================================================================================

//============================================ Mendapatkan nilai dari sensor =================================================== 
float getCurrent(int inputPin, int adc){
  current = 0.0;
  // for(int i = 0; i < 10; i++){
  adc1 = ads.readADC_SingleEnded(inputPin) - adc; // read ADC from inputPin of ADS1115
  //   current += ads.computeVolts(adc1) * 100; // convert ADS1115 ADC to Voltage, 10 (ACS758 200B sensitivity) equals to 100 multiplier
  //   delay(1);
  // }
  current = ads.computeVolts(adc1) * 100; // convert ADS1115 A0 ADC to Voltage
  // current /= 10;
  // current = (volts1 - 2.5) * 100; // convert Voltage (volts variable) to Current
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
  voc -= 1.06;
  return voc;
}

unsigned long getLuxVal(){
  lux = lightMeter.readLightLevel();
  if (lux < 20.0){
    calLux = lux * 3.2;
  }else if(lux >= 20.0 && lux < 50.0){
    calLux = lux * 3.53;
  }else if(lux >= 50.0 && lux < 120.0){
    calLux = lux * 3.1428;
  }else if(lux >= 120.0 && lux < 1000.0){
    calLux = lux * 3.12;
  }else if(lux >= 1000.0 && lux < 2000.0){
    calLux = lux * 3.02;
  }else if(lux >= 2000.0 && lux < 3000.0){
    calLux = lux * 3.18;
  }else if(lux >= 3000.0 && lux < 4000.0){
    calLux = lux * 3.31;
  }else if(lux >= 4000.0 && lux < 5000.0){
    calLux = lux * 3.47;
  }else if(lux >= 5000.0 && lux < 10000.0){
    calLux = lux * 3.54;
  }else if(lux >= 10000.0 && lux < 20000.0){
    calLux = lux * 3.77;
  }else if(lux >= 20000.0 && lux < 30000.0){
    calLux = lux * 4.19;
  }else if(lux >= 30000.0 && lux < 32000.0){
    calLux = lux * 4.4;
  }else if(lux >= 32000.0 && lux < 34000.0){
    calLux = lux * 4.54;
  }else if(lux >= 34000.0){
    calLux = lux * 4.62;
  }else{
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
  performa = getPerformance(voltV, arusA, vocData.toFloat(), iscData.toFloat());
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
  if(calValue >= (-100) && calValue < 0){
    adc--;
  }else if(calValue >= (-300) && calValue < (-100)){
    adc-= 100;
  }else if(calValue >= (-1000) && calValue < (-300)){
    adc-= 300;
  }else if(calValue < (-1000)){
    adc-= 900;
  }else if(calValue > 0 && calValue <= 100){
    adc++;
  }else if(calValue > 100 && calValue <= 800){
    adc+= 200;
  }else if(calValue > 800 && calValue <= 1500){
    adc+= 500;
  }else if(calValue > 1500){
    adc+= 1000;
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


void getTime(){
  DateTime now = rtc.now();
  hari = namaHari[now.dayOfTheWeek()];
  waktu = hari + ";" + now.day() + "/" + now.month() + "/" + now.year() + ";" + now.hour() + ":" + now.minute() + ":" + now.second();
}

String splitString(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void saveSetting(){
  file = SD.open("SETTING.txt", FILE_WRITE | O_TRUNC);
    if(file){
      String simpanData = vocData + ";" + iscData + ";" + adc;
      file.print(simpanData);
      file.close();
      // Serial.print("Berhasil > ");
      // Serial.println(simpanData);
    }else{
      // Serial.print("Gagal membuka file")
    }
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
  if(offlineState == true){
    file = SD.open("OFFLINE.txt", FILE_WRITE);
    if(file){
      file.println(kirim);
      Serial.println("Berhasil menulis data Offline");
      file.close();
    }
  }else{
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
      Serial.println("Berhasil menulis data Online");
    }
  }
  statusKirim = false;
  statusKirimM = false;
}

void uploadData(){
  file = SD.open("OFFLINE.txt"); // Membuka file txt
  if (file) {
    // int i = 1;
    // Serial.println(signal);
    while (file.available() && uploadState == true) {
      Serial.println(signal);
      if (myTouch.dataAvailable()) {
        myTouch.read();
        x=myTouch.getX(); // X coordinate where the screen has been pressed
        y=myTouch.getY();
        //Stop Button
        touchButton(35, 285, 130, 170, 6, drawWaitUpload, 0, 1, 1, false, false);
        }
      line = file.readStringUntil('\n'); // Membaca baris hingga akhir barisnya
      kirim = line;
      // delay(1000);
      Serial3.println(kirim);
      delay(1000);
      Serial.println(kirim); // Menampilkan baris ke Serial Monitor
      i = 0;
      // Serial.println(signal);
      signal = 0;
      if(signal != 404){
        if (myTouch.dataAvailable()) {
          myTouch.read();
          x=myTouch.getX(); // X coordinate where the screen has been pressed
          y=myTouch.getY();
          //Stop Button
          touchButton(35, 285, 130, 170, 6, drawWaitUpload, 0, 1, 1, false, false);
        }
        delay(1000);
        i += 1;
        Serial.print("loop ke : ");
        Serial.println(i);
        if(Serial3.available()){
          msg = "";
          signal = 0;
          while(Serial3.available()){
            msg += char(Serial3.read());
            delay(50);
          }
          Serial.print("Pesan masuk : ");
          Serial.println(msg);
          Serial3.println(808);
          signal = splitString(msg, ';', 0).toInt();
          Serial.print("Signal : ");
          Serial.println(signal);
          if(signal < 0){
            Serial3.println(kirim);
          }
        }
      }
    }
    file.close(); // Menutup file txt
  // }
  }
  uploadState = false;
  myGLCD.clrScr();
  currentPage = "17";
  drawDoneUpload();
}