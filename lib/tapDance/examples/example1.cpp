//remember to set the serial monitor to 115200 baud rate
#include <Arduino.h>
#include <tapDance.h>

void onTap(){
  Serial.println("single tap detected on touchPin");
}

void onDoubleTap(){
  Serial.println("double tap detected on touchPin");
  
}
void onHold(){
  Serial.println("hold detected on touchPin");
  
}

tapDance touchOn13(13, 30, 40, onTap, onDoubleTap, onHold);


void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("Serial began");
  touchOn13.init();
  pinMode(LED_BUILTIN,OUTPUT);
}

void loop() {
  delay(5000);
  digitalWrite(LED_BUILTIN,HIGH);
  delay(5000);
  digitalWrite(LED_BUILTIN,LOW);
}
