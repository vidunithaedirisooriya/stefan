#include "Arduino.h"
#include "tapDance.h"

#define poll_interval  40       //the time in milliseconds in between polls. the lesser this value is, polls are more frequent and everything is more responsive reciprocal of poll rate.
#define hold_counts  18         //(how long someone has to keep their finger in contact for it to be accepted as a touch in ms)/poll_interval
#define doubleTap_counts 6      //(acceptable time window between taps to be recognised as a double tap in ms)/poll_interval
#define until_finger_lifts 80


typedef void(*fxPtrForTouch)();

tapDance::tapDance(int touchPin, int contactThreshold, int restThreshold, fxPtrForTouch onTap, fxPtrForTouch onDoubleTap, fxPtrForTouch onHold) {
    _touchPin=touchPin;
    _contactThreshold=contactThreshold;
    _restThreshold=restThreshold;
    _currentTouchVal  = 0;
    _onTap=onTap;
    _onDoubleTap=onDoubleTap;
    _onHold=onHold;
}

void tapDance::taskEntry(void* parameter) {
    tapDance* instance = static_cast<tapDance*>(parameter);
    instance->poll_touchPin();   
}

void tapDance::poll_touchPin() {
    for (;;) {
        uint8_t prevTouchVal = _currentTouchVal;
        _currentTouchVal = touchRead(_touchPin);

        if (prevTouchVal > _restThreshold && _currentTouchVal < _contactThreshold) {
            uint8_t tempCount = 0;
            do {
                vTaskDelay(poll_interval / portTICK_PERIOD_MS);
                _currentTouchVal = touchRead(_touchPin);
                tempCount++;
            } while ((_currentTouchVal < _contactThreshold) && (tempCount <= hold_counts));

            if (tempCount > 14) {
                if(_onHold)
                _onHold();
                tempCount=0;
            } else {
                tempCount = 0;
                if(_onDoubleTap){do {
                    vTaskDelay(poll_interval / portTICK_PERIOD_MS);
                    _currentTouchVal = touchRead(_touchPin);
                    tempCount++;
                } while ((_currentTouchVal > _restThreshold) && (tempCount <= doubleTap_counts));
                }else tempCount=doubleTap_counts+1;
                if (tempCount > doubleTap_counts) {
                    if(_onTap)
                    _onTap();
                } else {
                    if(_onDoubleTap)
                    _onDoubleTap();
                }
            }
        }

        vTaskDelay(poll_interval / portTICK_PERIOD_MS);
    }
}

void tapDance::init() {
    xTaskCreate(
        taskEntry,           
        "polling Touch pin",
        4096,
        this,                
        1,
        NULL
    );
}