#include <SPI.h>
#include "command_st95.h"
#include "conver_hex.h"
#include <WiFiUdp.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "time.h"
#include <FirebaseESP32.h>
#include <Keypad.h>
#include "images.h"
#include "oled_0_96.h"
#include "RFID125.h"
#include "ChuongThongBao.h"
#include "LibrarySIM800.h"

#define SS_PIN 22
#define RST_PIN 21
#define FIREBASE_HOST "cloud-nfc-proj.firebaseio.com"
#define FIREBASE_AUTH "XYcRpajciWgqrcQNUWovKfSOEUTUFv5hgkyGEvnI"
#ifdef __cplusplus
extern "C" {
#endif
#ifdef __cplusplus
}
#endif
String currentDay = "";
String currentTime = "";
const byte ROWS = 4; 
const byte COLS = 3; 
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
byte rowPins[ROWS] = {33, 25, 26, 27}; 
byte colPins[COLS] = {32, 12, 14}; 

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

FirebaseData firebaseData;

FirebaseJson json;
uint8_t So_Tien_Pay[16];
uint8_t viTriTien = 0;

String serverName = "http://nfcapi.conek.net/conek/dulieudiemdanh?uidTag=";
unsigned long lastTime = 0;
unsigned long timerDelay = 5000;

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 3600 * 7;

uint8_t temprature_sens_read();
uint8_t TagID[8];
uint8_t UID[16];
int Type[2];
String textUID = " ";
WiFiUDP ntpUDP;
const char* ssid = "NhaChung661";
const char* password = "661phamtuantai";

long rssi = 0;
int bars = 0;

void Init_Module_Reader() {
  SPI.begin();
  SPI.setDataMode (SPI_MODE0);
  SPI.setBitOrder (MSBFIRST);
  SPI.setFrequency (4000000);
  pinMode (SS_Pin, OUTPUT);
  digitalWrite (SS_Pin, HIGH);
  WakeUp_TinZ();
}
static void UDP_send_data(String DataNeedSend, int gatePort)
{
  /*Gui goi tin di*/
  ntpUDP.beginPacket("255.255.255.255", gatePort);
  //ntpUDP.write(DataNeedSend.c_str(), DataNeedSend.length());
  ntpUDP.print(DataNeedSend);
  ntpUDP.endPacket();
}
/*===================Function de doc ma UID cua Tag NFC========================== */
String ReadTagNFC() {
  uint8_t count;
  String IdTag = "";
  for(int i = 0; i < 8; i++){
    TagID[i] = 0;
  }
  if (CR95HF_ping()) {
    if(Protocol_Selection_type2()){
      if(Type2_Request_A(TagID,Type) == true){
        if(TagID[4] == 0 && TagID[5] == 0 && TagID[6] == 0 && TagID[7] == 0){
          TagID[7] = TagID[3];
          TagID[6] = TagID[2];
          TagID[5] = TagID[1];
          TagID[4] = TagID[0];
          TagID[0] = 0;
          TagID[1] = 0;
          TagID[2] = 0;
          TagID[3] = 0;
          if (encode8byte_big_edian (TagID, UID) == 0) {
            for (count = 5; count < 8 * 2; count++) {
              UID[count] += 0x30;
              IdTag += char(UID[count]);
            }            
          }
        }else{
          TagID[7] = TagID[6];
          TagID[6] = TagID[5];
          TagID[5] = TagID[4];
          TagID[4] = TagID[3];
          TagID[3] = TagID[2];
          TagID[2] = TagID[1];
          TagID[1] = TagID[0];
          TagID[0] = 0;
          if (encode8byte_big_edian (TagID, UID) == 0) {
            for (count = 0; count < 8 * 2; count++) {
              UID[count] += 0x30;
              IdTag += char(UID[count]);
            }            
          }           
        }
        return IdTag;
      }
    }
    if (setprotocol_tagtype5()) {
      if (getID_Tag(TagID) == true ){
        TagID[0] = 0x00;
        if (encode8byte_big_edian (TagID, UID) == 0) {
          for (count = 0; count < 8 * 2; count++) {
            UID[count] += 0x30;
            IdTag += char(UID[count]);
          }
        }
        return IdTag;
      }
    }    
  }
}
int function = 0;
void setup() {

  Init_Module_Reader();
  
  Serial.begin(9600);
  Init_Oled();
  Oled_print(33,20,"Welcome");
  delay(1000);
  drawImageDemo(34,14,60,36,WiFi_Logo_bits);
  delay(500);
  WiFi.begin();
  int counter = 1;
  int goalValue = 0;
  int currentLoad = 0;  
  Serial.println("Connecting");
SettingWifi:
  while (WiFi.status() != WL_CONNECTED) {
    goalValue += 8;
    if(goalValue < 85){
      drawProgressBarDemo(currentLoad,counter, goalValue);
    }else{
      goto SettingWifi;
      //GetWifi();
      break;
    }    
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println(WiFi.localIP());
  Serial.println("Timer set to 5 seconds (timerDelay variable), it will take 5 seconds before publishing the first reading.");
  ntpUDP.begin(6688);
  pinMode (PIN_BUZZER, OUTPUT);
  digitalWrite (PIN_BUZZER, HIGH);

  pinMode (FUNCTION_READ125, INPUT_PULLDOWN);
  pinMode (FUNCTION_SIM, INPUT_PULLDOWN);
  rssi = WiFi.RSSI();
  bars = getBarsSignal(rssi);
  Main_Screen(Barca_Logo_bits,bars,"06-06-1996","12:00:00");
  //Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  //Firebase.reconnectWiFi(true);
}

void loop() {
  if(digitalRead(FUNCTION_READ125) == HIGH){
    function = 1;
  }else if(digitalRead(FUNCTION_SIM) == HIGH){
    function = 2;
  }
  switch(function){
    case 0:
      printLocalTime();
      rssi = WiFi.RSSI();
      bars = getBarsSignal(rssi);  
      Main_Screen(Barca_Logo_bits,bars,currentDay,currentTime);    
      textUID = ReadTagNFC();
      if (textUID != "") {
        Oled_print(65,20,"Sending");
        Serial.println(textUID);
        Serial.println("Input Your Money: ");
        function = 3;        
        Bip(1);        
      }
      break;
    case 1:
      printLocalTime();
      rssi = WiFi.RSSI();
      bars = getBarsSignal(rssi);  
      Main_Screen(Barca_Logo_bits,bars,currentDay,currentTime);    
      textUID = getTag125();
      if (textUID != "") {
        Serial.println(textUID);
        textUID = "";
      }
      break;
    case 3:
      Oled_print(62,20,"Success");
      char key = keypad.getKey();
      if (key != NO_KEY){
        Serial.print(key);
        if(key == '#'){
          Serial.println();
          Serial.println("Sending...");
          function = 0;
          Bip(2);              
        }else if(key == '*'){
          Serial.println("Back");
        }
      }         
      break;
  }
}

void printLocalTime() {
  struct tm timeinfo;
  int memSecond = 0;
  String secT = "";
  String minT = "";
  String houT = "";
  String dayT = "";
  String monT = "";
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  if(memSecond != timeinfo.tm_sec){
    memSecond = timeinfo.tm_sec;
    if(timeinfo.tm_hour < 10){
      houT = "0" + String(timeinfo.tm_hour);
    }else{
      houT =String(timeinfo.tm_hour);
    }
    if(timeinfo.tm_min < 10){
      minT = "0" + String(timeinfo.tm_min);
    }else{
      minT = String(timeinfo.tm_min);
    }
    if(timeinfo.tm_sec < 10){
      secT = "0" + String(timeinfo.tm_sec);
    }else{
      secT = String(timeinfo.tm_sec);
    }
    if(timeinfo.tm_mon + 1 < 10){
      monT = "0" + String(timeinfo.tm_mon + 1);         
    }else{
      monT = String(timeinfo.tm_mon + 1);
    }
    if(timeinfo.tm_mday < 10){
      dayT = "0" + String(timeinfo.tm_mday);
    }else{
      dayT = String(timeinfo.tm_mday);
    }
    currentDay = dayT + "-" + monT + "-" + String(timeinfo.tm_year + 1900);
    currentTime = houT + ":" + minT + ":" + secT;
  }      
}
