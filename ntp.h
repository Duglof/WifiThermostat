// **********************************************************************************
// ESP8266 / ESP32  NetWork Time Protocol - Time Zone - Daylight Saving Time
// **********************************************************************************
//
// History : V1.00 2025-02-03 - First release
//
// **********************************************************************************

#ifndef NTP_H
#define NTP_H

// we need wifi to get internet access
#ifdef ESP8266
    #include <ESP8266WiFi.h> 
#elif defined(ESP32)
    #include <WiFi.h>
#else
    #error "ce n'est ni un ESP8266 ni un ESP32"
#endif

#include <time.h>                    // for time() ctime()

class NTP {
public:
    NTP();
    
    void begin(char * tz, char *ntp_server);

    bool getTime(tm &tm);

private:
    char * ntp_server;
    char * tz;

    time_t now;                          // this are the seconds since Epoch (1970) - UTC
};

#endif // NTP_H
