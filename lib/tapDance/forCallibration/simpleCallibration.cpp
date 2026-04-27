#include <Arduino.h>

void setup() {
    Serial.begin(115200);
}

void loop(){
    Serial.printf(">touchReada:%d, touchReadb:%d, touchReadc:%d, touchReadd:%d, touchReade:%d, touchReadf:%d,"
                ,touchRead(13) ,touchRead(12) ,touchRead(14) ,touchRead(27) ,touchRead(33) ,touchRead(32));
    Serial.println();
    delay(40);
  
}