#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>

const char* host = "perftesid.com";

// int httpcode = 0;
float tegangan, suhu, arus, iradiasi, suhuPanel, suhuLingkungan, performa;
String token = "03ar2dffd";
String waktu = "";
int mode = 0;
String address = "";
int signal = 0;
int kirim = 0;
int httpCode = 0;

String postData;
String  msg = "";

WiFiClient client;
HTTPClient http;

void setup() {
  Serial.begin(115200);
  WiFiManager wifiManager;
  wifiManager.autoConnect("PV Performance Tester");
  Serial.println("connected :");
}

String splitString(String data, char separator, size_t index) {
  size_t strLength = data.length();
  size_t startIndex = 0;
  size_t endIndex = 0;
  size_t count = 0;

  if (index < 0) {
    // Handle negative index, e.g., to access elements from the end
    index = strLength + index;
    if (index < 0) {
      // Handle the case where the adjusted index is still negative
      return "";
    }
  }

  for (size_t i = 0; i < strLength; i++) {
    if (data.charAt(i) == separator) {
      count++;
      if (count == index) {
        startIndex = i + 1;
      } else if (count == index + 1) {
        endIndex = i;
      }
    }
  }

  if (startIndex < endIndex) {
    return data.substring(startIndex, endIndex);
  } else if (startIndex < strLength) {
    return data.substring(startIndex);
  } else {
    return "";
  }
}


void kirimData(){
  if(mode == 110011 || mode == 101101 || mode == 110001 || mode == 101001){
    kirim = 0;
    http.begin(client, address);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    httpCode = http.POST(postData);
    String payload;
    // payload.reserve(MAX_PAYLOAD_LENGTH);
    if(httpCode > 0){
      payload = http.getString();
      payload.trim();
      if(payload.length() > 0){
        Serial.println(payload+ "\n");
        msg = "";
        int i = 0;
        Serial.println(404);
        kirim = 1;
        mode = 0;
        // delay(4500);
        http.end();
      }
    }
  }
}

void loop() { 
  mode = 0;
  msg = "";
  // Serial.print("I received: ");
  // read the incoming byte:
  const unsigned long SERIAL_TIMEOUT = 100; // Timeout in milliseconds
  unsigned long serialStartTime = millis();

  while (Serial.available() && millis() - serialStartTime < SERIAL_TIMEOUT) {
    msg += char(Serial.read());
  }

// Process the message if it is complete
  if (msg.endsWith("\n")) {
    msg.trim();
    waktu = splitString(msg, ';', 1);
    tegangan = splitString(msg, ';', 2).toFloat();
    arus = splitString(msg, ';', 3).toFloat();
    iradiasi = splitString(msg, ';', 4).toFloat();
    suhuPanel = splitString(msg, ';', 5).toFloat();
    suhuLingkungan = splitString(msg, ';', 6).toFloat();
    performa = splitString(msg, ';', 7).toFloat();
    mode = splitString(msg, ';', 8).toInt();
  }
  msg = "";
  if(signal == 808 && kirim > 0){
    // delay(1000);
    Serial.println(404);
  }
  // Serial.println(msg);
  msg = "";
  // Serial.println(mode);
  // WiFiClient client;
  // HTTPClient http;
  // http.begin(client, address);
  // if (!client.connect("192.168.1.12", 80)) {
  //   Serial.println("connection failed"); 
  //   return;
  // }else{
  //   // Serial.print("connected to database");
  // }
  // http.end();

  if(mode == 110011){
    address = "http://" + String(host) + "/api/service.device.php";
  }else if (mode == 101101){
    address = "http://" + String(host) + "/api/service.deviceMan.php";
  }else if (mode == 110001){
    address = "http://" + String(host) + "/api/service.deviceOff.php";
  }else if (mode == 101001){
    address = "http://" + String(host) + "/api/service.deviceManOff.php";
  }else{
    address = "";
  }
  postData = "";
  postData += "token=";
  postData += token;
  postData += "&tegangan=";
  postData += tegangan;
  postData += "&arus=";
  postData += arus;
  postData += "&suhuPanel=";
  postData += suhuPanel;
  postData += "&suhuLingkungan=";
  postData += suhuLingkungan;
  postData += "&iradiasi=";
  postData += iradiasi;
  postData += "&performa=";
  postData += performa;
  postData += "&waktu=";
  postData += waktu;
  // Serial.print("HTTP CODE : ");
  // Serial.println(httpcode);
  // Serial.print("token : ");
  // Serial.println(token);
  // Serial.print("tegangan : ");
  // Serial.println(tegangan);
  // Serial.print("arus : ");
  // Serial.println(arus);
  // Serial.print("suhuPanel : ");
  // Serial.println(suhuPanel);
  // Serial.print("suhuLingkungan : ");
  // Serial.println(suhuLingkungan);
  // Serial.print("iradiasi : ");
  // Serial.println(iradiasi);
  // Serial.print("performa : ");
  // Serial.println(performa);
  // Serial.print("waktu : ");
  // Serial.println(waktu);
  // Serial.print("mode : ");
  // Serial.println(mode);
  kirimData();
  if(httpCode < 0){
    kirim = 0;
    kirimData();
  }else{
    kirim = 1;
  }
  delay(1000);
}

