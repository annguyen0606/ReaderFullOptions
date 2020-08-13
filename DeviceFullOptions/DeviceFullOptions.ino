#include <SPI.h>
#include "command_st95.h"
#include "conver_hex.h"
#include <WiFiUdp.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "time.h"
//#include <FirebaseESP32.h>
#include <Keypad.h>
#include "images.h"
#include "oled_0_96.h"
#include "RFID125.h"
#include "ChuongThongBao.h"
#include "LibrarySIM800.h"
#include "DateTime.h"
#include "webserver.h"

#define FUNCTION_SET_WIFI 0
#define FUNCTION_KEYPAD 15

bool initModuleSim = false;
unsigned long lastTime = 0;
unsigned long timerDelay = 1800000;
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

//FirebaseData firebaseData;
//FirebaseJson json;
char So_Tien_Pay[16];
uint8_t viTriTien = 0;
bool ESP_HTTPGET(String url, String idTag, String money);
String serverName = "http://nfcapi.conek.net/conek/dulieutest?uidTag=";
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 3600 * 7;
void SettingWifi();
uint8_t temprature_sens_read();
uint8_t TagID[8];
uint8_t UID[16];
int Type[2];
String textUID = " ";
WiFiUDP ntpUDP;

long rssi = 0;
int bars = 0;
void ClearBuff(char ArrayBuff[], int lenghtBuf);
void ClearBuff(char ArrayBuff[], int lenghtBuf){
  for(int i = 0; i < lenghtBuf; i++){
    ArrayBuff[i] = 0;
  }
}
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

bool permissSend = false;
int count125 = 4;
void setup() {
  pinMode (PIN_BUZZER, OUTPUT);
  digitalWrite (PIN_BUZZER, HIGH);
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
  while (WiFi.status() != WL_CONNECTED) {
    goalValue += 8;
    if(goalValue < 85){
      drawProgressBarDemo(currentLoad,counter, goalValue);
    }else{
      SettingWifi();
      break;
    }    
    delay(500);
  }
  Oled_print(55,20,"Wifi ready");
  delay(1000);
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  pinMode (FUNCTION_READ125, INPUT_PULLDOWN);
  pinMode (FUNCTION_SIM, INPUT_PULLDOWN);
  pinMode (FUNCTION_SET_WIFI, INPUT_PULLDOWN);
  pinMode (FUNCTION_KEYPAD, INPUT_PULLDOWN);
  //Serial.println(WiFi.macAddress());
  textUID = ReadTagNFC();
  textUID = "";
}

void loop() {
  char key;
  while(1){
    key = keypad.getKey();
    if (((millis() - lastTime) > timerDelay) && initModuleSim == true) {
      String link = "http://nfcapi.conek.net/conek/activesim";
      Oled_print(55,20,"Sending...");
      if(SIM_HTTPGET(link) == true){
        Oled_print(60,20,"Send success");
        Bip(2);
        Main_Screen(Barca_Logo_bits,bars,"Conek Welcome","4G Mode");
      }else{
        Oled_print(55,20,"Send failed");
        Bip(4);      
        Main_Screen(Barca_Logo_bits,bars,"Conek Welcome","4G Mode");
      }
      lastTime = millis();
    }
    if(digitalRead(FUNCTION_SET_WIFI) == LOW){
      Main_Screen(Barca_Logo_bits,bars,"Conek Welcome","Set Wifi");
      WiFi.disconnect();   
      WiFi.softAP(ssid, password);
      IPAddress IP = WiFi.softAPIP();
      server.begin(); 
      SettingWifi();    
    }
    if(digitalRead(FUNCTION_SIM) == HIGH){
      if(initModuleSim == true){
        Main_Screen(Barca_Logo_bits,bars,"Conek Welcome","4G Mode");
      }else{
        delay(2000);
        Oled_print(55,20,"Setup 4G");
        if(InitSIM()){
          initModuleSim = true;
          lastTime = millis();
          Oled_print(55,20,"4G ready");  
        }else{
          initModuleSim = false;
          while(1){
            Oled_print(55,20,"4G Failed");  
            delay(1000);  
            Oled_print(60,20,"Please Reset");  
            delay(1000);  
          }
        }        
      }
    }else{
      ThoiGian tg = printLocalTime();
      currentDay = tg.ngay;
      currentTime = tg.gio;
      rssi = WiFi.RSSI();
      bars = getBarsSignal(rssi);  
      if(currentDay != "" && currentTime != ""){
        Main_Screen(Barca_Logo_bits,bars,currentDay,currentTime);    
      }
    }    
    if(digitalRead(FUNCTION_KEYPAD) == HIGH){
      break;
    }
    if(key == '#'){
      break;
    }
  }
  
  if(digitalRead(FUNCTION_KEYPAD) == LOW){
    Bip(1);
    Oled_print_money(60,10,"Input Money",55,30,"");
    while(1){
      delay(5);
      key = keypad.getKey();
      if (key != NO_KEY){
        if(key == '#'){
          Bip(1);
          Oled_print(60,20,"Touch Tag");
          break;
        }else if(key == '*'){
          String tienPay = "";
          So_Tien_Pay[viTriTien - 1] = 0;
          if(viTriTien > 0){
            viTriTien--;
          }else{
            viTriTien = 0;
          }                
          for(int i = 0; i < viTriTien; i++){
              if(viTriTien > 3){
                if(viTriTien == 6 || viTriTien == 9){
                  if(i == 3 || i == 6){
                    tienPay += ",";
                    tienPay += So_Tien_Pay[i]; 
                  }else{
                    tienPay += So_Tien_Pay[i]; 
                  }
                }else if((viTriTien < 6) || (viTriTien > 6 && viTriTien < 9)){
                  if((viTriTien % 3 == i) || ((viTriTien % 3) + 3) == i){
                    tienPay += ",";  
                    tienPay += So_Tien_Pay[i]; 
                  }else{
                    tienPay += So_Tien_Pay[i]; 
                  }
                }else{
                  tienPay += So_Tien_Pay[i]; 
                }
              }else{
                tienPay += So_Tien_Pay[i]; 
              }
          }              
          Oled_print_money(60,10,"Input Money",55,30,tienPay);                        
        }else{
          if(viTriTien <= 8){
            String tienPay = "";
            viTriTien++;
            So_Tien_Pay[viTriTien - 1] = key;
            for(int i = 0; i < viTriTien; i++){
              if(viTriTien > 3){
                if(viTriTien == 6 || viTriTien == 9){
                  if(i == 3 || i == 6){
                    tienPay += ",";
                    tienPay += So_Tien_Pay[i]; 
                  }else{
                    tienPay += So_Tien_Pay[i]; 
                  }
                }else if((viTriTien < 6) || (viTriTien > 6 && viTriTien < 9)){
                  if((viTriTien % 3 == i) || ((viTriTien % 3) + 3) == i){
                    tienPay += ",";  
                    tienPay += So_Tien_Pay[i]; 
                  }else{
                    tienPay += So_Tien_Pay[i]; 
                  }
                }else{
                  tienPay += So_Tien_Pay[i]; 
                }
              }else{
                tienPay += So_Tien_Pay[i]; 
              }
            }
            Oled_print_money(60,10,"Input Money",55,30,tienPay);                   
          }else{
            Bip(3);
          }
        }
      }
    }
    while(1){
      if(digitalRead(FUNCTION_READ125) == LOW){
        if(textUID == ""){
          textUID = ReadTagNFC();
          if (textUID != "") {
            Bip(1);
            Oled_print(60,20,"Sending...");
            permissSend = true;
            break;          
          }
        }
      }else{
        if (Serial.available()) {
          char dataRev = Serial.read();
          if(dataRev >= 33 && dataRev <= 126){
            TagID[count125] = dataRev;
            count125++;
            if(count125 >= 8){
              while(Serial.available()){
                char dataTemp = Serial.read();
              }
              if (encode8byte_big_edian (TagID, UID) == 0) {
                for (count125 = 5; count125 < 8 * 2; count125++) {
                  UID[count125] += 0x30;
                  textUID += char(UID[count125]);
                }  
                count125 = 4;      
                permissSend = true;                                 
                Oled_print(60,20,"Sending...");
                Bip(1);              
                break;
              }
            }
          }
        }
      }
    }        
  }else{
      if(digitalRead(FUNCTION_READ125) == LOW){
        if(textUID == ""){
          textUID = ReadTagNFC();
          if (textUID != "") {
            Bip(1);
            permissSend = true;
            UDP_send_data(textUID,6699);
            Oled_print(60,20,"Sending...");
          }
        }
      }else{
        if (Serial.available()) {
          char dataRev = Serial.read();
          if(dataRev >= 33 && dataRev <= 126){
            TagID[count125] = dataRev;
            count125++;
            if(count125 >= 8){
              while(Serial.available()){
                char dataTemp = Serial.read();
              }
              if (encode8byte_big_edian (TagID, UID) == 0) {
                for (count125 = 5; count125 < 8 * 2; count125++) {
                  UID[count125] += 0x30;
                  textUID += char(UID[count125]);
                }  
                count125 = 4;            
                UDP_send_data(textUID,6699);   
                Oled_print(60,20,"Sending...");
                permissSend = true;
                Bip(1);    
                Serial.flush();          
              }
            }
          }
        }
      }
  }
  if(permissSend == true){
    while(1){
      if(digitalRead(FUNCTION_SIM) == HIGH){
          String tienPay = "";
          for(int i = 0; i < viTriTien; i++){
            tienPay += So_Tien_Pay[i];
          }      
          int countSendSuccess = 0;
              
          if(textUID != "" && textUID.length() > 6){
            String API = serverName + textUID + "&money=" + tienPay;
            Oled_print(55,20,"Sending...");
            while(SIM_HTTPGET(API) == false){
              countSendSuccess ++;
              delay(500);
              if(countSendSuccess > 3){
                break;
              }
            }    
            if(countSendSuccess <= 3){
              Oled_print(60,20,"Send success");
              Bip(2);
            }else{
              Oled_print(55,20,"Send failed");
              Bip(4);
            }        
            viTriTien = 0;
            ClearBuff((char*)TagID,8);
            ClearBuff(So_Tien_Pay,16);
            ClearBuff((char*)UID,16);   
            textUID = "";   
            Main_Screen(Barca_Logo_bits,bars,"Conek Welcome","4G Mode");
            break;
          }        
      }else{
        String tienPay = "";
        for(int i = 0; i < viTriTien; i++){
          tienPay += So_Tien_Pay[i];
        }      
        int countSendSuccess = 0;    
        if(textUID != "" && textUID.length() > 6){
          
          Oled_print(56,20,"Sending...");
          while(ESP_HTTPGET(serverName, textUID, tienPay) == false){
            countSendSuccess ++;
            delay(500);
            if(countSendSuccess > 3){
              break;
            }
          }
          if(countSendSuccess <= 3){
            Oled_print(55,20,"Send success");
            Bip(2);
          }else{
            Oled_print(55,20,"Send failed");
            Bip(4);
          }
          textUID = ""; 
          
          viTriTien = 0;
          ClearBuff((char*)TagID,8);
          ClearBuff(So_Tien_Pay,16);
          ClearBuff((char*)UID,16);               
        } 
        break;     
      }
    } 
    permissSend = false;   
  }
}

bool ESP_HTTPGET(String url, String idTag, String money){
  if(WiFi.status()== WL_CONNECTED){
    HTTPClient http;
    String serverPath = url + idTag + "&money=" + money;
    http.begin(serverPath.c_str());
    int httpResponseCode = http.GET();
    if (httpResponseCode == 200) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      String payload = http.getString();
      return true;
    }
    else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
      return false;
    }
    http.end();    
  }else{
    return false;
  }
}
