//25 = swingrelay, 14 = swing touch ctrl

#include <Arduino.h>                  //including all the needed libraries
#include "PinDefinitionsAndMore.h"
#include <IRremote.hpp>
#include <Wire.h>
#include <U8g2lib.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif
U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

#define DECODE_NEC                                    //comment this line and add your protocol if you use a remote with other IR protocols

#define bleServerName   "venturo 0001"
#define SERVICE_UUID    "5cfd3a85-6b69-4396-85e3-bdef0b414d0a"
#define relayState_UUID "5cfd3a86-6b69-4396-85e3-bdef0b414d0a"
#define timeSrc_UUID    "5cfd3a87-6b69-4396-85e3-bdef0b414d0a"
#define n_UUID          "5cfd3a88-6b69-4396-85e3-bdef0b414d0a"
#define timer_UUID      "5cfd3a89-6b69-4396-85e3-bdef0b414d0a"
#define swing_UUID      "5cfd3a8a-6b69-4396-85e3-bdef0b414d0a"
#define name_UUID       "5cfd3a8b-6b69-4396-85e3-bdef0b414d0a"

BLECharacteristic relayStateCharacteristic(BLEUUID(relayState_UUID), BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_WRITE_NR);
BLEDescriptor relayStateDescriptor(BLEUUID(relayState_UUID));

BLECharacteristic timeSrcCharacteristic(BLEUUID(timeSrc_UUID), BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_WRITE_NR);
BLEDescriptor timeSrcDescriptor(BLEUUID(timeSrc_UUID));

BLECharacteristic nCharacteristic(BLEUUID(n_UUID), BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_WRITE_NR);
BLEDescriptor nDescriptor(BLEUUID(n_UUID));

BLECharacteristic swingCharacteristic(BLEUUID(swing_UUID), BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_WRITE_NR);
BLEDescriptor swingDescriptor(BLEUUID(swing_UUID));

BLECharacteristic timerCharacteristic(BLEUUID(timer_UUID), BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_WRITE_NR);
BLEDescriptor timerDescriptor(BLEUUID(timer_UUID));

BLECharacteristic nameCharacteristic(BLEUUID(name_UUID), BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_WRITE);
BLEDescriptor nameDescriptor(BLEUUID(name_UUID));

bool deviceConnected = false;

unsigned long currentMillis;              //variables to store the time for incoming timing functions
unsigned long prevOnMillis;
unsigned long prevSpeedMillis;
unsigned long prevIncrementTimeSrc;
unsigned long prevDecrementTimeSrc;
unsigned long prevToggleTimer;
unsigned long prevToggleSwing;
unsigned long prevCountDownMillis;

bool relayState;      //power: on/off var
int n =0;                //speed of rotation of fan motor
int timeSrc =0;          //number of minutes in timer
bool timer;           //timer: on/off
bool swing;           //swinging or not

int speeds[]={23,18,5,4};   //pins to which the speed controlling modules (triacs) are connected


const unsigned char hourglass [] PROGMEM = {        //hourglas icon bitmap
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0xff, 0xff, 0x0f, 
	0xf0, 0xff, 0xff, 0x0f, 0xe0, 0xff, 0xff, 0x07, 0x40, 0x00, 0x00, 0x02, 0x40, 0x00, 0x00, 0x02, 
	0x40, 0x00, 0x00, 0x02, 0x40, 0xfe, 0x3f, 0x03, 0xc0, 0xfc, 0x9f, 0x01, 0x80, 0xf9, 0xcf, 0x00, 
	0x00, 0xf3, 0x67, 0x00, 0x00, 0xe6, 0x33, 0x00, 0x00, 0xcc, 0x19, 0x00, 0x00, 0xd8, 0x0d, 0x00, 
	0x00, 0x90, 0x04, 0x00, 0x00, 0x18, 0x1c, 0x00, 0x00, 0x4c, 0x30, 0x00, 0x00, 0x06, 0x60, 0x00, 
	0x00, 0x03, 0xc1, 0x00, 0x80, 0x41, 0x80, 0x01, 0xc0, 0x00, 0x04, 0x03, 0x40, 0xfe, 0x7f, 0x02, 
	0x40, 0xff, 0xff, 0x02, 0x40, 0x00, 0x00, 0x02, 0xe0, 0xff, 0xff, 0x07, 0xf0, 0xff, 0xff, 0x0f, 
	0xf0, 0xff, 0xff, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
const unsigned char nothing [] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// Setup callbacks onConnect and onDisconnect (BLE) 
class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("Device Connected");
  };
  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("Device Disconnected");
    pServer->getAdvertising()->start();
  }
};

class AllCallbacks: public BLECharacteristicCallbacks { //handle controlling this device via BLE app
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    uint16_t numValue = *(uint16_t*)value.data();
    // Serial.print(numValue);
    // Serial.print("__from__");
    // Serial.println(pCharacteristic->getUUID().toString().c_str());
    // Check which characteristic this is
    if (pCharacteristic->getUUID().equals(BLEUUID(relayState_UUID))) {
      // Handle RelayState
      relayState=!relayState;
      uint8_t relayStateForBLE = relayState ? 1 : 0;
      relayStateCharacteristic.setValue(&relayStateForBLE,1);
      relayStateCharacteristic.notify();

      uint8_t swingValueForBLE = swing&&relayState ? 1 : 0;  //since swing is dependent on relayState
      swingCharacteristic.setValue(&swingValueForBLE, 1);
      swingCharacteristic.notify();
    }
    else if (pCharacteristic->getUUID().equals(BLEUUID(timeSrc_UUID))) {
      timeSrc = numValue;
      timeSrcCharacteristic.setValue(timeSrc);
      timeSrcCharacteristic.notify(); 
    }
    else if (pCharacteristic->getUUID().equals(BLEUUID(n_UUID))) {
      // Handle N
      n = numValue ;
      nCharacteristic.setValue(n);
      nCharacteristic.notify(); 
    }
    else if (pCharacteristic->getUUID().equals(BLEUUID(timer_UUID))) {
      // Handle N
      timer=!timer;
      uint8_t timerValueForBLE = timer ? 1 : 0;
      timerCharacteristic.setValue(&timerValueForBLE, 1);
      timerCharacteristic.notify();
    }
    else if (pCharacteristic->getUUID().equals(BLEUUID(swing_UUID))) {
      // Handle N
      swing=!swing;
      uint8_t swingValueForBLE = swing&&relayState ? 1 : 0;
      swingCharacteristic.setValue(&swingValueForBLE, 1);
      swingCharacteristic.notify();
    }
    // else if (pCharacteristic->getUUID().equals(BLEUUID(name_UUID))) {
    //   // Handle N
    //   if (numValue == 1) Serial.println("hey i got the command to power toggle");
    // }
  }
};

void setup() {
  Serial.begin(9600);
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
  pinMode(13, INPUT);
  pinMode(12, INPUT);
  pinMode(14, INPUT);
  pinMode(27, INPUT);
  pinMode(33, INPUT);
  pinMode(32, INPUT);
  pinMode(25,OUTPUT);
  for(int a; a<3 ; a++){
    pinMode(speeds[a], OUTPUT);
    digitalWrite(speeds[a], false);
  }
  pinMode(4,OUTPUT);
  digitalWrite(4, false);

  u8g2.begin();
  BLEDevice::init(bleServerName);
  
  // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  
  // Create the BLE Service
  BLEService *fanService = pServer->createService(SERVICE_UUID);
  
  // Create BLE Characteristics and corresponding Descriptors
  fanService->addCharacteristic(&relayStateCharacteristic);
  relayStateCharacteristic.setCallbacks(new AllCallbacks());
  
  fanService->addCharacteristic(&timeSrcCharacteristic);
  timeSrcCharacteristic.setCallbacks(new AllCallbacks());
  
  fanService->addCharacteristic(&nCharacteristic);
  nCharacteristic.setCallbacks(new AllCallbacks());
  
  fanService->addCharacteristic(&swingCharacteristic);
  swingCharacteristic.setCallbacks(new AllCallbacks());
  
  fanService->addCharacteristic(&timerCharacteristic);
  timerCharacteristic.setCallbacks(new AllCallbacks());
  
  fanService->addCharacteristic(&nameCharacteristic);
  nameCharacteristic.setCallbacks(new AllCallbacks());
  
  fanService->start();
  
  pServer->getAdvertising()->start();
  delay(50);
}

void changerelayState() {   //function to toggle power when called
  if (currentMillis - prevOnMillis >= 1250) {
    prevOnMillis = currentMillis;
    relayState = !relayState;
    uint8_t relayStateForBLE = relayState ? 1 : 0;
    relayStateCharacteristic.setValue(&relayStateForBLE,1);
    relayStateCharacteristic.notify();

    uint8_t swingValueForBLE = swing&&relayState ? 1 : 0;   //since swing is dependent on relayState
    swingCharacteristic.setValue(&swingValueForBLE, 1);
    swingCharacteristic.notify();
  }
}

void changeSpeed() {      //function to change speed when called
  if (currentMillis-prevSpeedMillis >= 250) {
    prevSpeedMillis = currentMillis;
    n++;
    if(n > 3) n = 0;
    nCharacteristic.setValue(n);
    nCharacteristic.notify();
  }
}

void incrementTimeSrc() {   //function to increase timer amount when called by IR or touch
  if(currentMillis - prevIncrementTimeSrc >= 50){
    prevIncrementTimeSrc = currentMillis;
    timeSrc++;
  }
  if(timeSrc > 99) timeSrc=99;
  timeSrcCharacteristic.setValue(timeSrc);
  timeSrcCharacteristic.notify();
}

void decrementTimeSrc() {   //function to decrease timer amount when called by IR or touch
  if(currentMillis - prevDecrementTimeSrc >=50){
    prevDecrementTimeSrc = currentMillis;
    timeSrc = timeSrc-1;
  }
  if(timeSrc < 0) timeSrc=0;
  timeSrcCharacteristic.setValue(timeSrc);
  timeSrcCharacteristic.notify();
}

void toggleTimer() {        //turn timer on/off
  if(currentMillis - prevToggleTimer >= 250){
    prevToggleTimer = currentMillis;
    timer = !timer;
    uint8_t timerValueForBLE = timer ? 1 : 0;
    timerCharacteristic.setValue(&timerValueForBLE, 1);
    timerCharacteristic.notify();
  }
}

void toggleSwing() {        //same old
  if(currentMillis - prevToggleSwing >= 250){
    prevToggleSwing = currentMillis;
    swing = !swing;
    uint8_t swingValueForBLE = swing&&relayState ? 1 : 0;
    swingCharacteristic.setValue(&swingValueForBLE, 1);
    swingCharacteristic.notify();
  }
}

void remote() {         //to receive, decode, understand and function according to IR remote
  if (IrReceiver.decode()) {
    IrReceiver.resume();  // Early enable receiving of the next IR frame

    switch (IrReceiver.decodedIRData.command) {
      case 0x85:
       changerelayState();
        break;
      
      case 0x87:
       changeSpeed();
       break;

      case 0x84:
       incrementTimeSrc();
       break;
      
      case 0x83:
       decrementTimeSrc();
       break;

      case 0x82:
       toggleTimer();
       break;

       case 0x86:
       toggleSwing();
       break;
    }
  }
}

void ctrlPanel(){     //read what touchbuttons are contacted and function accordingly
  if(touchRead(13)<20){
    changerelayState();
  }
  if(touchRead(12)<20){
    changeSpeed();
  }
  if(touchRead(14)<20){
    toggleSwing();
  }
  if(touchRead(27)<20){
    incrementTimeSrc();
  }
  if(touchRead(33)<20){
    decrementTimeSrc();
  }
  if(touchRead(32)<20){
    toggleTimer();
  }
}



void execute() {      //timer countdown, display and misc.
 digitalWrite(speeds[n],relayState);
//  Serial.println(n);
//  Serial.println(relayState);
//  Serial.println("on");     //these serial.prints were used for troubleshooting
 digitalWrite(speeds[n-1], LOW);
//  Serial.println(n-1);
 if(n!=3) digitalWrite(speeds[3], LOW);   //the relays turns on when 0 and off when 1 (this is because most relay modules I bought are this way.)
 digitalWrite(25, swing&&relayState);   //let the fan swing only when it is spinning and commanded to swing  

 if(timer == 1) {
      if(currentMillis - prevCountDownMillis >= 60000){
        prevCountDownMillis = currentMillis;
        timeSrc = timeSrc - 1;
        if(timeSrc <= 0) timeSrc = 0;
        timeSrcCharacteristic.setValue(timeSrc);
        timeSrcCharacteristic.notify();
      }
    if(timeSrc == 0){
      relayState = 0;
      timer = 0;

      timeSrcCharacteristic.setValue(timeSrc);
      timeSrcCharacteristic.notify();

      uint8_t relayStateForBLE = relayState ? 1 : 0;
      relayStateCharacteristic.setValue(&relayStateForBLE,1);
      relayStateCharacteristic.notify();

      uint8_t timerValueForBLE = timer ? 1 : 0;
      timerCharacteristic.setValue(&timerValueForBLE, 1);
      timerCharacteristic.notify();

      uint8_t swingValueForBLE = swing&&relayState ? 1 : 0;
      swingCharacteristic.setValue(&swingValueForBLE, 1);
      swingCharacteristic.notify();
    }
  }

  u8g2.firstPage();
  do{
    u8g2.setFont(u8g2_font_inb63_mn);
    u8g2.setCursor(0, 63);      
    u8g2.print (timeSrc);  
    u8g2.setFont(u8g2_font_logisoso32_tf);
    u8g2.setCursor(98, 63);
    if(relayState){
      u8g2.print(n+1);
    } else{
    u8g2.print (0);  
    }
    if(timer){
    u8g2.drawXBM(96, 0, 32, 32, hourglass);
    } else {
    u8g2.drawXBM(96, 0, 32, 32, nothing);}
  } while ( u8g2.nextPage() );
  delay(100);
}


void loop() {     //just call the functions
  currentMillis = millis();
  remote();
  ctrlPanel();
  execute();
  delay(50);
}
 
//vidunithaedirisooriya 2025