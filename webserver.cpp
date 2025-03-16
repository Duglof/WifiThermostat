// **********************************************************************************
// ESP8266 / ESP32 Wifi Thermostat WEB Server
// **********************************************************************************
//
// Historique : V1.0.0 2025-02-03 - Première Version Béta
//
//              V1.0.1 2025-03-10 - Add Thermostat errors
// **********************************************************************************

// Include Arduino header
#include "webserver.h"

// Optimize string space in flash, avoid duplication
const char FP_JSON_START[] PROGMEM = "{\r\n";
const char FP_JSON_END[] PROGMEM = "\r\n}\r\n";
const char FP_Q[] PROGMEM = "\"";
const char FP_QCQ[] PROGMEM = "\":\"";
const char FP_QCNL[] PROGMEM = "\",\r\n\"";
const char FP_RESTART[] PROGMEM = "OK, Redémarrage en cours\r\n";
const char FP_NL[] PROGMEM = "\r\n";

/* ======================================================================
Function: formatSize 
Purpose : format a asize to human readable format
Input   : size 
Output  : formated string
Comments: -
====================================================================== */
String formatSize(size_t bytes)
{
  if (bytes < 1024){
    return String(bytes) + F(" Byte");
  } else if(bytes < (1024 * 1024)){
    return String(bytes/1024.0) + F(" KB");
  } else if(bytes < (1024 * 1024 * 1024)){
    return String(bytes/1024.0/1024.0) + F(" MB");
  } else {
    return String(bytes/1024.0/1024.0/1024.0) + F(" GB");
  }
}

/* ======================================================================
Function: getContentType 
Purpose : return correct mime content type depending on file extension
Input   : -
Output  : Mime content type 
Comments: -
====================================================================== */
String getContentType(String filename) {
  if(filename.endsWith(".htm")) return F("text/html");
  else if(filename.endsWith(".html")) return F("text/html");
  else if(filename.endsWith(".css")) return F("text/css");
  else if(filename.endsWith(".json")) return F("text/json");
  else if(filename.endsWith(".js")) return F("application/javascript");
  else if(filename.endsWith(".png")) return F("image/png");
  else if(filename.endsWith(".gif")) return F("image/gif");
  else if(filename.endsWith(".jpg")) return F("image/jpeg");
  else if(filename.endsWith(".ico")) return F("image/x-icon");
  else if(filename.endsWith(".xml")) return F("text/xml");
  else if(filename.endsWith(".pdf")) return F("application/x-pdf");
  else if(filename.endsWith(".zip")) return F("application/x-zip");
  else if(filename.endsWith(".gz")) return F("application/x-gzip");
  else if(filename.endsWith(".otf")) return F("application/x-font-opentype");
  else if(filename.endsWith(".eot")) return F("application/vnd.ms-fontobject");
  else if(filename.endsWith(".svg")) return F("image/svg+xml");
  else if(filename.endsWith(".woff")) return F("application/x-font-woff");
  else if(filename.endsWith(".woff2")) return F("application/x-font-woff2");
  else if(filename.endsWith(".ttf")) return F("application/x-font-ttf");
  return "text/plain";
}

/*
 *  String tempConfigToDisplay(int16_t i_temp)
 * 
 * i_temp : temperature value in value in tenths of a degree
 * 
 * return Sring value in celsius or fahrenheit with one decimal 
 *   191 => 19.1 for config in ° celsuis
 *   191 => 66.3 for config in ° fahrenheit
 * 
 * use global thermostat parameter config.thermostat.options : bit t_option_fahrenheit
 * all temperature value are stored in °C in thermostat configuration
 * 
 */
String tempConfigToDisplay(int16_t i_temp)
{
String value;

  if (config.thermostat.options & t_option_fahrenheit)
    value = String((float)i_temp * 9.0 / 10.0 / 5.0 + 32.0 ,1);
  else
    value = String((float)i_temp / 10.0,1);

  return(value);
}

/*
 * int16_t tempDisplayToConfig(String i_temp)
 * 
 * i_temp : temperature Sring value in celsius or fahrenheit with one decimal (ex : 19.1) 
 * 
 * return value value in tenths of a degree
 * 
 * use global thermostat parameter config.thermostat.options : bit t_option_fahrenheit
 * arrondi à l'entier le plus proche en ajoutant 0.5
 */
 
int16_t tempDisplayToConfig(float i_temp)
{
int16_t value;

  if (config.thermostat.options & t_option_fahrenheit)
    value = (( i_temp - 32.0 ) * 5.0 * 10.0 / 9.0) + 0.5;
  else  
    value = (i_temp * 10.0) + 0.5;
    
  return(value);
}

/* ======================================================================
Function: handleFileRead 
Purpose : return content of a file stored on LittleFS file system
Input   : file path
Output  : true if file found and sent
Comments: -
====================================================================== */
bool handleFileRead(String path) {
  if ( path.endsWith("/") ) 
    path += "index.htm";
  
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";

  DebugF("handleFileRead ");
  Debug(path);

  if(LittleFS.exists(pathWithGz) || LittleFS.exists(path)) {
    if( LittleFS.exists(pathWithGz) ){
      path += ".gz";
      DebugF(".gz");
    }

    DebuglnF(" found on FS");
 
    File file = LittleFS.open(path, "r");
    server.streamFile(file, contentType);
    file.close();
    return true;
  }

  Debugln(" File Not Found");

  server.send(404, "text/plain", "File Not Found");
  return false;
}

/* ======================================================================
Function: handleFormProgHeat 
Purpose : handle Prog Heat page
Input   : -
Output  : - 
Comments: -
====================================================================== */
void handleFormProgHeat(void)
{
String response="";
int ret ;
char arg[32]; 
int h,m;

  // Default temp heat
  if ( server.hasArg(CFG_FORM_THER_PROG_HEAT_DEFAULT) ) {
    String value = server.arg(CFG_FORM_THER_PROG_HEAT_DEFAULT); 
    // Debugf("%s = %s\n", arg, value.c_str());
    config.thermostat.t_prog_heat_default = value.toFloat() * 10;
  }

  // All prog heat items
  for ( int program = 0 ; program < CFG_THER_PROG_COUNT ; program++) {
    // All days
    for ( int wday = 0; wday < 8 ; wday++) {
      sprintf(arg , "p%d_wday_%d", program, wday);
      if ( server.hasArg(arg) ) {
        String value = server.arg(arg); 
        // Debugf("%s = %s\n", arg, value.c_str());
        if (value == "true") {
          // Debugln("Set bit");
          config.thermostat.t_prog_heat[program].days |= (1 << wday); // wday 0 -> bit 0, wday 1 -> bit 1, etc
        } else {
          // Debugln("Reset bit");
          config.thermostat.t_prog_heat[program].days &= ~(1 << wday);
        }
        // Debugf("days = %02x\n", config.thermostat.t_prog_heat[program].days);
      }
    }

    sprintf(arg, "p%d_temperature", program);
    if ( server.hasArg(arg) ) {
      String value = server.arg(arg); 
      // Debugf("%s = %s\n", arg, value.c_str());
      config.thermostat.t_prog_heat[program].t_temp = tempDisplayToConfig(value.toFloat());
    }
    sprintf(arg, "p%d_time_d", program);
    if ( server.hasArg(arg) ) {
      String value = server.arg(arg); 
      Debugf("%s = %s\n", arg, value.c_str());
      h = 0; m = 0;
      sscanf(value.c_str(), "%2d:%2d", &h, &m);
      Debugf("h = %d; m = %d\n", h, m);
      config.thermostat.t_prog_heat[program].h_begin = h * 60 + m;    // minutes depuis le debut du jour
    }
     sprintf(arg, "p%d_time_f", program);
    if ( server.hasArg(arg) ) {
      String value = server.arg(arg);
      // Debugf("%s = %s\n", arg, value.c_str());
      h = 0; m = 0;
      sscanf(value.c_str(), "%2d:%2d", &h, &m);
      config.thermostat.t_prog_heat[program].h_end = h * 60 + m;      // minutes depuis le debut du jour
    }
   
  }

  if ( saveConfig() ) {
    ret = 200;
    response = "OK";
  } else {
    ret = 412;
    response = "Unable to save configuration";
  }

  server.send ( ret, "text/plain", response);

}

/* ======================================================================
Function: handleFormProgCool 
Purpose : handle Prog Cool page
Input   : -
Output  : - 
Comments: -
====================================================================== */
void handleFormProgCool(void)
{
String response="";
int ret ;
char arg[32]; 
int h,m;

  // Default temp cool
  if ( server.hasArg(CFG_FORM_THER_PROG_COOL_DEFAULT) ) {
    String value = server.arg(CFG_FORM_THER_PROG_COOL_DEFAULT); 
    // Debugf("%s = %s\n", arg, value.c_str());
    config.thermostat.t_prog_cool_default = value.toFloat() * 10;
  }

  // All prog cool items
  for ( int program = 0 ; program < CFG_THER_PROG_COUNT ; program++) {
    // All days
    for ( int wday = 0; wday < 8 ; wday++) {
      sprintf(arg , "p%d_wday_%d", program, wday);
      if ( server.hasArg(arg) ) {
        String value = server.arg(arg); 
        // Debugf("%s = %s\n", arg, value.c_str());
        if (value == "true") {
          // Debugln("Set bit");
          config.thermostat.t_prog_cool[program].days |= (1 << wday); // wday 0 -> bit 0, wday 1 -> bit 1, etc
        } else {
          // Debugln("Reset bit");
          config.thermostat.t_prog_cool[program].days &= ~(1 << wday);
        }
        // Debugf("days = %02x\n", config.thermostat.t_prog_cool[program].days);
      }
    }

    sprintf(arg, "p%d_temperature", program);
    if ( server.hasArg(arg) ) {
      String value = server.arg(arg); 
      // Debugf("%s = %s\n", arg, value.c_str());
      config.thermostat.t_prog_cool[program].t_temp = tempDisplayToConfig(value.toFloat());
    }
    sprintf(arg, "p%d_time_d", program);
    if ( server.hasArg(arg) ) {
      String value = server.arg(arg); 
      Debugf("%s = %s\n", arg, value.c_str());
      h = 0; m = 0;
      sscanf(value.c_str(), "%2d:%2d", &h, &m);
      Debugf("h = %d; m = %d\n", h, m);
      config.thermostat.t_prog_cool[program].h_begin = h * 60 + m;    // minutes depuis le debut du jour
    }
     sprintf(arg, "p%d_time_f", program);
    if ( server.hasArg(arg) ) {
      String value = server.arg(arg);
      // Debugf("%s = %s\n", arg, value.c_str());
      h = 0; m = 0;
      sscanf(value.c_str(), "%2d:%2d", &h, &m);
      config.thermostat.t_prog_cool[program].h_end = h * 60 + m;      // minutes depuis le debut du jour
    }
   
  }

  if ( saveConfig() ) {
    ret = 200;
    response = "OK";
  } else {
    ret = 412;
    response = "Unable to save configuration";
  }

  server.send ( ret, "text/plain", response);

}

/* ======================================================================
Function: handleFormConfig 
Purpose : handle main configuration page
Input   : -
Output  : - 
Comments: -
====================================================================== */
void handleFormConfig(void) 
{
  String response="";
  int ret ;

  LedBluON();

  // We validated config ?
  if (server.hasArg("save"))
  {
    int itemp;
    DebuglnF("===== Posted configuration"); 

    // Wifi Thermostat
    strncpy(config.ssid ,       server.arg("ssid").c_str(),       CFG_SSID_SIZE );
    strncpy(config.psk ,        server.arg("psk").c_str(),        CFG_PSK_SIZE );
    strncpy(config.host ,       server.arg("host").c_str(),       CFG_HOSTNAME_SIZE );
    strncpy(config.ntp_server,  server.arg("ntp_server").c_str(), CFG_NTP_SERVER_SIZE );
    strncpy(config.tz ,         server.arg("tz").c_str(),         CFG_TZ_SIZE );
    strncpy(config.ap_psk ,     server.arg("ap_psk").c_str(),     CFG_PSK_SIZE );
    strncpy(config.ota_auth,    server.arg("ota_auth").c_str(),   CFG_PSK_SIZE );
    itemp = server.arg("ota_port").toInt();
    config.ota_port = (itemp>=0 && itemp<=65535) ? itemp : CFG_DEFAULT_OTA_PORT ;
    strncpy(config.syslog_host ,   server.arg("syslog_host").c_str(),     CFG_SYSLOG_HOST_SIZE );
    itemp = server.arg("syslog_port").toInt();
    config.syslog_port = (itemp>=0 && itemp<=65535) ? itemp : CFG_DEFAULT_SYSLOG_PORT ;

    // Pour les checkbox, l'argument n'est transmit que s'il est coché

    // mqtt
    strncpy(config.mqtt.host,   server.arg("mqtt_host").c_str(),  CFG_MQTT_HOST_SIZE );
    strncpy(config.mqtt.user,    server.arg("mqtt_user").c_str(),   CFG_MQTT_USER_SIZE );
    strncpy(config.mqtt.pswd, server.arg("mqtt_pswd").c_str(),CFG_MQTT_PSWD_SIZE );
    strncpy(config.mqtt.topic, server.arg("mqtt_topic").c_str(),CFG_MQTT_TOPIC_SIZE );
    itemp = server.arg("mqtt_port").toInt();
    config.mqtt.port = (itemp>=0 && itemp<=65535) ? itemp : CFG_MQTT_DEFAULT_PORT ; 
    itemp = server.arg("mqtt_freq").toInt();
    if (itemp>0 && itemp<=86400){
      // Mqtt Update if needed
      Tick_mqtt.detach();
      Tick_mqtt.attach(itemp, Task_mqtt);
    } else {
      itemp = 0 ; 
    }
    config.mqtt.freq = itemp;
    
    // jeedom
    strncpy(config.jeedom.host,   server.arg("jdom_host").c_str(),  CFG_JDOM_HOST_SIZE );
    strncpy(config.jeedom.url,    server.arg("jdom_url").c_str(),   CFG_JDOM_URL_SIZE );
    strncpy(config.jeedom.apikey, server.arg("jdom_apikey").c_str(),CFG_JDOM_APIKEY_SIZE );
    itemp = server.arg("jdom_port").toInt();
    config.jeedom.port = (itemp>=0 && itemp<=65535) ? itemp : CFG_JDOM_DEFAULT_PORT ; 
    itemp = server.arg("jdom_freq").toInt();
    if (itemp>0 && itemp<=86400){
      // Jeedom Update if needed
      Tick_jeedom.detach();
      Tick_jeedom.attach(itemp, Task_jeedom);
    } else {
      itemp = 0 ; 
    }
    config.jeedom.freq = itemp;

    // ============
    // HTTP Request
    // ============
    strncpy(config.httpReq.host, server.arg("httpreq_host").c_str(), CFG_HTTPREQ_HOST_SIZE );
    strncpy(config.httpReq.path, server.arg("httpreq_path").c_str(), CFG_HTTPREQ_PATH_SIZE );
    itemp = server.arg("httpreq_port").toInt();
    config.httpReq.port = (itemp>=0 && itemp<=65535) ? itemp : CFG_HTTPREQ_DEFAULT_PORT ; 
    itemp = server.arg("httpreq_freq").toInt();
    if (itemp>0 && itemp<=86400)
    {
      Tick_httpRequest.detach();
      Tick_httpRequest.attach(itemp, Task_httpRequest);
    } else {
      itemp = 0 ; 
    }
    config.httpReq.freq = itemp;

    itemp = server.arg("httpreq_swidx").toInt();
    if (itemp > 0 && itemp <= 65535)
      config.httpReq.swidx = itemp;
    else
      config.httpReq.swidx = 0;

    // ==========
    // Thermostat
    // ==========
    float ftemp = server.arg("term_hyteresis").toFloat();

    // to be completed xxxxxx : check if positive value ?
    Debugf("ftemp = %f\n", ftemp);
    config.thermostat.hysteresis = (ftemp * 10.0) + 0.5;        // on ne fait pas de conversion !!!

    ftemp = server.arg(CFG_FORM_THER_TEMP_HORSGEL).toFloat();
    config.thermostat.t_horsgel = tempDisplayToConfig(ftemp);

    ftemp = server.arg(CFG_FORM_THER_TEMP_MANU_HEAT).toFloat();
    config.thermostat.t_manu_heat = tempDisplayToConfig(ftemp);

    ftemp = server.arg(CFG_FORM_THER_TEMP_MANU_COOL).toFloat();
    config.thermostat.t_manu_cool = tempDisplayToConfig(ftemp);

    config.thermostat.config = server.arg("term_config").toInt();

    config.thermostat.mode = server.arg("term_mode").toInt();

    itemp = server.arg(CFG_FORM_THER_UNIT).toInt();
    if (itemp)
      config.thermostat.options |= t_option_fahrenheit;
    else
      config.thermostat.options &= ~t_option_fahrenheit;
    
    if ( saveConfig() ) {
      ret = 200;
      response = "OK";
    } else {
      ret = 412;
      response = "Unable to save configuration";
    }

    showConfig();
  }
  else
  {
    ret = 400;
    response = "Missing Form Field";
  }

  DebugF("Sending response "); 
  Debug(ret); 
  Debug(":"); 
  Debugln(response); 
  server.send ( ret, "text/plain", response);
  LedBluOFF();
}

/* ======================================================================
Function: handleRoot 
Purpose : handle main page /
Input   : -
Output  : - 
Comments: -
====================================================================== */
void handleRoot(void) 
{
  LedBluON();
  handleFileRead("/");
  LedBluOFF();
}

/* ======================================================================
Function: formatNumberJSON 
Purpose : check if data value is full number and send correct JSON format
Input   : String where to add response
          char * value to check 
Output  : - 
Comments: 00150 => 150
          ADCO  => "ADCO"
          1     => 1
====================================================================== */
void formatNumberJSON( String &response, char * value)
{
  // we have at least something ?
  if (value && strlen(value))
  {
    boolean isNumber = true;
    char * p = value;

    // just to be sure
    if (strlen(p)<=16) {
      // check if value is number
      while (*p && isNumber) {
        if ( *p < '0' || *p > '9' )
          isNumber = false;
        p++;
      }

      // this will add "" on not number values
      if (!isNumber) {
        response += '\"' ;
        response += value ;
        response += F("\"") ;
      } else {
        // this will remove leading zero on numbers
        p = value;
        while (*p=='0' && *(p+1) )
          p++;
        response += p ;
      }
    } else {
      Debugln(F("formatNumberJSON error!"));
    }
  }
}


/* ======================================================================
Function: tinfoJSON 
Purpose : get thermostat info values in JSON format for browser
Input   : 
Output  : return thermostat info in json format 
Comments: -
====================================================================== */
void tinfoJSON(void)
{
String  response = "";
tm      timeinfo;
char    buf_date[32];
char    buf_jour_en[16];
char    buf_jour_fr[16];

const char    *days_name_fr[] = {
  "Dimanche",
  "Lundi",
  "Mardi",
  "Mercredi",
  "Jeudi",
  "Vendredi",
  "Samedi"
};

  if(ntp.getTime(timeinfo)){
    strftime(buf_date,sizeof(buf_date),"%d/%m/%Y %H:%M:%S",&timeinfo);
    strftime(buf_jour_en,sizeof(buf_jour_en),"%A",&timeinfo);
    sprintf(buf_jour_fr, "%s", days_name_fr[timeinfo.tm_wday]);
  } else {
    strcpy(buf_date, "?");
    strcpy(buf_jour_en, "?");
    strcpy(buf_jour_fr, "?");
  }

  // Json start
  response += FPSTR(FP_JSON_START);

  response+=FPSTR(FP_Q);
  response+=CFG_FORM_ETAT_WDAY_EN;   response+=FPSTR(FP_QCQ); response+=String(buf_jour_en);                      response+= FPSTR(FP_QCNL);
  response+=CFG_FORM_ETAT_WDAY_FR;   response+=FPSTR(FP_QCQ); response+=String(buf_jour_fr);                      response+= FPSTR(FP_QCNL);
  
  response+=CFG_FORM_ETAT_DATE;      response+=FPSTR(FP_QCQ); response+=String(buf_date);                         response+= FPSTR(FP_QCNL);

  response+=CFG_FORM_HOST;           response+=FPSTR(FP_QCQ); response+=config.host;                              response+= FPSTR(FP_QCNL);

  response+=CFG_FORM_ETAT_TEMP;      response+=FPSTR(FP_QCQ); response+=tempConfigToDisplay(t_current_temp);      response+= FPSTR(FP_QCNL);
  response+=CFG_FORM_ETAT_TARG;      response+=FPSTR(FP_QCQ); response+=tempConfigToDisplay(t_target_temp);       response+= FPSTR(FP_QCNL);

  response+=CFG_FORM_ETAT_HUM;       response+=FPSTR(FP_QCQ); response+=String(t_current_hum);                    response+= FPSTR(FP_QCNL);

  response+=CFG_FORM_ETAT_RELAY;     response+=FPSTR(FP_QCQ); response+=(t_relay_status == 1 ? "ON": "OFF" );     response+= FPSTR(FP_QCNL);

  response+=CFG_FORM_ERREUR_THER;    response+=FPSTR(FP_QCQ); response+=String(t_errors);                         response+= FPSTR(FP_QCNL);

  response+=CFG_FORM_THER_MODE;      response+=FPSTR(FP_QCQ); response+=String(config.thermostat.mode);           response+= FPSTR(FP_QCNL);
  response+=CFG_FORM_THER_CONFIG;    response+=FPSTR(FP_QCQ); response+=String(config.thermostat.config);         response+= FPSTR(FP_QCNL);

  switch(config.thermostat.config) {
    case t_config_manu :
      // Mode MANUEL : pas de prog item
      response+=CFG_FORM_ETAT_PROG_ITEM; response+=FPSTR(FP_QCQ); response+=String("   ");                        response+= FPSTR(FP_QCNL);
      break;
    case t_config_program :
      // Mode PROGRAMME : envoyer la ligne de programme active
      response+=CFG_FORM_ETAT_PROG_ITEM; response+=FPSTR(FP_QCQ); response+=String('P') +String(t_current_prog_item); response+= FPSTR(FP_QCNL);
      break;
  }

  response+=CFG_FORM_THER_UNIT;      response+=FPSTR(FP_QCQ); response+=(config.thermostat.options & t_option_fahrenheit ? "fahrenheit" : "celsius"); response+= FPSTR(FP_QCNL);

  response+="adresse_ip";            response+=FPSTR(FP_QCQ); response+=WiFi.localIP().toString();                                                    response+= FPSTR(FP_Q);

  // Json end
  response += FPSTR(FP_JSON_END);
  
  Debug(F("sending... "));Debugln(response);

  server.send ( 200, "text/json", response );

  // Debugln(F("sending 404..."));
  // server.send ( 404, "text/plain", "No data" );
  Debugln(F("OK!"));
  
}

/* ======================================================================
Function: getSysJSONData 
Purpose : Return JSON string containing system data
Input   : Response String
Output  : - 
Comments: -
====================================================================== */
void getSysJSONData(String & response)
{
  response = "";
  char buffer[32];
  int32_t adc = ( 1000 * analogRead(A0) / 1024 );

  // Json start
  response += F("[\r\n");

  response += "{\"na\":\"Uptime\",\"va\":\"";
  response += sysinfo.sys_uptime;
  response += "\"},\r\n";

  response += "{\"na\":\"Adresse IP\",\"va\":\"";
  response += WiFi.localIP().toString();
  response += "\"},\r\n";

  response += "{\"na\":\"Adresse MAC station\",\"va\":\"";
  response += WiFi.macAddress();
  response += "\"},\r\n";
  
  if (WiFi.status() == WL_CONNECTED)
  {
      response += "{\"na\":\"Wifi network\",\"va\":\"";
      response += config.ssid;
      response += "\"},\r\n";
      response += "{\"na\":\"Wifi RSSI\",\"va\":\"";
      response += WiFi.RSSI();
      response += " dB\"},\r\n";
  } else {
      response += "{\"na\":\"Wifi network\",\"va\":\"";
      response += "Not connected";
      response += "\"},\r\n";
  }

  response += "{\"na\":\"WifiTherm Version\",\"va\":\"" WIFITERM_VERSION "\"},\r\n";

  response += "{\"na\":\"Sortie relai\",\"va\":\"";

  response += "GPIO ";
  response += String(CFG_RELAY_GPIO);

  response += "\"},\r\n";

  response += "{\"na\":\"Led interne\",\"va\":\"";

  response += "GPIO ";
  response += String(BLU_LED_PIN);

  response += "\"},\r\n";

  response += "{\"na\":\"Compile le\",\"va\":\"" __DATE__ " " __TIME__ "\"},\r\n";
  
  response += "{\"na\":\"Options de compilation\",\"va\":\"";
  response += optval;
  response += "\"},\r\n";

#if defined(THER_BME280) || defined(THER_HTU21)
  response += "{\"na\":\"Sensor SDA, SCL \",\"va\":\"";
  response += "GPIO " + String(SDA) + ", GPIO " + String(SCL);
  response += "\"},\r\n";
#elif defined(THER_DS18B20)

  response += "{\"na\":\"Sensor Data \",\"va\":\"";
  
  #ifdef ESP8266
    OneWire oneWire(4);   // GPIO 4
    response += "GPIO 4";
  #else
    response += "GPIO 33";
  #endif
  
  response += "\"},\r\n";
#endif

  response += "{\"na\":\"SDK Version\",\"va\":\"";
#ifdef ESP8266
  response += system_get_sdk_version() ;
#else
  response += esp_get_idf_version() ;
#endif
  response += "\"},\r\n";

  response += "{\"na\":\"Chip ID\",\"va\":\"";
#ifdef ESP8266
  sprintf_P(buffer, "0x%0X",system_get_chip_id() );
#else
  int ChipId;
  uint64_t macAddress = ESP.getEfuseMac();
  uint64_t macAddressTrunc = macAddress << 40;
  ChipId = macAddressTrunc >> 40;
  sprintf_P(buffer, PSTR("0x%06X"), ChipId);
#endif
  response += buffer ;
  response += "\"},\r\n";

  response += "{\"na\":\"Boot Version\",\"va\":\"";
#ifdef ESP8266
  sprintf_P(buffer, "0x%0X",system_get_boot_version() );
  response += buffer ;
#else
  response += "??????????";
#endif
  response += "\"},\r\n";

  response += "{\"na\":\"Flash Real Size\",\"va\":\"";
#ifdef ESP8266
  response += formatSize(ESP.getFlashChipRealSize()) ;
#else
  response += formatSize(ESP.getFlashChipSize()) ;
#endif
  response += "\"},\r\n";

  response += "{\"na\":\"Firmware Size\",\"va\":\"";
  response += formatSize(ESP.getSketchSize()) ;
  response += "\"},\r\n";

  response += "{\"na\":\"Free Size\",\"va\":\"";
  response += formatSize(ESP.getFreeSketchSpace()) ;
  response += "\"},\r\n";

  response += "{\"na\":\"Analog A0\",\"va\":\"";
  adc = ( (1000L * (long)analogRead(A0)) / 1024L);
  sprintf_P( buffer, PSTR("%ld mV"), adc);
  response += buffer ;
  response += "\"},\r\n";

#ifdef ESP8266
  // ESP8266
  FSInfo info;
  LittleFS.info(info);
#endif

  response += "{\"na\":\""; response += "LittleFS"; response += " Total\",\"va\":\"";
#ifdef ESP8266
  response += formatSize(info.totalBytes) ;
#else
  response += formatSize(LittleFS.totalBytes()) ;
#endif
  response += "\"},\r\n";

  response += "{\"na\":\""; response += "LittleFS"; response += " Used\",\"va\":\"";
#ifdef ESP8266
  response += formatSize(info.usedBytes) ;
#else
  response += formatSize(LittleFS.usedBytes()) ;
#endif
  response += "\"},\r\n";

  response += "{\"na\":\""; response += "LittleFS"; response += " Occupation\",\"va\":\"";
#ifdef ESP8266
  sprintf_P(buffer, "%d%%",100*info.usedBytes/info.totalBytes);
#else
  sprintf_P(buffer, "%d%%",100 * LittleFS.usedBytes() / LittleFS.totalBytes());
#endif

  response += buffer ;
  response += "\"},\r\n"; 

  // Free mem
  response += "{\"na\":\"Free Ram\",\"va\":\"";
#ifdef ESP8266
  response += formatSize(system_get_free_heap_size()) ;
#else
  response += formatSize(esp_get_free_heap_size()) ;
#endif
  response += "\"},\r\n";

  // Thermostat errors
  response += "{\"na\":\"WifiThermostat errors\",\"va\":\"";
  response += get_t_errors_str();
  response += "\"}\r\n";
 
  // Json end
  response += F("]\r\n");
}

/* ======================================================================
Function: prog_heatJSONTable 
Purpose : dump all prog heat values in JSON format for browser
Input   : -
Output  : - 
Comments: -
====================================================================== */
void prog_heatJSONTable()
{
String response = "";
char elementId[32]; 
char elementValue[16];
String stringValue;

  Debugln("prog_heatJSONTable");

  // Json start
  response = FPSTR(FP_JSON_START); 


  response+="\"";

  // Default heat temp
  response+=CFG_FORM_THER_PROG_HEAT_DEFAULT; response+=FPSTR(FP_QCQ);      response+=String((float)config.thermostat.t_prog_heat_default / 10.0, 1);     response+= FPSTR(FP_QCNL);

  // Thermostat unit : celsius / fahrenheit
  response+=CFG_FORM_THER_UNIT;      response+=FPSTR(FP_QCQ); response+=(config.thermostat.options & t_option_fahrenheit ? "fahrenheit" : "celsius"); response+= FPSTR(FP_QCNL);

  // Les valeurs true / false doivent être transmises entourées de guillemets
  // sinon le $.getJSON( "/prog_heat.json" ) échoue 

  for ( int program = 0 ; program < CFG_THER_PROG_COUNT ; program++) {
    // Debugf( "program %d : %02x\n", program, config.thermostat.t_prog_heat[program].days);
    for ( int wday = 0; wday < 8 ; wday++) {
      sprintf(elementId , "p%d_wday_%d", program, wday);
      if ( config.thermostat.t_prog_heat[program].days & (1 << wday) )
        stringValue = "true";
      else
        stringValue = "false";
      response+=elementId;      response+=FPSTR(FP_QCQ);      response+=stringValue;     response+= FPSTR(FP_QCNL);
    }
        
    // target temp
    sprintf(elementId, "p%d_temperature", program);
    stringValue = tempConfigToDisplay(config.thermostat.t_prog_heat[program].t_temp);
    response+=elementId;      response+=FPSTR(FP_QCQ);      response+=stringValue;     response+= FPSTR(FP_QCNL);

    // heureminute debut
    sprintf(elementId, "p%d_time_d", program);
    sprintf(elementValue, "%02d:%02d", config.thermostat.t_prog_heat[program].h_begin / 60, config.thermostat.t_prog_heat[program].h_begin % 60);
    response+=elementId;      response+=FPSTR(FP_QCQ);      response+=elementValue;     response+= FPSTR(FP_QCNL);

    // heureminute fin
    sprintf(elementId, "p%d_time_f", program);
    sprintf(elementValue, "%02d:%02d", config.thermostat.t_prog_heat[program].h_end / 60, config.thermostat.t_prog_heat[program].h_end % 60);

    response+=elementId;      response+=FPSTR(FP_QCQ);      response+=elementValue;  
    if (program == CFG_THER_PROG_COUNT - 1) {
      response+= FPSTR(FP_Q);    // Last item
    } else {
      response+= FPSTR(FP_QCNL);
    }

  }
  // Json end
  response += FPSTR(FP_JSON_END);

  server.send ( 200, "text/json", response );
  Debugln(F("Ok!"));
}

/* ======================================================================
Function: prog_coolJSONTable 
Purpose : dump all prog heat values in JSON format for browser
Input   : -
Output  : - 
Comments: -
====================================================================== */
void prog_coolJSONTable()
{
String response = "";
char elementId[32]; 
char elementValue[16];
String stringValue;

  Debugln("prog_coolJSONTable");

  // Json start
  response = FPSTR(FP_JSON_START); 


  response+="\"";

  // Default cool temp
  response+=CFG_FORM_THER_PROG_COOL_DEFAULT; response+=FPSTR(FP_QCQ);      response+=String((float)config.thermostat.t_prog_cool_default / 10.0, 1);     response+= FPSTR(FP_QCNL);

  // Thermostat unit : celsius / fahrenheit
  response+=CFG_FORM_THER_UNIT;      response+=FPSTR(FP_QCQ); response+=(config.thermostat.options & t_option_fahrenheit ? "fahrenheit" : "celsius"); response+= FPSTR(FP_QCNL);

  // Les valeurs true / false doivent être transmises entourées de guillemets
  // sinon le $.getJSON( "/prog_cool.json" ) échoue 

  for ( int program = 0 ; program < CFG_THER_PROG_COUNT ; program++) {
    // Debugf( "program %d : %02x\n", program, config.thermostat.t_prog_cool[program].days);
    for ( int wday = 0; wday < 8 ; wday++) {
      sprintf(elementId , "p%d_wday_%d", program, wday);
      if ( config.thermostat.t_prog_cool[program].days & (1 << wday) )
        stringValue = "true";
      else
        stringValue = "false";
      response+=elementId;      response+=FPSTR(FP_QCQ);      response+=stringValue;     response+= FPSTR(FP_QCNL);
    }
        
    // target temp
    sprintf(elementId, "p%d_temperature", program);
    stringValue = tempConfigToDisplay(config.thermostat.t_prog_cool[program].t_temp);
    response+=elementId;      response+=FPSTR(FP_QCQ);      response+=stringValue;     response+= FPSTR(FP_QCNL);

    // heureminute debut
    sprintf(elementId, "p%d_time_d", program);
    sprintf(elementValue, "%02d:%02d", config.thermostat.t_prog_cool[program].h_begin / 60, config.thermostat.t_prog_cool[program].h_begin % 60);
    response+=elementId;      response+=FPSTR(FP_QCQ);      response+=elementValue;     response+= FPSTR(FP_QCNL);

    // heureminute fin
    sprintf(elementId, "p%d_time_f", program);
    sprintf(elementValue, "%02d:%02d", config.thermostat.t_prog_cool[program].h_end / 60, config.thermostat.t_prog_cool[program].h_end % 60);

    response+=elementId;      response+=FPSTR(FP_QCQ);      response+=elementValue;  
    if (program == CFG_THER_PROG_COUNT - 1) {
      response+= FPSTR(FP_Q);    // Last item
    } else {
      response+= FPSTR(FP_QCNL);
    }

  }
  // Json end
  response += FPSTR(FP_JSON_END);

  server.send ( 200, "text/json", response );
  Debugln(F("Ok!"));
}

/* ======================================================================
Function: sysJSONTable 
Purpose : dump all sysinfo values in JSON table format for browser
Input   : -
Output  : - 
Comments: -
====================================================================== */
void sysJSONTable()
{
  String response = "";
  
  getSysJSONData(response);

  // Just to debug where we are
  Debug(F("Serving /system page... "));Debugln(response);
  server.send ( 200, "text/json", response );
  Debugln(F("Ok!"));
}



/* ======================================================================
Function: getConfigJSONData 
Purpose : Return JSON string containing configuration data
Input   : Response String
Output  : - 
Comments: -
====================================================================== */
void getConfJSONData(String & r)
{
  // Json start
  r = FPSTR(FP_JSON_START); 

  r+="\"";
  r+=CFG_FORM_SSID;                   r+=FPSTR(FP_QCQ);       r+=config.ssid;                  r+= FPSTR(FP_QCNL);
  r+=CFG_FORM_PSK;                    r+=FPSTR(FP_QCQ);       r+=config.psk;                   r+= FPSTR(FP_QCNL);
  r+=CFG_FORM_HOST;                   r+=FPSTR(FP_QCQ);       r+=config.host;                  r+= FPSTR(FP_QCNL);
  r+=CFG_FORM_NTP_HOST;               r+=FPSTR(FP_QCQ);       r+=config.ntp_server;            r+= FPSTR(FP_QCNL);
  r+=CFG_FORM_TZ;                     r+=FPSTR(FP_QCQ);       r+=config.tz;                    r+= FPSTR(FP_QCNL);
  
  r+=CFG_FORM_THER_HYSTERESIS;        r+=FPSTR(FP_QCQ);       r+=String((float)config.thermostat.hysteresis / 10.0,1);           r+= FPSTR(FP_QCNL);
  r+=CFG_FORM_THER_TEMP_HORSGEL;      r+=FPSTR(FP_QCQ);       r+=tempConfigToDisplay(config.thermostat.t_horsgel);               r+= FPSTR(FP_QCNL);
  r+=CFG_FORM_THER_TEMP_MANU_HEAT;    r+=FPSTR(FP_QCQ);       r+=tempConfigToDisplay(config.thermostat.t_manu_heat);             r+= FPSTR(FP_QCNL);
  r+=CFG_FORM_THER_TEMP_MANU_COOL;    r+=FPSTR(FP_QCQ);       r+=tempConfigToDisplay(config.thermostat.t_manu_cool);             r+= FPSTR(FP_QCNL);
  r+=CFG_FORM_THER_CONFIG;            r+=FPSTR(FP_QCQ);       r+=config.thermostat.config;                                       r+= FPSTR(FP_QCNL);
  r+=CFG_FORM_THER_MODE;              r+=FPSTR(FP_QCQ);       r+=config.thermostat.mode;                                         r+= FPSTR(FP_QCNL);

  r+=CFG_FORM_THER_UNIT;              r+=FPSTR(FP_QCQ);       r+=(config.thermostat.options & t_option_fahrenheit ? "fahrenheit" : "celsius"); r+= FPSTR(FP_QCNL);

  r+=CFG_FORM_THER_UNIT_CELSIUS;      r+=FPSTR(FP_QCQ);       r+=(config.thermostat.options & t_option_fahrenheit ? "0" : "1");  r+= FPSTR(FP_QCNL); 
  r+=CFG_FORM_THER_UNIT_FAHRENHEIT;   r+=FPSTR(FP_QCQ);       r+=(config.thermostat.options & t_option_fahrenheit ? "1" : "0");  r+= FPSTR(FP_QCNL); 
 
  r+=CFG_FORM_MQTT_HOST;              r+=FPSTR(FP_QCQ);       r+=config.mqtt.host;      r+= FPSTR(FP_QCNL);
  r+=CFG_FORM_MQTT_PORT;              r+=FPSTR(FP_QCQ);       r+=config.mqtt.port;      r+= FPSTR(FP_QCNL);
  r+=CFG_FORM_MQTT_USER;              r+=FPSTR(FP_QCQ);       r+=config.mqtt.user;      r+= FPSTR(FP_QCNL);
  r+=CFG_FORM_MQTT_PSWD;              r+=FPSTR(FP_QCQ);       r+=config.mqtt.pswd;      r+= FPSTR(FP_QCNL);
  r+=CFG_FORM_MQTT_TOPIC;             r+=FPSTR(FP_QCQ);       r+=config.mqtt.topic;     r+= FPSTR(FP_QCNL);
  r+=CFG_FORM_MQTT_FREQ;              r+=FPSTR(FP_QCQ);       r+=config.mqtt.freq;      r+= FPSTR(FP_QCNL);
  
  r+=CFG_FORM_JDOM_HOST;              r+=FPSTR(FP_QCQ);       r+=config.jeedom.host;    r+= FPSTR(FP_QCNL); 
  r+=CFG_FORM_JDOM_PORT;              r+=FPSTR(FP_QCQ);       r+=config.jeedom.port;    r+= FPSTR(FP_QCNL); 
  r+=CFG_FORM_JDOM_URL;               r+=FPSTR(FP_QCQ);       r+=config.jeedom.url;     r+= FPSTR(FP_QCNL); 
  r+=CFG_FORM_JDOM_KEY;               r+=FPSTR(FP_QCQ);       r+=config.jeedom.apikey;  r+= FPSTR(FP_QCNL); 
  r+=CFG_FORM_JDOM_FREQ;              r+=FPSTR(FP_QCQ);       r+=config.jeedom.freq;    r+= FPSTR(FP_QCNL);
  
  r+=CFG_FORM_HTTPREQ_HOST;           r+=FPSTR(FP_QCQ);       r+=config.httpReq.host;   r+= FPSTR(FP_QCNL);
  r+=CFG_FORM_HTTPREQ_PORT;           r+=FPSTR(FP_QCQ);       r+=config.httpReq.port;   r+= FPSTR(FP_QCNL);
  r+=CFG_FORM_HTTPREQ_PATH;           r+=FPSTR(FP_QCQ);       r+=config.httpReq.path;   r+= FPSTR(FP_QCNL); 
  r+=CFG_FORM_HTTPREQ_FREQ;           r+=FPSTR(FP_QCQ);       r+=config.httpReq.freq;   r+= FPSTR(FP_QCNL);
  r+=CFG_FORM_HTTPREQ_SWIDX;          r+=FPSTR(FP_QCQ);       r+=config.httpReq.swidx;  r+= FPSTR(FP_QCNL);
  
  r+=CFG_FORM_AP_PSK;                 r+=FPSTR(FP_QCQ);       r+=config.ap_psk;         r+= FPSTR(FP_QCNL); 
  r+=CFG_FORM_OTA_AUTH;               r+=FPSTR(FP_QCQ);       r+=config.ota_auth;       r+= FPSTR(FP_QCNL); 
  r+=CFG_FORM_OTA_PORT;               r+=FPSTR(FP_QCQ);       r+=config.ota_port;       r+= FPSTR(FP_QCNL);
  r+=CFG_FORM_SYSLOG_HOST;            r+=FPSTR(FP_QCQ);       r+=config.syslog_host;    r+= FPSTR(FP_QCNL); 
  r+=CFG_FORM_SYSLOG_PORT;            r+=FPSTR(FP_QCQ);       r+=config.syslog_port;

  r+= F("\""); 
  // Json end
  r += FPSTR(FP_JSON_END);

}

/* ======================================================================
Function: confJSONTable 
Purpose : dump all config values in JSON table format for browser
Input   : -
Output  : - 
Comments: -
====================================================================== */
void confJSONTable()
{
  String response = "";
  getConfJSONData(response);
  // Just to debug where we are
  Debug(F("Serving /config page..."));
  server.send ( 200, "text/json", response );
  Debugln(F("Ok!"));
}

/* ======================================================================
Function: jsonlittleFSlistAllFilesInDir 
Purpose : Return JSON string containing list of LittleFS files
Input   : Response String
Output  : - 
Comments: recursive in each subfolders
====================================================================== */
void jsonlittleFSlistAllFilesInDir(String &response, String dir_path, bool &first_item)
{
#ifdef ESP8266
  // ESP8266 : LittleFS
  Dir dir = LittleFS.openDir(dir_path);

  while(dir.next()) {
    if (dir.isFile()) {
      String fileName = dir_path + dir.fileName();
      size_t fileSize = dir.fileSize();
      
      if (first_item)  
        first_item=false;
      else
        response += ",";
  
      response += F("{\"na\":\"");
      response += fileName.c_str();
      response += F("\",\"va\":\"");
      response += fileSize;
      response += F("\"}\r\n");
      
    }
    if (dir.isDirectory()) {
      jsonlittleFSlistAllFilesInDir(response, dir_path + dir.fileName() + "/", first_item);
    }
    
  }
#else
  // ESP32 : LittleFS
  File root = LittleFS.open(dir_path.c_str());

  File file;
  
  if (root) {
    while ((file = root.openNextFile())) {
      if (file.isDirectory()) {
        jsonlittleFSlistAllFilesInDir(response,String(file.path()) + "/", first_item);
      } else {
        String fileName = dir_path + file.name();
        size_t fileSize = file.size();
        
        if (first_item)  
          first_item=false;
        else
          response += ",";
    
        response += F("{\"na\":\"");
        response += fileName.c_str();
        response += F("\",\"va\":\"");
        response += fileSize;
        response += F("\"}\r\n");
      }      
    }
  }
#endif
}

/* ======================================================================
Function: getSpiffsJSONData 
Purpose : Return JSON string containing list of SPIFFS / LittleFS files
Input   : Response String
Output  : - 
Comments: -
====================================================================== */
void getSpiffsJSONData(String & response)
{
  bool first_item = true;

  // Json start
  response = FPSTR(FP_JSON_START);

  // Files Array ; Début
  response += F("\"files\":[\r\n");

  // LittleFS
  jsonlittleFSlistAllFilesInDir(response, "/", first_item);

  // File Arrau : Fin
  response += F("],\r\n");

  response += F("\"spiffs\":[\r\n{");

  // Get LittleFS File system informations
#ifdef ESP8266
  FSInfo info;
  LittleFS.info(info);
  response += F("\"Total\":");
  response += info.totalBytes ;
  response += F(", \"Used\":");
  response += info.usedBytes ;
#else
  response += F("\"Total\":");
  response += LittleFS.totalBytes() ;
  response += F(", \"Used\":");
  response += LittleFS.usedBytes() ;
#endif


  response += F(", \"ram\":");
#ifdef ESP8266
  response += system_get_free_heap_size() ;
#else
  response += esp_get_free_heap_size() ;
#endif
  response += F("}\r\n]"); 

  // Json end
  response += FPSTR(FP_JSON_END);
}

/* ======================================================================
Function: spiffsJSONTable 
Purpose : dump all spiffs / littlefs system in JSON table format for browser
Input   : -
Output  : - 
Comments: -
====================================================================== */
void spiffsJSONTable()
{
  String response = "";
  getSpiffsJSONData(response);

  // xxxxxx
  Debugln(response);
  
  server.send ( 200, "text/json", response );
}

/* ======================================================================
Function: sendJSON 
Purpose : dump all values in JSON
Input   : linked list pointer on the concerned data
          true to dump all values, false for only modified ones
Output  : - 
Comments: -
====================================================================== */
void sendJSON(void)
{
  String response = "";
  
  server.send ( 404, "text/plain", "No data" );

  // server.send ( 200, "text/json", response );
}


/* ======================================================================
Function: wifiScanJSON 
Purpose : scan Wifi Access Point and return JSON code
Input   : -
Output  : - 
Comments: -
====================================================================== */
void wifiScanJSON(void)
{
  String response = "";
  bool first = true;

  // Just to debug where we are
  Debug(F("Serving /wifiscan page..."));

  int n = WiFi.scanNetworks();

  // Json start
  response += F("[\r\n");

  for (uint8_t i = 0; i < n; ++i)
  {
    int8_t rssi = WiFi.RSSI(i);
    
    // uint8_t percent;

    // dBm to Quality
    // if(rssi<=-100)      percent = 0;
    // else if (rssi>=-50) percent = 100;
    // else                percent = 2 * (rssi + 100);

    if (first) 
      first = false;
    else
      response += F(",");

    response += F("{\"ssid\":\"");
    response += WiFi.SSID(i);
    response += F("\",\"rssi\":") ;
    response += rssi;
    response += FPSTR(FP_JSON_END);
  }

  // Json end
  response += FPSTR("]\r\n");

  Debug(F("sending..."));
  server.send ( 200, "text/json", response );
  Debugln(F("Ok!"));
}


/* ======================================================================
Function: handleFactoryReset 
Purpose : reset the module to factory settingd
Input   : -
Output  : - 
Comments: -
====================================================================== */
void handleFactoryReset(void)
{
  // Just to debug where we are
  Debug(F("Serving /factory_reset page..."));
  ResetConfig();
#ifdef ESP8266
  ESP.eraseConfig();
#else
  esp_wifi_restore();
  // wifi_config_t current_conf;
  // esp_wifi_get_config((wifi_interface_t)ESP_IF_WIFI_STA, &current_conf);
  // memset(current_conf.sta.ssid, 0, sizeof(current_conf.sta.ssid));
  // memset(current_conf.sta.password, 0, sizeof(current_conf.sta.password));
  // esp_wifi_set_config((wifi_interface_t)ESP_IF_WIFI_STA, &current_conf);
#endif
  Debug(F("sending..."));
  server.send ( 200, "text/plain", FPSTR(FP_RESTART) );
  Debugln(F("Ok!"));
  delay(1000);
  ESP.restart();
  while (true)
    delay(1);
}

/* ======================================================================
Function: handleReset 
Purpose : reset the module
Input   : -
Output  : - 
Comments: -
====================================================================== */
void handleReset(void)
{
  // Just to debug where we are
  Debug(F("Serving /reset page..."));
  Debug(F("sending..."));
  server.send ( 200, "text/plain", FPSTR(FP_RESTART) );
  Debugln(F("Ok!"));
  delay(1000);
  ESP.restart();
  while (true)
    delay(1);

}


/* ======================================================================
Function: handleNotFound 
Purpose : default WEB routing when URI is not found
Input   : -
Output  : - 
Comments: -
====================================================================== */
void handleNotFound(void) 
{
  String response = "";
  boolean found = false;  

  // Led on
  LedBluON();

  // try to return LittleFS file
  found = handleFileRead(server.uri());

  // All trys failed
  if (!found) {
    // send error message in plain text
    String message = F("File Not Found\n\n");
    message += F("URI: ");
    message += server.uri();
    message += F("\nMethod: ");
    message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
    message += F("\nArguments: ");
    message += server.args();
    message += FPSTR(FP_NL);

    for ( uint8_t i = 0; i < server.args(); i++ ) {
      message += " " + server.argName ( i ) + ": " + server.arg ( i ) + FPSTR(FP_NL);
    }

    server.send ( 404, "text/plain", message );
  }

  // Led off
  LedBluOFF();
}
