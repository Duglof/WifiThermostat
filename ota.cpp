// **********************************************************************************
// ESP8266 / ESP32  Ota
// **********************************************************************************
//
// History : V1.00 2025-03-12 - First release
//        
// **********************************************************************************

#include "WifiTherm.h"
#include "ota.h"

/*
 * Constructor
 */
OTAClass::OTAClass() {
}

/*
 * Function: void OTA::Setup(const char* hostName, const char* passwd)
 * Purpose : Ota Setup
 * Input   : const char* hostName
 *         : const int port
 *         : const char* passwd
 *         : THandlerFunction OnStart_fn
 *         : THandlerFunction_Progress onProgress_fn
 *         : THandlerFunction onEnd_fn
 *         : THandlerFunction_Error onError_fn
 *         
 * return  -
 */

void OTAClass::Begin(const char* hostName, const uint16_t port, const char* passwd, ArduinoOTAClass::THandlerFunction otaOnStart_fn, ArduinoOTAClass::THandlerFunction_Progress otaOnProgress_fn, ArduinoOTAClass::THandlerFunction otaOnEnd_fn, ArduinoOTAClass::THandlerFunction_Error otaOnError_fn)
{
  // Set OTA parameters
  ArduinoOTA.setPort(port);
  ArduinoOTA.setHostname(hostName);
  ArduinoOTA.setPassword(passwd);
  ArduinoOTA.onStart(otaOnStart_fn);
  ArduinoOTA.onProgress(otaOnProgress_fn);
  ArduinoOTA.onEnd(otaOnEnd_fn);
  ArduinoOTA.onError(otaOnError_fn);
  
  ArduinoOTA.begin();
  
}

/*
 * Function: void OTA::Loop()
 * Purpose : Ota Loop
 * Input   : -
 *         
 * return  -
 */
void OTAClass::Loop()
{
  ArduinoOTA.handle();
}
