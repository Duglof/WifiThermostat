// **********************************************************************************
// ESP8266 / ESP32  Ota
// **********************************************************************************
//
// History : V1.00 2025-03-12 - First release
//        
// **********************************************************************************

#ifndef OTA_H
#define OTA_H

#include <ArduinoOTA.h>

class OTAClass {
public:
    OTAClass();

  void Begin(const char* hostName, const uint16_t port, const char* passwd, ArduinoOTAClass::THandlerFunction otaOnStart_fn, ArduinoOTAClass::THandlerFunction_Progress otaOnProgress_fn, ArduinoOTAClass::THandlerFunction otaOnEnd_fn, ArduinoOTAClass::THandlerFunction_Error otaOnError_fn);

   // Must be called in loop()
  void Loop();

};  

#endif // OTA_H
