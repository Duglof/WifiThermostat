// **********************************************************************************
// ESP8266 / ESP32  Digital Relay
// **********************************************************************************
//
// History : V1.00 2025-02-03 - First release
//
// **********************************************************************************


#include "relay.h"


DigitalRelay::DigitalRelay() {
}

/*
 * pin : gpio pin number 
 * inverted : true or false
 *   true  : gpio LOW ; Relay Activated
 *   false : gpio HIGH : Relay Activated
*/

void DigitalRelay::begin( int pin, bool inverted)

{
  _pin = pin;
  _inverted = inverted;

  pinMode(_pin, OUTPUT);

  Off();
}

/*
 * Relay On
 */
void DigitalRelay::On()
{
  if (_inverted)
    digitalWrite(_pin, LOW);    // Active LOW relay
  else
    digitalWrite(_pin, HIGH);   // Active HIGHT relay
}

/*
 * Relay Off
 */
void DigitalRelay::Off()
{
  if (_inverted)
    digitalWrite(_pin, HIGH);   // Active LOW relay
  else
    digitalWrite(_pin, LOW);    // Active HIGHT relay
  
}
