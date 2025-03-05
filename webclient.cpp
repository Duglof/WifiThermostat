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
Function: build_mqtt_json string (usable by webserver.cpp)
Purpose : construct the json part of mqtt url
Input   : -
Output  : String if some Teleinfo data available
Comments: -
====================================================================== */
// Non utilisé : mqtt data sent by DataCallback()
/* exemple data
 *  From https://forum.hacf.fr/t/mqtt-instable-linky/21008
 * {« TIC »:{« ADSC »:« 0222761xxxx »,« VTIC »:2,« NGTF »:« TEMPO »,« LTARF »:« HP BLEU »,« EAST »:999935,« EASF01 »:834119,« EASF02 »:30571,« EASF03 »:60434,« EASF04 »:58796,« EASF05 »:8101,« EASF06 »:7914,« EASF07 »:0,« EASF08 »:0,« EASF09 »:0,« EASF10 »:0,« EASD01 »:902654,« EASD02 »:97281,« EASD03 »:0,« EASD04 »:0,« IRMS1 »:2,« IRMS2 »:1,« IRMS3 »:1,« URMS1 »:239,« URMS2 »:240,« URMS3 »:238,« PREF »:12,« PCOUP »:12,« SINSTS »:659,« SINSTS1 »:378,« SINSTS2 »:125,« SINSTS3 »:154,« SMAXSN »:7270,« SMAXSN1 »:2850,« SMAXSN2 »:2110,« SMAXSN3 »:2300,« SMAXSN-1 »:7190,« SMAXSN1-1 »:2830,« SMAXSN2-1 »:2090,« SMAXSN3-1 »:2300,« CCASN »:528,« CCASN-1 »:522,« UMOY1 »:239,« UMOY2 »:240,« UMOY3 »:238,« STGE »:« 013A4401 »,« DPM1 »:0,« FPM1 »:0,« PRM »:2147483647,« RELAIS »:0,« NTARF »:2,« NJOURF »:0,« NJOURF+1 »:0,« PPOINTE »:« 00004003 06004004 16004003 NONUTILE NONUTILE NONUTILE NONUTILE NONUTILE NONUTILE NONUTILE NONUTILE »,« SINTS3 »:180,« UMS1 »:241,« ESD02 »:88269}}
*/

/*
String build_mqtt_json(void)
{
  boolean first_item = true;
  boolean skip_item;

  String mqtt_data = "{\"TIC\":{" ;

  ValueList * me = tinfo.getList();

      // Loop thru the node
      while (me->next) {
        // go to next node
        me = me->next;
        skip_item = false;

        // Si Item virtuel, on le met pas
        if (*me->name =='_')
          skip_item = true;

        // On doit ajouter l'item ?
        if (!skip_item) {
          if (first_item) {
            first_item = false; 
            mqtt_data += "\"";
          } else {
            mqtt_data += ",\"";
          }
          mqtt_data += String(me->name);
          mqtt_data += "\":\"";
          mqtt_data += String(me->value);
          mqtt_data += "\"";
        }
      } // While me
      // Json end
      mqtt_data += "}}";
      Debugln(mqtt_data);
      
  return mqtt_data;
}
*/


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
      url += "?";
      ret = httpPost( config.httpReq.host, config.httpReq.port, (char *) url.c_str(), NULL) ;
  } // if host
  return ret;
}
