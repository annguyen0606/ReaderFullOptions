#define FUNCTION_READ125 4
String getTag125(){
  uint8_t TagID[8];
  uint8_t UID[16];  
  uint8_t count = 4;
  String IdTag = "";
  for(int i = 0; i < 8; i++){
    TagID[i] = 0;
  }  
  while(1){
    if (Serial.available()) {
      char dataRev = Serial.read();
      if(dataRev >= 33 && dataRev <= 126){
        TagID[count] = dataRev;
        count++;
        if(count >= 8){
          break;
        }
      }
    }
  }
  if (encode8byte_big_edian (TagID, UID) == 0) {
    for (count = 5; count < 8 * 2; count++) {
      UID[count] += 0x30;
      IdTag += char(UID[count]);
    }            
  }
  return IdTag;  
}
