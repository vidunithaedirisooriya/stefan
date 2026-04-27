
#include <Arduino.h>                  //including all the needed libraries
#include "PinDefinitionsAndMore.h"
#include <IRremote.hpp>
#include <Wire.h>
#include <U8g2lib.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Preferences.h>
#include <tapDance.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif
U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

#define DECODE_NEC                                    //comment this line and add your protocol if you use a remote with other IR protocols

#define defaultBLEServerName   "venturo 0001"
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

Preferences prefs;
bool deviceConnected = false;

unsigned long currentMillis;              //variables to store the time for incoming timing functions
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

void changerelayState() {   //function to toggle power when called
    relayState = !relayState;
    uint8_t relayStateForBLE = relayState ? 1 : 0;
    relayStateCharacteristic.setValue(&relayStateForBLE,1);
    relayStateCharacteristic.notify();
    
    uint8_t swingValueForBLE = swing&&relayState ? 1 : 0;   //since swing is dependent on relayState
    swingCharacteristic.setValue(&swingValueForBLE, 1);
    swingCharacteristic.notify();
}

void changeSpeed() {      //function to change speed when called
    n++;
    if(n > 3) n = 0;
    int nForBLe = n+1;
    nCharacteristic.setValue(nForBLe);
    nCharacteristic.notify();
}

void incrementTimeSrc() {   //function to increase timer amount when called by IR or touch
  timeSrc++;
  if(timeSrc > 99) timeSrc=99;
  timeSrcCharacteristic.setValue(timeSrc);
  timeSrcCharacteristic.notify();
}

void decrementTimeSrc() {   //function to decrease timer amount when called by IR or touch
  timeSrc = timeSrc-1;
  if(timeSrc < 0) timeSrc=0;
  timeSrcCharacteristic.setValue(timeSrc);
  timeSrcCharacteristic.notify();
}

void toggleTimer() {        //turn timer on/off
  timer = !timer;
  uint8_t timerValueForBLE = timer ? 1 : 0;
  timerCharacteristic.setValue(&timerValueForBLE, 1);
  timerCharacteristic.notify();
}

void toggleSwing() {        //same old
  swing = !swing;
  uint8_t swingValueForBLE = swing&&relayState ? 1 : 0;
  swingCharacteristic.setValue(&swingValueForBLE, 1);
  swingCharacteristic.notify();
}


// Setup callbacks onConnect and onDisconnect (BLE) 
class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    // Serial.println("Device Connected");
  };
  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    // Serial.println("Device Disconnected");
    pServer->getAdvertising()->start();
  }
};

class AllCallbacks: public BLECharacteristicCallbacks { //handle controlling this device via BLE app
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    uint16_t numValue = *(uint16_t*)value.data();

    if (pCharacteristic->getUUID().equals(BLEUUID(relayState_UUID))) {
      changerelayState();
    }
    else if (pCharacteristic->getUUID().equals(BLEUUID(timeSrc_UUID))) {
      timeSrc = numValue;
      timeSrcCharacteristic.setValue(timeSrc);
      timeSrcCharacteristic.notify(); 
    }
    else if (pCharacteristic->getUUID().equals(BLEUUID(n_UUID))) {
      n = numValue -1 ;
      int nForBLe = n+1;
      nCharacteristic.setValue(nForBLe);
      nCharacteristic.notify(); 
    }
    else if (pCharacteristic->getUUID().equals(BLEUUID(timer_UUID))) {
      toggleTimer();
    }
    else if (pCharacteristic->getUUID().equals(BLEUUID(swing_UUID))) {
      toggleSwing();
    }
    else if (pCharacteristic->getUUID().equals(BLEUUID(name_UUID))) {
      String deviceName;
      for (int i = 0; i < value.length(); i++) {
         deviceName += (char)value[i];
        }
      prefs.putString("deviceName",deviceName);
    }
  }
};



void remote(void *Parameters) {         //to receive, decode, understand and function according to IR remote
  for(;;){
  if (IrReceiver.decode()) {
    
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
      vTaskDelay(500/portTICK_PERIOD_MS);
      IrReceiver.resume();  // enable receiving of the next IR frame
    }
  vTaskDelay(50/portTICK_PERIOD_MS);
}
}



tapDance touchOn13(13, 30, 40, changerelayState, NULL, NULL);
tapDance touchOn12(12, 30, 45, changeSpeed, NULL, NULL);
tapDance touchOn14(14, 30, 40, toggleSwing, NULL, NULL);
tapDance touchOn27(27, 30, 40, incrementTimeSrc, NULL, NULL);
tapDance touchOn33(33, 30, 40, decrementTimeSrc, NULL, NULL);
tapDance touchOn32(32, 30, 45, toggleTimer, NULL, NULL);


void setup() {
  Serial.begin(115200);
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
  xTaskCreate(
    remote,
    "handle inputs from ir receiver",
    4096,
    NULL,
    1,
    NULL
  );

  touchOn13.init();
  touchOn12.init();
  touchOn14.init();
  touchOn27.init();
  touchOn33.init();
  touchOn32.init();
  pinMode(25,OUTPUT);
  for(int a; a<3 ; a++){
    pinMode(speeds[a], OUTPUT);
    digitalWrite(speeds[a], false);
  }
  pinMode(4,OUTPUT);
  digitalWrite(4, false);

  u8g2.begin();
  prefs.begin("BLEname",false);
  std::string deviceName = std::string(prefs.getString("deviceName",defaultBLEServerName).c_str());
  BLEDevice::init(deviceName);
  
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


void execute() {      //timer countdown, display and misc.
  digitalWrite(speeds[n],relayState);
  //  Serial.println(n);
  //  Serial.println("on");     //these serial.prints were used for troubleshooting
  //  Serial.println(relayState);
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
    execute();
    // delay(50);
  }
  
  //vidunithaedirisooriya 2025