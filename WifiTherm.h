// **********************************************************************************
// ESP8266 / ESP32  WifiThermostat WEB Server global Include file
// **********************************************************************************
//
// History : V1.00 2025-02-03 - First release
//
// **********************************************************************************
#ifndef WIFITHERM_H
#define WIFITHERM_H

// File System to Use LittleFS

// Include Arduino header
#include <Arduino.h>
#ifdef ESP8266
  // ESP8266
  #include <ESP8266WiFi.h>
  #include <ESP8266mDNS.h>
  #include <ESP8266WebServer.h>
  #include <ESP8266HTTPClient.h>
  #include <ESP8266mDNS.h>
  #include "LittleFS.h"
#elif defined(ESP32)
  // ESP32
  #include <WiFi.h>
  #include <ESPmDNS.h>
  #include <ESP32WebServer.h>    // https://github.com/Pedroalbuquerque/ESP32WebServer download and place in your Libraries folder
  #include <HTTPClient.h>
  #include <ESPmDNS.h>
  #include <esp_wifi.h>

  #include "LittleFS.h"
#else
  #error "ce n'est ni un ESP8266 ni un ESP32"
#endif

#include "ntp.h"    // Network Time

#include <WiFiUdp.h>
#include <EEPROM.h>
#include <Ticker.h>
#include <FS.h>

#ifdef ESP8266
// ESP8266
extern "C" {
  #include "user_interface.h"
}
#else

// ESP32 : Vérifier si tous nécessaires !!!
extern "C" {
  #include "esp_system.h"
  // #include "esp_spi_flash.h"  deprecated
  #include "spi_flash_mmap.h"
}
#endif

#include "webserver.h"
#include "webclient.h"
#include "config.h"

//=====================================================
// Temperature Sensors
// Define only one sensor type
//=====================================================
// Décommenter THER_SIMU (et commenter les autres) pour compiler une version de test
// Le thermostat ne lit plus le capteur de température / humidité
// Les valeurs affichée sont des valeurs simulées

// #define  THER_SIMU
// #define  THER_DS18B20
// #define  THER_BME280
#define   THER_HTU21

//======== DS18B20 ==============
#ifdef THER_DS18B20
#ifdef ESP8266
  #define PIN_DQ_DS18B20  13
#else
  #define PIN_DQ_DS18B20  15
#endif
#endif

//========== BME280 ==============
#define bme280_SensorAddress   0x76    // Use 0x77 for an Adafruit variant

//==========  RELAY PIN ============
// see file esp32-hal.h for config define
#ifdef ESP8266
  #pragma message "xxxxxxx Compilation pour ESP8266"
  #define   PIN_RELAY_1     12
  // #define   PIN_RELAY_2     14
#elif defined(CONFIG_IDF_TARGET_ESP32C3)
  #pragma message "xxxxxxx Compilation pour CONFIG_IDF_TARGET_ESP32C3"
  #error you must define relay pin for esp32-c3 
#elif defined(CONFIG_IDF_TARGET_ESP32C6)
  #pragma message "xxxxxxx Compilation pour CONFIG_IDF_TARGET_ESP32C6"
  // relay pin definition for esp32-c6
  #define   PIN_RELAY_1     6
#elif defined(CONFIG_IDF_TARGET_ESP32)
  #pragma message "xxxxxxx Compilation pour CONFIG_IDF_TARGET_ESP32"
  // relay pin definition for esp32 standard
  #define   PIN_RELAY_1     32
  // #define   PIN_RELAY_2     33
#else
 #error you must define relay pin 
#endif

//=========== LED PIN ==============
// esp8266 12E : LED_BUILTIN : blue led on gpio 2
#ifdef LED_BUILTIN
#define BLU_LED_PIN    LED_BUILTIN
#else
// Pour ESP32-WROOM-DA module ce n'est pas déclaré 
#define BLU_LED_PIN  2
#endif

// red led is declare on pin 14 in place off relay 2
// #define RED_LED_PIN    14

#define BLINK_LED_MS   50 // 50 ms blink


// Décommenter DEBUG pour une version capable d'afficher du Debug
//  sur Serial
#define DEBUG

// Décommenter SYSLOG pour une version capable d'envoyer du Debug
//  vers un serveur rsyslog du réseau
// #define SYSLOG



// ESP8266 ESP32
#define DEBUG_SERIAL  Serial

#define WIFITERM_VERSION "1.0.0"

// I prefix debug macro to be sure to use specific for THIS library
// debugging, this should not interfere with main sketch or other 
// libraries
#ifdef SYSLOG
  #include <Syslog.h>

  #define APP_NAME "WifiTherm"

  #define Debug(x)     Myprint(x)
  #define Debugln(x)   Myprintln(x)
  #define DebugF(x)    Myprint(F(x))
  #define DebuglnF(x)  Myprintln(F(x))

  // Ceci compile et transmet les arguments
  #define Debugf(...)    Myprintf(__VA_ARGS__)

  #define Debugflush() Myflush()
#else
  #ifdef DEBUG
    #define Debug(x)     DEBUG_SERIAL.print(x)
    #define Debugln(x)   DEBUG_SERIAL.println(x)
    #define DebugF(x)    DEBUG_SERIAL.print(F(x))
    #define DebuglnF(x)  DEBUG_SERIAL.println(F(x))
    #define Debugf(...)  DEBUG_SERIAL.printf(__VA_ARGS__)
    #define Debugflush() DEBUG_SERIAL.flush()
  #else
    #define Debug(x) 
    #define Debugln(x)
    #define DebugF(x) 
    #define DebuglnF(x) 
    #define Debugf(...)
    #define Debugflush()
  #endif
#endif // SYSLOG

// value for HSL color
// see http://www.workwithcolor.com/blue-color-hue-range-01.htm
#define COLOR_RED             0
#define COLOR_ORANGE         30
#define COLOR_ORANGE_YELLOW  45
#define COLOR_YELLOW         60
#define COLOR_YELLOW_GREEN   90
#define COLOR_GREEN         120
#define COLOR_GREEN_CYAN    165
#define COLOR_CYAN          180
#define COLOR_CYAN_BLUE     210
#define COLOR_BLUE          240
#define COLOR_BLUE_MAGENTA  275
#define COLOR_MAGENTA	      300
#define COLOR_PINK		      350

// blue led
#ifdef BLU_LED_PIN
#define LedBluON()  {digitalWrite(BLU_LED_PIN, 0);}
#define LedBluOFF() {digitalWrite(BLU_LED_PIN, 1);}
#else
#define LedBluON()  {}
#define LedBluOFF() {}
#endif
// GPIO 14 red led
#ifdef RED_LED_PIN
#define LedRedON()  {digitalWrite(RED_LED_PIN, 1);}
#define LedRedOFF() {digitalWrite(RED_LED_PIN, 0);}
#else
#define LedRedON()  {}
#define LedRedOFF() {}
#endif

// sysinfo informations
typedef struct 
{
  String sys_uptime;
} _sysinfo;

// Exported variables/object instancied in main sketch
// ===================================================
#ifdef ESP8266
  extern ESP8266WebServer server;
#else
  extern ESP32WebServer server;
#endif

extern WiFiUDP OTA;
extern unsigned long seconds;
extern _sysinfo sysinfo;
extern  Ticker  Tick_mqtt;
extern  Ticker  Tick_jeedom;
extern  Ticker  Tick_httpRequest;
extern  String  optval;     // On conserve le même nom
extern  NTP     ntp;
extern  int16_t t_current_temp;           // Temperature en dixième de degré : 175 = 17.5°C
extern  int16_t t_current_hum;            // Humidité de 0 à 100%
extern  int16_t t_target_temp;
extern  int16_t t_current_prog_item;
extern  int8_t  t_relay_status;
extern  int8_t  t_errors;

// Exported function located in main sketch
// ===================================================
void ResetConfig(void);
void Task_mqtt();
void Task_jeedom();
void Task_httpRequest();

// Exported function located in webserver
// ===================================================
extern String tempConfigToDisplay(int16_t i_temp);

#ifdef SYSLOG
void Myprint(void);
void Myprint(char *msg);
void Myprint(const char *msg);
void Myprint(String msg);
void Myprint(int i);
void Myprint(unsigned int i);
void Myprintf(const char * format, ...);
void Myprintln(void);
void Myprintln(char *msg);
void Myprintln(const char *msg);
void Myprintln(String msg);
void Myprintln(const __FlashStringHelper *msg);
void Myprintln(int i);
void Myprintln(unsigned int i);
void Myprintln(unsigned long i);
void Myflush(void);
#endif  // SYSLOG

#endif  // WIFITHERM_H
