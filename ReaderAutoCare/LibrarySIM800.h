#define FUNCTION_SIM 2
bool Send_Receive(String dataSend, String dataCheck){
  String dataRev = "";
  Serial2.println(dataSend);
  while(1){
    if(Serial2.available()){
      char c = Serial2.read();
      dataRev += c;
      if(dataRev.indexOf(dataCheck) >= 0){
        delay(15);
        return true;
      }
    }
  }
}

bool InitSIM(){
  Serial2.begin(9600);
  delay(10000);
  if(Send_Receive("AT","OK")){
    if(Send_Receive("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"","OK")){
      if(Send_Receive("AT+SAPBR=3,1,\"APN\",\"e-connect\"","OK")){
        if(Send_Receive("AT+SAPBR=1,1","OK")){
          if(Send_Receive("AT+HTTPINIT","OK")){
             return true;
          }
        }
      }
    }
  }
}

bool SIM_HTTPGET(String API){
  String url = "AT+HTTPPARA=\"URL\",\"" + API +"\"";
  if(Send_Receive(url,"OK")){
    if(Send_Receive("AT+HTTPACTION=0","200")){
      return true;
    }
  }
}
