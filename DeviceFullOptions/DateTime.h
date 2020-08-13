typedef struct ThoiGians{
  String ngay;
  String gio;
}ThoiGian;

ThoiGians printLocalTime() {
  ThoiGian thoiGian;
  struct tm timeinfo;
  int memSecond = 0;
  String secT = "";
  String minT = "";
  String houT = "";
  String dayT = "";
  String monT = "";
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    thoiGian.ngay = "";
    thoiGian.gio = "";
    return thoiGian;
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
    thoiGian.ngay = dayT + "-" + monT + "-" + String(timeinfo.tm_year + 1900);
    thoiGian.gio = houT + ":" + minT + ":" + secT;
    return thoiGian;
  }
 
}
