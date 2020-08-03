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

#define FUNCTION_SET_WIFI 0
#define FUNCTION_KEYPAD 15

WiFiServer server(80);
const char* ssid = "Conek";
const char* password = "661phamtuantai";
String header;
unsigned long currentTimeNow = millis();
unsigned long previousTime = 0; 
const long timeoutTime = 2000;
unsigned long lastTime = 0;
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
char So_Tien_Pay[16];
uint8_t viTriTien = 0;
bool ESP_HTTPGET(String url, String idTag, String money);
String serverName = "http://nfcapi.conek.net/conek/dulieutest?uidTag=";
unsigned long timerDelay = 5000;
bool wifiConfig = false;
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
int functionRead = 0;
int functionSend = 0;
int functionKeyPad = 0;
bool permissSend = false;
int count125 = 4;
void setup() {
  Init_Module_Reader();
  pinMode (PIN_BUZZER, OUTPUT);
  digitalWrite (PIN_BUZZER, HIGH);
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
  delay(1500);
  Oled_print(55,20,"Setup 4G");
  if(InitSIM()){
    Oled_print(55,20,"4G ready");  
  }else{
    while(1){
      Oled_print(55,20,"4G Failed");  
      delay(1000);  
      Oled_print(60,20,"Please Reset");  
      delay(1000);  
    }
  }
  delay(1000);
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  
  pinMode (FUNCTION_READ125, INPUT_PULLDOWN);
  pinMode (FUNCTION_SIM, INPUT_PULLDOWN);
  pinMode (FUNCTION_SET_WIFI, INPUT_PULLDOWN);
  pinMode (FUNCTION_KEYPAD, INPUT_PULLDOWN);
  
  rssi = WiFi.RSSI();
  bars = getBarsSignal(rssi);
  Main_Screen(Barca_Logo_bits,bars,"06-06-1996","12:00:00");
  textUID = "";
}

void loop() {
  if(digitalRead(FUNCTION_KEYPAD) == HIGH){
    functionKeyPad = 1;
  }else{
    functionKeyPad = 0;
  }
  if(digitalRead(FUNCTION_SET_WIFI) == LOW){
    WiFi.disconnect();   
    WiFi.softAP(ssid, password);
    IPAddress IP = WiFi.softAPIP();
    server.begin(); 
    SettingWifi();    
  }
  if(digitalRead(FUNCTION_READ125) == HIGH){
    functionRead = 1;
  }else{
    functionRead = 0; 
  }
  if(digitalRead(FUNCTION_SIM) == HIGH){
    functionSend= 1;
  }else{
    printLocalTime();
    rssi = WiFi.RSSI();
    bars = getBarsSignal(rssi);  
    Main_Screen(Barca_Logo_bits,bars,currentDay,currentTime);    
    functionSend= 0;
  }
  switch(functionRead){
    case 0:  
      if(textUID == ""){
        textUID = ReadTagNFC();
        if (textUID != "") {
          Oled_print_money(55,10,"Input Money",55,30,"");
          Serial.println(textUID);
          Serial.println("Input Your Money: ");        
          Bip(1);
          if(functionKeyPad == 0){
            while(1){
              char key = keypad.getKey();
              if (key != NO_KEY){
                if(key == '#'){
                  permissSend = true;
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
                    tienPay += So_Tien_Pay[i];
                  }      
                  Oled_print_money(55,10,"Input Money",55,30,tienPay);                        
                }else{
                  String tienPay = "";
                  viTriTien++;
                  So_Tien_Pay[viTriTien - 1] = key;
                  for(int i = 0; i < viTriTien; i++){
                    tienPay += So_Tien_Pay[i];
                  }      
                  Oled_print_money(55,10,"Input Money",55,30,tienPay);        
                }
              }                              
            }            
          }else{
            permissSend = true;
          }
        }        
      }
      break;
    default:
      if(permissSend == false){
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
                Serial.println(textUID);
                Serial.println("Input Your Money: ");   
                Bip(1);
                Oled_print_money(55,10,"Input Money",55,30,"");     
                if(functionKeyPad == 0){
                  while(1){
                    char key = keypad.getKey();
                    if (key != NO_KEY){
                      if(key == '#'){
                        count125 = 4;
                        permissSend = true;
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
                          tienPay += So_Tien_Pay[i];
                        }      
                        Oled_print_money(55,10,"Input Money",55,30,tienPay);                        
                      }else{
                        String tienPay = "";
                        viTriTien++;
                        So_Tien_Pay[viTriTien - 1] = key;
                        for(int i = 0; i < viTriTien; i++){
                          tienPay += So_Tien_Pay[i];
                        }      
                        Oled_print_money(55,10,"Input Money",55,30,tienPay);                       
                      }
                    }                              
                  }                  
                }else{
                  permissSend = true;
                }
              }
            }
          }
        }        
      }
      break;
  }

  switch(functionSend){
    case 1:
      {
        String tienPay = "";
        for(int i = 0; i < viTriTien; i++){
          tienPay += So_Tien_Pay[i];
        }      
        int countSendSuccess = 0;
            
        if(textUID != "" && textUID.length() > 6 && permissSend == true){
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
          for(int i = 0; i < 8; i++){
            TagID[i] = 0;
          }                       
          for(int i = 0; i < 16; i++){
            So_Tien_Pay[i] = 0;
            UID[i] = 0;
          }                  
          textUID = "";   
          permissSend = false;  
          Main_Screen(Barca_Logo_bits,bars,currentDay,currentTime); 
        }        
      }
      break;
    default:
      String tienPay = "";
      for(int i = 0; i < viTriTien; i++){
        tienPay += So_Tien_Pay[i];
      }      
      int countSendSuccess = 0;    
      if(textUID != "" && textUID.length() > 6 && permissSend == true){
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
        for(int i = 0; i < 8; i++){
          TagID[i] = 0;
        }                       
        for(int i = 0; i < 16; i++){
          So_Tien_Pay[i] = 0;
          UID[i] = 0;
        }               
        permissSend = false;          
      }
      break;
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

void SettingWifi(){
  while(1){
    if(wifiConfig == true){
      break;
    }
    WiFiClient client = server.available();   // Listen for incoming clients
    if (client) {                             // If a new client connects,
      currentTimeNow = millis();
      previousTime = currentTimeNow;
      String list[20];
      int numberWifi = 0;
      Serial.println("New Client.");          // print a message out in the serial port
      String currentLine = "";                // make a String to hold incoming data from the client
      while (client.connected() && currentTimeNow - previousTime <= timeoutTime) {  // loop while the client's connected
        currentTimeNow = millis();
        if (client.available()) {             // if there's bytes to read from the client,
          char c = client.read();             // read a byte, then
          Serial.write(c);                    // print it out the serial monitor
          header += c;
          if (c == '\n') {                    // if the byte is a newline character
            if (currentLine.length() == 0) {
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/html");
              client.println("Connection: close");
              client.println();              
            
              if (header.indexOf("GET /scanWifi") >= 0 && header.indexOf("?ssid=") < 0){
                  Serial.println("Scanning Wifi");
                  numberWifi = WiFi.scanNetworks();
                  if (numberWifi == 0) {
                    Serial.println("no networks found");
                  }else {
                    Serial.print(numberWifi);
                    Serial.println(" networks found");
                    for (int i = 0; i < numberWifi; ++i) {
                      list[i] = String(WiFi.SSID(i));
                    }  
                  } 
              }else if(header.indexOf("?ssid=") >= 0){
                int fromSSID = header.indexOf("?ssid=");
                int toSSID = header.indexOf("&pass=");
                int fromPass = header.indexOf("&pass=");
                int toPass = header.indexOf(" HTTP/1.1");
                Serial.println("Phuong Hoa 1: ");
                Serial.println(header.substring(fromSSID + 6,toSSID));
                Serial.println("Phuong Hoa 2: ");
                Serial.println(header.substring(fromPass + 6,toPass));
                WiFi.mode(WIFI_STA);
                WiFi.begin(header.substring(fromSSID + 6,toSSID).c_str(), header.substring(fromPass + 6,toPass).c_str());
                int counter = 1;
                int goalValue = 0;
                int currentLoad = 0;  
                while (WiFi.status() != WL_CONNECTED) {
                  goalValue += 8;
                  if(goalValue < 85){
                    drawProgressBarDemo(currentLoad,counter, goalValue);
                  }else{
                    break;
                  }    
                  delay(500);
                }
                while(1){
                  Oled_print(56,20,"Pl, Reset Device");
                }
                client.stop();
                wifiConfig = true;
                break;                
              }
              // Display the HTML web page
              client.println("<!DOCTYPE html><html>");
              client.println("<head><title>Setting Wifi Device Conek</title></head>");
              client.println("<meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
              client.println("<body style=\"display: flex; justify-content: center;align-items: center;flex-direction: column;\"><h1>Setting Device</h1>");
              client.println("<p><a href=\"/scanWifi\">");
              client.println("<button class=\"button1\" style=\"padding: 12px 36px;color: #fff; text-decoration: none; text-transform: uppercase; font-size: 18px;border-radius: 40px; background: linear-gradient(90deg, #0162c8, #55e7fc)\">Scan Wifi</button>");
              client.println("</a>");
  
              client.println("</p>");
                        
              client.println("<form style=\"text-align: center;\">");
              client.println("<p>This is SSID WiFi <br/><input style=\"text-align: center;\" type=\"text\" id=\"txt\" name=\"ssid\" onclick=\"this.value = '' \"></p>");
              client.println("<p>This is Password Wifi <br/><input style=\"text-align: center;\" type=\"text\" id=\"txt2\" name=\"pass\" onclick=\"this.value = '' \"></p>");
              client.println("<a href=\"/configureWifi\">");
              client.println("<button class=\"button2\" style=\"padding: 12px 36px;color: #fff; text-decoration: none; text-transform: uppercase; font-size: 18px;border-radius: 40px; background: linear-gradient(90deg, #0162c8, #55e7fc)\">Connect</button>");
              client.println("</a>");
              client.println("<ul id=\"list-anc\" style=\"margin-left: -40px\">");
              
              for(int i = 0; i < numberWifi; i++){
                client.print("<li style=\"list-style: none;padding: 10px; background: #fff; box-shadow: 0 5px 25px rgba(0,0,0,.1);transition: transform 0.5s\">");
                client.print(list[i]);
                client.println("</li>");
              }
              client.println("</ul></form>");
              client.println("<footer style=\"text-align: center;\"><p>Created by <a href=\"https://www.google.com/\">ANC</a></p></footer>");
              client.println("<script type=\"text/javascript\">");
              //client.println("document.getElementsByTagName('input')[0].value = \"Input SSID Wifi\";");
              //client.println("function getAndSetVal(){var txt1 = document.getElementById('txt').value;document.getElementById('txt2').value = txt1;}");
              client.println("var items = document.querySelectorAll(\"#list-anc li\");");
              client.println("for(var i = 0; i < items.length; i++){items[i].onclick = function(){document.getElementById('txt').value = this.innerHTML;};}");
              client.println("</script></body></html>");
              client.println();
              break;
            } else { // if you got a newline, then clear currentLine
              currentLine = "";
            }
          } else if (c != '\r') {  // if you got anything else but a carriage return character,
            currentLine += c;      // add it to the end of the currentLine
          }
        }
      }
      header = "";
      // Close the connection
      client.stop();
      Serial.println("Client disconnected.");
      Serial.println("");
    }      
  }
}
