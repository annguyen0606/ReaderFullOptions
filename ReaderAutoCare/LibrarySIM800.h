#define FUNCTION_SIM 2
bool Send_Receive(String dataSend, String dataCheck, unsigned int timeOut){
  String dataRev = "";
  unsigned long timeStart, timeEnd;
  timeStart = millis();
  Serial2.println(dataSend);
  while(1){
    if(Serial2.available()){
      char c = Serial2.read();
      dataRev += c;
      if(dataRev.indexOf(dataCheck) >= 0){
        break;
      }
    }
    timeEnd = millis();
    if(timeEnd - timeStart > timeOut){
      return false;
    }
  }
  while(Serial2.available()){
    Serial2.read();
  }
  delay(15);
  return true;
}

bool InitSIM(){
  int counter = 1;
  int goalValue = 0;
  int currentLoad = 0;  
    
  Serial2.begin(9600);
  delay(10000);
  if(Send_Receive("AT","OK",2000)){
    goalValue += 8;
    drawProgressBarDemo(currentLoad,counter, goalValue);
    if(Send_Receive("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"","OK",2000)){
      goalValue += 8;
      drawProgressBarDemo(currentLoad,counter, goalValue);
      if(Send_Receive("AT+SAPBR=3,1,\"APN\",\"e-connect\"","OK",2000)){
        goalValue += 8;
        drawProgressBarDemo(currentLoad,counter, goalValue);        
        if(Send_Receive("AT+SAPBR=1,1","OK",2000)){
          goalValue += 8;
          drawProgressBarDemo(currentLoad,counter, goalValue);          
          if(Send_Receive("AT+HTTPINIT","OK",2000)){
             return true;
          }else{
            return false;
          }
        }else{
          return false;
        }
      }else{
        return false;
      }
    }else{
      return false;
    }
  }else{
    return false;
  }
}

bool SIM_HTTPGET(String API){
  String url = "AT+HTTPPARA=\"URL\",\"" + API +"\"";
  if(Send_Receive(url,"OK",3000)){
    if(Send_Receive("AT+HTTPACTION=0","200",8000)){
      return true;
    }else{
      return false;
    }
  }else{
    return false;
  }
}
