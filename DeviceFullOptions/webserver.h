WiFiServer server(80);
bool wifiConfig = false;
const char* ssid = "Conek";
const char* password = "661phamtuantai";
String header;
unsigned long currentTimeNow = millis();
unsigned long previousTime = 0; 
const long timeoutTime = 2000;

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
                  Oled_print(60,20,"Pl, Reset Device");
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
