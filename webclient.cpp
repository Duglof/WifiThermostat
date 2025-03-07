// **********************************************************************************
// ESP8266 / ESP32 Wifi Thermostat WEB Server
// **********************************************************************************
//
// Historique : V1.0.0 2025-02-03 - Première Version Béta
//
// **********************************************************************************

#include "webclient.h"

/* ======================================================================
Function: httpPost
Purpose : Do a http post
Input   : hostname
          port
          url
          data
Output  : true if received 200 OK
Comments: -
====================================================================== */
boolean httpPost(char * host, uint16_t port, char * url, char * data)
{
  WiFiClient client;
  HTTPClient http;
  bool ret = false;
  int httpCode=0;

  unsigned long start = millis();

  // configure traged server and url
  http.begin(client, host, port, url); 
  //http.begin("http://emoncms.org/input/post.json?node=20&apikey=2f13e4608d411d20354485f72747de7b&json={PAPP:100}");
  //http.begin("emoncms.org", 80, "/input/post.json?node=20&apikey=2f13e4608d411d20354485f72747de7b&json={}"); //HTTP

  Debugf("http%s://%s:%d%s => ", port==443?"s":"", host, port, url);

  // start connection and send HTTP header
  if ( data != NULL) { // data string is not null, use POST instead of GET
    http.addHeader("Content-Type", "application/json");  //Specify content-type header
    httpCode = http.POST(data);
  } else {
    httpCode = http.GET();
  }
  if(httpCode) {
      // HTTP header has been send and Server response header has been handled
      Debug(httpCode);
      Debug(" ");
      // file found at server
      if(httpCode >= 200 && httpCode < 300) {
        String payload = http.getString();
        Debug(payload);
        ret = true;
      }
  } else {
      DebugF("failed!");
  }
  Debugf(" in %lu ms\r\n",millis()-start);
  return ret;
}

/* ======================================================================
Function: jeedomPost
Purpose : Do a http post to jeedom server
Input   : 
Output  : true if post returned 200 OK
Comments: -
====================================================================== */
boolean jeedomPost(void)
{
  boolean ret = false;

   Debugln("jeedomPost : début");
  
  // Some basic checking
  if (*config.jeedom.host) {
    String url ; 
    // boolean skip_item;
       
    url = *config.jeedom.url ? config.jeedom.url : "/";

    if (*config.jeedom.url) {
      url += F("&apikey=");
    } else {
      url += F("?apikey=");
    }
    
    url += config.jeedom.apikey;

    url.replace("%TEMP%",tempConfigToDisplay(t_current_temp));

    url.replace("%TARG%",tempConfigToDisplay(t_target_temp));

    url.replace("%HUM%",String(t_current_hum));

    url.replace("%ITEM%",String(t_current_prog_item));

    url.replace("%REL1%",String(t_relay_status));
  
    ret = httpPost( config.jeedom.host, config.jeedom.port, (char *) url.c_str(),NULL) ;
 
  } else {
   Debugln("jeedomPost : jeedom host non configuré");
  }
  return ret;
}

/* ======================================================================
Function: HTTP Request
Purpose : Do a http request
Input   : 
Output  : true if post returned 200 OK
Comments: -
====================================================================== */
boolean httpRequest(void)
{
  boolean ret = false;

  // Some basic checking
  if (*config.httpReq.host)
  {
    String url ; 

    url = *config.httpReq.path ? config.httpReq.path : "/";

    url.replace("%TEMP%",tempConfigToDisplay(t_current_temp));

    url.replace("%TARG%",tempConfigToDisplay(t_target_temp));

    url.replace("%HUM%",String(t_current_hum));

    url.replace("%ITEM%",String(t_current_prog_item));

    url.replace("%REL1%",String(t_relay_status));
      
    ret = httpPost( config.httpReq.host, config.httpReq.port, (char *) url.c_str(), NULL) ;
  } // if host
  return ret;
}
