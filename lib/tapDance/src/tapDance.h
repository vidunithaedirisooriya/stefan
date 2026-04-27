#ifndef tapDance_h
#define tapDance_h

#include "Arduino.h"

typedef void(*fxPtrForTouch)();

class tapDance {
  public:
    tapDance(int touchPin, int contactThreshold, int restThreshold, fxPtrForTouch onTap, fxPtrForTouch onDoubleTap, fxPtrForTouch onHold);
    void init();

    
  private:
    static void taskEntry(void* parameter);  // static bridge for FreeRTOS
    void poll_touchPin();   
    fxPtrForTouch _onTap;
    fxPtrForTouch _onDoubleTap;
    fxPtrForTouch _onHold;

    int     _touchPin;
    uint8_t _currentTouchVal;
    uint8_t _restThreshold;
    uint8_t _contactThreshold;
};

#endif