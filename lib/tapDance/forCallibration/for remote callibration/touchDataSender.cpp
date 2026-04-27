#include <esp_now.h>
#include <WiFi.h>

//REPLACE THIS WITH YOUR RECEIVER ESP MAC ADDRESS
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

typedef struct struct_message {
  int a;
  int b;
  int c;
  int d;
  int e;
  int f;
} struct_message;

struct_message touchData;
esp_now_peer_info_t peerInfo;

 
void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    return;
  }
  
  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
}
 
void loop() {
  touchData.a=touchRead(13);
  touchData.b=touchRead(12);
  touchData.c=touchRead(14);
  touchData.d=touchRead(27);
  touchData.e=touchRead(33);
  touchData.f=touchRead(32);

  // Send message via ESP-NOW
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &touchData, sizeof(touchData));
   
  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }
  delay(2000);
}
