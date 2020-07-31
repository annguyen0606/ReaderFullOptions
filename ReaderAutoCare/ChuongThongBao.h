#define PIN_BUZZER 13

void Bip(int times){
  for(int i = 0; i < times; i++){
    digitalWrite(PIN_BUZZER, HIGH);
    delay(100);
    digitalWrite(PIN_BUZZER, LOW);
    delay(100);
  }
  digitalWrite(PIN_BUZZER, HIGH);
}
