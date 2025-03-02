// **********************************************************************************
// ESP8266 / ESP32  NetWork Time Protocol - Time Zone - Daylight Saving Time
// **********************************************************************************
//
// History : V1.00 2025-02-03 - First release
//
// **********************************************************************************

#include "ntp.h"

NTP::NTP() {
}

/*
 * tz : tz string : see https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
 * server : ntp server : ex : 
 */
void NTP::begin(char * tz, char * ntp_server) {
    this->tz = tz;
    this->ntp_server = ntp_server; 

#ifdef ESP8266
    configTime(tz, ntp_server);     // --> for the ESP8266 only
#elif defined(ESP32)
    configTime(0, 0, ntp_server);   // 0, 0 because we will use TZ in the next line
    setenv("TZ", tz, 1);        // Set environment variable with your time zone
    tzset();
#else
  #error "ce n'est ni un ESP8266 ni un ESP32"
#endif
}

/*
 * tm 
 * Return true if ok else false
*/

bool NTP::getTime(tm &timeinfo)
{
    time(&now);                     // read the current time
    if (localtime_r(&now, &timeinfo))     // update the structure tm with the current time
        return(true);
    else
        return(false);
}
