// **********************************************************************************
// ESP8266 / ESP32 Wifi Thermostat WEB Server
// **********************************************************************************
//
// Historique : V1.0.0 2025-02-03 - Première Version Béta
//
// **********************************************************************************

#ifndef WEBSERVER_H
#define WEBSERVER_H

// Include main project include file
#include "WifiTherm.h"

// Web response max size
#define RESPONSE_BUFFER_SIZE 4096

// Exported variables/object instancied in main sketch
// ===================================================
extern char response[];
extern uint16_t response_idx;

// declared exported function from route.cpp
// ===================================================
void handleTest(void);
void handleRoot(void); 
void handleFormConfig(void) ;
void handleFormProgHeat(void);
void handleFormProgCool(void);
void handleNotFound(void);
void tinfoJSON(void);
void getSysJSONData(String & r);
void prog_heatJSONTable(void);
void prog_coolJSONTable(void);
void sysJSONTable(void);
void getConfJSONData(String & r);
void confJSONTable(void);
void getSpiffsJSONData(String & r);
void spiffsJSONTable(void);
void sendJSON(void);
void wifiScanJSON(void);
void handleFactoryReset(void);
void handleReset(void);

#endif
