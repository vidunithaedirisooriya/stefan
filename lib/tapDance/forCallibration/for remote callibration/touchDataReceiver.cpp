#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

typedef struct struct_message {
    int a;
    int b;
    int c;
    int d;
    int e;
    int f;
} struct_message;

struct_message ReceivedTouchData;

void onReceiving(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&ReceivedTouchData, incomingData, sizeof(ReceivedTouchData));
  Serial.printf(">touchReada:%d, touchReadb:%d, touchReadc:%d, touchReadd:%d, touchReade:%d, touchReadf:%d,"
                ,ReceivedTouchData.a ,ReceivedTouchData.b ,ReceivedTouchData.c ,ReceivedTouchData.d ,ReceivedTouchData.e ,ReceivedTouchData.f);
  Serial.println();
    
//   Serial.print(ReceivedTouchData.a);
//   Serial.print("\t");
//   Serial.print(ReceivedTouchData.b);
//   Serial.print("\t");
//   Serial.print(ReceivedTouchData.c);
//   Serial.print("\t");
//   Serial.print(ReceivedTouchData.d);
//   Serial.print("\t");
//   Serial.print(ReceivedTouchData.e);
//   Serial.print("\t");
//   Serial.print(ReceivedTouchData.f);
//   Serial.println();
}
 
void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("something went wrong while initialising ESP_NOW");
    return;
  }
  esp_now_register_recv_cb(onReceiving);
}
 
void loop() {

}