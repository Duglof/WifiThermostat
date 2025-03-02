// **********************************************************************************
// ESP8266 / ESP32 Wifi Thermostat WEB Server
// **********************************************************************************
//
// Historique : V1.0.0 2025-02-03 - Première Version Béta
//
// **********************************************************************************

#ifndef WEBCLIENT_H
#define WEBCLIENT_H

// Include main project include file
#include "WifiTherm.h"

// Exported variables/object instancied in main sketch
// ===================================================

// declared exported function from route.cpp
// ===================================================
boolean httpPost(char * host, uint16_t port, char * url, char * data);
boolean jeedomPost(void);
boolean httpRequest(void);
// String  build_mqtt_json(void);

#endif
