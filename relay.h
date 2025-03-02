// **********************************************************************************
// ESP8266 / ESP32  Digital Relay
// **********************************************************************************
//
// History : V1.00 2025-02-03 - First release
//
// **********************************************************************************

#ifndef DIGITAL_RELAY_H
#define DIGITAL_RELAY_H

#include <Arduino.h>

#define DIGITAL_RELAY_INVERTED    1
#define DIGITAL_RELAY_NORMAL      0


class DigitalRelay {
public:
    DigitalRelay();
    
    void begin( int pin, bool inverted);    // pin for digitalWrite()

    bool  _inverted;  // DIGITAL_RELAY_INVERTED or DIGITAL_RELAY_NORMAL

    void On();      // Set Relay On
    void Off();     // Set Relay Off
    
private:
  int _pin;
};

#endif // DIGITAL_RELAY_H
