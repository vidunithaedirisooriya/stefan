//23 = swingrelay, 14 = swing touch ctrl

#include <Arduino.h>                  //including all the needed libraries
#include "PinDefinitionsAndMore.h"
#include <IRremote.hpp>
#include <Wire.h>
#include <U8g2lib.h>
#include "BluetoothSerial.h"

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif
U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

#define DECODE_NEC                                    //comment this line if you use other protocols

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif
BluetoothSerial ESP_BT;


unsigned long currentMillis;              //variables to store the time for incoming timing functions
unsigned long prevOnMillis;
unsigned long prevSpeedMillis;
unsigned long prevIncrementTimeSrc;
unsigned long prevDecrementTimeSrc;
unsigned long prevToggleTimer;
unsigned long prevToggleSwing;
unsigned long prevCountDownMillis;

bool relayState;      //power: on/off var
int n =0;                //speed
int timeSrc =0;          //number of minutes in timer
bool timer;           //timer: on/off
bool swing;           //swinging or not

int speeds[]={25,4,5,18};   //pins to which the speed controlling relays are connected

int incoming;         //to store BT message

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

void setup() {
  Serial.begin(9600);
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
  ESP_BT.begin("fan_ESP32");
  pinMode(13, INPUT);
  pinMode(12, INPUT);
  pinMode(14, INPUT);
  pinMode(27, INPUT);
  pinMode(33, INPUT);
  pinMode(32, INPUT);
  pinMode(23,OUTPUT);
  for(int a; a<3 ; a++){
    pinMode(speeds[a], OUTPUT);
    digitalWrite(speeds[a], false);
  }
  pinMode(18,OUTPUT);
  digitalWrite(18, false);

  u8g2.begin();
  delay(500);
}

void changerelayState() {   //function to toggle power when called
  if (currentMillis - prevOnMillis >= 1250) {
    prevOnMillis = currentMillis;
    relayState = !relayState;
    ESP_BT.write(102 - relayState);
  }
}

void changeSpeed() {      //function to change speed when called
  if (currentMillis-prevSpeedMillis >= 250) {
    prevSpeedMillis = currentMillis;
    n++;
    if(n > 3){ n = 0; }
    ESP_BT.write(104 + n);
  }
}

void incrementTimeSrc() {   //function to increase timer amount when called by IR or touch
 if(currentMillis - prevIncrementTimeSrc >= 50){
  prevIncrementTimeSrc = currentMillis;
  timeSrc++;
  ESP_BT.write(timeSrc);
 }
 if(timeSrc > 99){
  timeSrc=99;
 }
}

void decrementTimeSrc() {   //function to decrease timer amount when called by IR or touch
  if(currentMillis - prevDecrementTimeSrc >=50){
    prevDecrementTimeSrc = currentMillis;
    timeSrc = timeSrc-1;
    ESP_BT.write(timeSrc);
  }
  if(timeSrc < 0) {
    timeSrc=0;
  }
}

void toggleTimer() {        //turn timer on/off
  if(currentMillis - prevToggleTimer >= 250){
    prevToggleTimer = currentMillis;
   timer = !timer;
   ESP_BT.write(112 + timer);
  }
}

void toggleSwing() {        //same old
  if(currentMillis - prevToggleSwing >= 250){
    prevToggleSwing = currentMillis;
   swing = !swing;
   ESP_BT.write(109 + timer);
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

void BTctrl() {       //input bluetooth functions and update app display
  if (ESP_BT.available()) {
    incoming = ESP_BT.read();
    switch (incoming) {
      case 100:
        relayState = !relayState;
        ESP_BT.write(102 - relayState);   //update app that it changed succesfully
       break;
      case 103:
        if (n < 3) {
          n++;
        } else {
          n = 0;
        }
        ESP_BT.write(104 + n);
       break;
      case 108:
        swing = !swing;
        ESP_BT.write(109 + swing);
       break;
      case 111:
        timer = !timer;
        ESP_BT.write(112 + timer);
       break;
      case 0 ... 99:
        timeSrc = incoming;
      break;
    }
  }
}

void execute() {      //timer countdown, display and misc.
 digitalWrite(speeds[n],relayState);
 Serial.println(n);
 Serial.println(relayState);
 Serial.println("on");
 digitalWrite(speeds[n-1], LOW);
 Serial.println(n-1);
 if(n!=3){digitalWrite(speeds[3], LOW);}
digitalWrite(23, swing);     //the relays turns on when 0 and off when 1 (this is because most relay modules I bought are this way.)

 if(timer == 1) {
  if(currentMillis - prevCountDownMillis >= 60000){
    prevCountDownMillis = currentMillis;
    timeSrc = timeSrc - 1;
    if(timeSrc <= 0){
      timeSrc = 0;
    }
  }
  if(timeSrc == 0){
    relayState = 0;
    timer = 0;
  }
  ESP_BT.write(timeSrc);    //update BT app indicator
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
  BTctrl();

}
 
//vidunithaedirisooriya 2025