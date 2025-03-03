// **********************************************************************************
// ESP8266 / ESP32 Wifi Thermostat WEB Server
// **********************************************************************************
//
// Historique : V1.0.0 2025-02-03 - Première Version Béta
//
// ===================================================================
// WifiTherm : Connection à votre réseau Wifi
// ===================================================================
//  A faire une seule fois ou après des changements dans le répertoire data
//
// Téléverser WifiTherm et le data sur votre ESP12E
// Alimenter votre module ESP12E par le cable USB (ou autre) 
// Avec votre téléphone
//   Se connecter au réseau Wifi WifiTher-xxxxxx
//   Ouvrir votre navigateur préféré (Chrome ou autre)
//   Accéder à l'url http://192.168.4.1 (la page web de WifiTherm doit appaître)
//   Sélectionner l'onglet 'Configuration'
//   Renseigner
//     Réseau Wifi
//     Clé Wifi
//     Serveur NTP
//     Time Zone (heure, heure été /heure hiver)
//   Cliquer sur 'Enregistrer'
//   Enfin dans la partie 'Avancée' cliquer sur Redémarrer WifiThermostat
//   WifiThermostat se connectera à votre réseau Wifi
//   Si ce n'est pas le cas c'est que ne nom du réseau ou la clé sont erronés
// ===================================================================
//
//          Environment
//           Arduino IDE 1.8.18
//             Préférences : https://arduino.esp8266.com/stable/package_esp8266com_index.json
//             Folder Arduino/tools : https://github.com/esp8266/arduino-esp8266fs-plugin/releases/download/0.5.0/ESP8266FS-0.5.0.zip
//               (Arduino/tools/ESP8266FS/tool/esp8266fs.jar)
//             Folder Arduino/libraries : WifiTherm/librairie/Syslog-master.zip : uncompress
//             Type de carte : NodeMCU 1.0 (ESP-12E Module)
//             Compilation / Téléversement
//               Options de compilation dans WifiTherm.h (THER_SIMU ou THER_DS18B20 ou THER_BME280 ou THER_HTU21, DEBUG, SYSLOG)
//               Croquis/Compiler (sans erreur !!!)
//               Tools/ESP8266 Data Upload (Arduino/tools/ESP8266FS doit être installé)
//               Croquis/Téléverser
//          Hardware : NodeMCU V1.0 Kit de développement V3 CH340 NodeMCU Esp8266 Esp-12e
//            - Relay on GPIO 12 (esp8266)
//            - Ruf relay on GPIO 14 (esp8266)
// **********************************************************************************
// Global project file
#include "WifiTherm.h"

// PubSubClient V3.0.2 : https://github.com/hmueller01/pubsubclient3/releases/tag/v3.0.2
// The maximum message size, including header, is 256 bytes by default. This is configurable via MQTT_MAX_PACKET_SIZE in PubSubClient.h
// Better define : client.setBufferSize(512);
#include <PubSubClient.h> //attention mettre #define MQTT_MAX_PACKET_SIZE 512, sinon le payload data ne se raffraichit pas.
#include <ArduinoOTA.h>
#include <EEPROM.h>
#include <Ticker.h>
#include "relay.h"

#ifdef THER_SIMU

  // Nothing

#elif defined(THER_DS18B20)
  // Capteur Température DS18B20
  #include <OneWire.h>                    // Librairie OneWire V2.3.8
  #include <DallasTemperature.h>          // Librairie DallasTemperature V4.0.3


  OneWire oneWire(PIN_DQ_DS18B20);
  
  DeviceAddress DS18B20_Address;
  DallasTemperature sensors_ds18B20(&oneWire);

#elif defined(THER_BME280)

  #include <Adafruit_Sensor.h>           // Adafruit sensor
  #include <Adafruit_BME280.h>           // For BME280 support
  Adafruit_BME280 sensors_bme280;        // I2C mode

#elif defined(THER_HTU21)
  // On garde la librairie Adafruit malgrès le pb du htu.begin qui retoune toujours false
  // En cas de pb on se tournera vers https://github.com/enjoyneering/HTU2xD_SHT2x_Si70xx
  // qui est une lib testée pour ESP8266 et ESP32 
  // c'est d'ailleurs celle qui est utilisée pour Tasmota !!!
  
  #include <Wire.h>
  #include "Adafruit_HTU21DF.h"
  Adafruit_HTU21DF sensors_htu = Adafruit_HTU21DF();  // I2C mode 
#else
  #error "You must define THER_SIMU or THER_DS18B20 or THER_BME280 or THER_HTU21"
#endif

// Relay of thermostat
DigitalRelay  relay1;

#ifdef ESP8266
  ESP8266WebServer server(80);
#else
  ESP32WebServer server(80);
#endif

bool ota_blink;
String optval;    // Options de compilation


// define whole brigtness level for RGBLED (50%)
// LED Blink timers
Ticker rgb_ticker;
Ticker blu_ticker;
Ticker red_ticker;
Ticker Every_1_Sec;
Ticker Every_1_Min;
Ticker Tick_mqtt;
Ticker Tick_jeedom;
Ticker Tick_httpRequest;

volatile boolean task_1_sec = false;
volatile boolean task_1_min = false;
volatile boolean task_mqtt = false;
volatile boolean task_jeedom = false;
volatile boolean task_httpRequest = false;
volatile boolean task_updsw = false;
unsigned long seconds = 0;                // Nombre de secondes depuis la mise sous tension

// sysinfo data
_sysinfo sysinfo;

enum _t_errors {
  t_error_none = 0,
  t_error_reading_sensor = -1,
};

// Thermostat informations
int16_t   t_current_temp = 0;             // Temperature en dixième de degré : 175 = 17.5°C
int16_t   t_current_hum = 45;              // Humidité de 0 à 100%
int16_t   t_target_temp = 175;            // Temperature en dixième de degré : 175 = 17.5°C
int16_t   t_current_prog_item = -1;       // -1 = none : value [0..(CFG_THER_PROG_COUNT-1)] : 0 = 1ère ligne
int8_t    t_relay_status = 0;             // 0 = off, 1 = on
int8_t    t_errors = t_error_none;        // Erreurs : ex : t_error_reading_sensor

//Client Mqtt
WiFiClient MqttClient;
PubSubClient MQTTclient(MqttClient);
bool Mqtt_Init=0;

NTP ntp;

#ifdef SYSLOG
WiFiUDP udpClient;
Syslog syslog(udpClient, SYSLOG_PROTO_IETF);

char logbuffer[1024];
char waitbuffer[1024];

char *lines[50];
int in=-1;
int out=-1;

unsigned int pending = 0 ;
volatile boolean SYSLOGusable=false;
volatile boolean SYSLOGselected=false;
int plog=0;

void convert(const __FlashStringHelper *ifsh)
{
  PGM_P p = reinterpret_cast<PGM_P>(ifsh);
  plog=0;
  while (1) {
    unsigned char c = pgm_read_byte(p++);
    if (c == 0) {
      logbuffer[plog]=0;
      break;
    }
    logbuffer[plog]=c;
    plog++;
  }
}

/* ======================================================================
Function: process_line
Purpose : Ajoute la chaîne dans le buffer d'attente SYSLOG et l'envoi si nécessaire
Input   : char *msg
Output  : - 
Comments: Taille maximale du message envoyé limité à sizeof(waitbuffer)
Envoi le message SYSLOG si:
 - Celui-ci est terminé par CR ou LF
 - Le buffer d'attente est plein 
   ======================================================================
 */
void process_line(char *msg) {
    // Ajouter à waitbuffer et tronquer si nécessaire à la taille maximale de waitbuffer
    // Dans tous les cas strncat() met un null à la fin
    strncat(waitbuffer,msg, sizeof(waitbuffer) - strlen(waitbuffer) - 1);
    pending=strlen(waitbuffer);
    // En cas de buffer plein, forcer le dernier caractère à LF (0x0A) ce qui forcera l'envoi
    // et permettra un affichage correct sur la console socat
    if ( strlen(waitbuffer) == (sizeof(waitbuffer) - 1) && waitbuffer[pending-1] != 0x0A)
      waitbuffer[pending-1] = 0x0A;
    // Si le dernier vaut CR ou LF ou buffer plein => on envoie le message
    if( waitbuffer[pending-1] == 0x0D || waitbuffer[pending-1] == 0x0A ) {
      //Cette ligne est complete : l'envoyer !
      for(int i=0; i < pending-1; i++) {
        if(waitbuffer[i] <= 0x20)
          waitbuffer[i] = 0x20;
      }
      syslog.log(LOG_INFO,waitbuffer);
      delay(2*pending);
      memset(waitbuffer,0,sizeof(waitbuffer));
      pending=0;
      
    }
}


// Toutes les fonctions aboutissent sur la suivante :
void Myprint(char *msg) {
  
#ifdef DEBUG
  DEBUG_SERIAL.print(msg);
#endif

  if( SYSLOGusable ) {
    process_line(msg);   
  } else if ( SYSLOGselected) {
    //syslog non encore disponible
    //stocker les messages à envoyer plus tard
    in++;
    if(in >= 50) {
      //table saturée !
      in=0;
    }
    if(lines[in]) {
      //entrée occupée : l'écraser, tant pis !
      free(lines[in]);
    }
    lines[in]=(char *)malloc(strlen(msg)+2);
    memset(lines[in],0,strlen(msg+1));
    strcpy(lines[in],msg);   
  }
}

void Myprint() {
  logbuffer[0] = 0;
  Myprint(logbuffer);
}

void Myprint(const char *msg)
{
  // strcpy(logbuffer, msg);
  // Myprint(logbuffer);
  Myprint((char *)msg);
}

void Myprint(String msg) {
  // sprintf(logbuffer,"%s",msg.c_str());
  // Myprint(logbuffer);
  Myprint((char *)msg.c_str());
}

void Myprint(int i) {
  sprintf(logbuffer,"%d", i);
  Myprint(logbuffer);
}

void Myprint(unsigned int i) {
  sprintf(logbuffer,"%u", i);
  Myprint(logbuffer);
}

// void Myprintf(...)
void Myprintf(const char * format, ...)
{

  va_list args;
  va_start (args, format);
  vsnprintf (logbuffer,sizeof(logbuffer),format, args);
  Myprint(logbuffer);
  va_end (args);

}

void Myprintln() {
  sprintf(logbuffer,"\n");
  Myprint(logbuffer);
}

void Myprintln(char *msg)
{
  // sprintf(logbuffer,"%s\n",msg);
  // Myprint(logbuffer);
  strncpy(logbuffer,msg, sizeof(logbuffer) - 2);
  int length = strlen(logbuffer);
  logbuffer[length] = '\n';
  logbuffer[length+1] = 0;
  Myprint(logbuffer);
}

void Myprintln(const char *msg)
{
  // sprintf(logbuffer,"%s\n",msg);
  // Myprint(logbuffer);
  strncpy(logbuffer,msg, sizeof(logbuffer) - 2);
  int length = strlen(logbuffer);
  logbuffer[length] = '\n';
  logbuffer[length+1] = 0;
  Myprint(logbuffer);
}

void Myprintln(String msg) {
  // sprintf(logbuffer,"%s\n",msg.c_str());
  // Myprint(logbuffer);
  strncpy(logbuffer,msg.c_str(), sizeof(logbuffer) - 2);
  int length = strlen(logbuffer);
  logbuffer[length] = '\n';
  logbuffer[length+1] = 0;
  Myprint(logbuffer);
}

void Myprintln(const __FlashStringHelper *msg) {
  convert(msg);
  logbuffer[plog]=(char)'\n';
  logbuffer[plog+1]=0;
  Myprint(logbuffer);
}

void Myprintln(int i) {
  sprintf(logbuffer,"%d\n", i);
  Myprint(logbuffer);
}

void Myprintln(unsigned int i) {
  sprintf(logbuffer,"%u\n", i);
  Myprint(logbuffer);
}

void Myprintln(unsigned long i)
{
  sprintf(logbuffer,"%lu\n", i);
  Myprint(logbuffer);
}

void Myflush() {
#ifdef DEBUG
  DEBUG_SERIAL.flush();
#endif
}

#endif // SYSLOG

/* ======================================================================
Function: UpdateSysinfo 
Purpose : update sysinfo variables
Input   : true if first call
          true if needed to print on serial debug
Output  : - 
Comments: -
====================================================================== */
void UpdateSysinfo(boolean first_call, boolean show_debug)
{
  char buff[64];

  int sec = seconds;
  int min = sec / 60;
  int hr = min / 60;
  long day = hr / 24;

  sprintf_P( buff, PSTR("%ld days %02d h %02d m %02d sec"),day, hr % 24, min % 60, sec % 60);
  sysinfo.sys_uptime = buff;
}

/* ======================================================================
Function: Task_1_Sec 
Purpose : Mise à jour du nombre de seconds depuis la mise sous tension
Input   : -
Output  : Incrément variable globale seconds 
Comments: -
====================================================================== */
void Task_1_Sec()
{
  task_1_sec = true;
  seconds++;
}

/* ======================================================================
Function: Task_1_Min 
Purpose : Lecture température et mise à jour état du relais
Input   : -
Output  : 
Comments: -
====================================================================== */
void Task_1_Min()
{
  task_1_min = true;
}

/* ======================================================================
Function: Task_mqtt
Purpose : callback of mqtt ticker
Input   : 
Output  : -
Comments: Like an Interrupt, need to be short, we set flag for main loop
====================================================================== */
void Task_mqtt()
{
  task_mqtt = true;
}

/* ======================================================================
Function: Task_jeedom
Purpose : callback of jeedom ticker
Input   : 
Output  : -
Comments: Like an Interrupt, need to be short, we set flag for main loop
====================================================================== */
void Task_jeedom()
{
  task_jeedom = true;
}

/* ======================================================================
Function: Task_httpRequest
Purpose : callback of http request ticker
Input   : 
Output  : -
Comments: Like an Interrupt, need to be short, we set flag for main loop
====================================================================== */
void Task_httpRequest()
{
  task_httpRequest = true;
}

/* ======================================================================
Function: LedON
Purpose : callback called after led blink delay
Input   : led (defined in term of PIN)
Output  : - 
Comments: -
====================================================================== */
void LedON(int led)
{
  #ifdef BLU_LED_PIN
  if (led==BLU_LED_PIN)
    LedBluON();
  #endif
  #ifdef RED_LED_PIN
  if (led==RED_LED_PIN)
    LedRedON();
  #endif
}


/* ======================================================================
Function: LedOFF 
Purpose : callback called after led blink delay
Input   : led (defined in term of PIN)
Output  : - 
Comments: -
====================================================================== */
void LedOFF(int led)
{
  #ifdef BLU_LED_PIN
  if (led==BLU_LED_PIN)
    LedBluOFF();
  #endif
  #ifdef RED_LED_PIN
  if (led==RED_LED_PIN)
    LedRedOFF();
  #endif
}


/* ======================================================================
Function: ResetConfig
Purpose : Set configuration to default values
Input   : -
Output  : -
Comments: -
====================================================================== */
void ResetConfig(void) 
{
  // Start cleaning all that stuff
  memset(&config, 0, sizeof(_Config));

  // Set default Hostname
#ifdef ESP8266
  // ESP8266
  sprintf_P(config.host, PSTR("WifiTher-%06X"), ESP.getChipId());
#else
  //ESP32
  int ChipId;
  uint64_t macAddress = ESP.getEfuseMac();
  uint64_t macAddressTrunc = macAddress << 40;
  ChipId = macAddressTrunc >> 40;
  sprintf_P(config.host, PSTR("WifiTher-%06X"), ChipId);
#endif

  strcpy_P(config.ntp_server, CFG_DEFAULT_NTP_SERVER);
  strcpy_P(config.tz, CFG_DEFAULT_TZ);

  // Mqtt
  strcpy_P(config.mqtt.host, CFG_MQTT_DEFAULT_HOST);
  config.mqtt.port = CFG_MQTT_DEFAULT_PORT;
  strcpy_P(config.mqtt.pswd, CFG_MQTT_DEFAULT_PSWD);
  strcpy_P(config.mqtt.user, CFG_MQTT_DEFAULT_USER);
  strcpy_P(config.mqtt.topic, CFG_MQTT_DEFAULT_TOPIC);

  // Thermostat
  config.thermostat.hysteresis = CFG_THER_DEFAULT_HYSTERESIS;
  config.thermostat.t_manu_heat = CFG_THER_DEFAULT_TEMP_MANU_HEAT;
  config.thermostat.t_manu_cool = CFG_THER_DEFAULT_TEMP_MANU_COOL;
  config.thermostat.t_horsgel = CFG_THER_DEFAULT_TEMP_HORSGEL;

  config.thermostat.t_prog_heat_default = CFG_THER_DEFAULT_TEMP_MANU_HEAT;
  config.thermostat.t_prog_cool_default = CFG_THER_DEFAULT_TEMP_MANU_COOL;

  // Jeedom
  strcpy_P(config.jeedom.host, CFG_JDOM_DEFAULT_HOST);
  config.jeedom.port = CFG_JDOM_DEFAULT_PORT;
  strcpy_P(config.jeedom.url, CFG_JDOM_DEFAULT_URL);
  //strcpy_P(config.jeedom.plugin, CFG_JDOM_DEFAULT_PLUGIN);

  // HTTP Request
  strcpy_P(config.httpReq.host, CFG_HTTPREQ_DEFAULT_HOST);
  config.httpReq.port = CFG_HTTPREQ_DEFAULT_PORT;
  strcpy_P(config.httpReq.path, CFG_HTTPREQ_DEFAULT_PATH);

  // Avancé
  strcpy_P(config.ota_auth, PSTR(CFG_DEFAULT_OTA_AUTH));
  config.ota_port = CFG_DEFAULT_OTA_PORT ;
  config.syslog_port = CFG_DEFAULT_SYSLOG_PORT;
  
  // save back
  saveConfig();
}

/* ======================================================================
Function: WifiHandleConn
Purpose : Handle Wifi connection / reconnection and OTA updates
Input   : setup true if we're called 1st Time from setup
Output  : state of the wifi status
Comments: -
====================================================================== */
int WifiHandleConn(boolean setup = false) 
{
  int ret = WiFi.status();

  if (setup) {
    // Pourquoi ce n'était pas appelé avant V1.0.9 et précédente
    WiFi.mode(WIFI_STA);

    DebuglnF("========== WiFi.printDiag Start"); 
    WiFi.printDiag(DEBUG_SERIAL);
    DebuglnF("========== WiFi.printDiag End"); 
    Debugflush();

    // no correct SSID
    if (!*config.ssid) {
      DebugF("no Wifi SSID in config, trying to get SDK ones..."); 

      // Let's see of SDK one is okay
      if ( WiFi.SSID() == "" ) {
        DebuglnF("Not found may be blank chip!"); 
      } else {
        *config.psk = '\0';

        // Copy SDK SSID
        strcpy(config.ssid, WiFi.SSID().c_str());

        // Copy SDK password if any
        if (WiFi.psk() != "")
          strcpy(config.psk, WiFi.psk().c_str());

        DebuglnF("found one!"); 

        // save back new config
        saveConfig();
      }
    }

    // correct SSID
    if (*config.ssid) {
      uint8_t timeout ;

      DebugF("Connecting to: "); 
      Debug(config.ssid);
      Debugflush();

      // Do wa have a PSK ?
      if (*config.psk) {
        // protected network
        Debug(F(" with key '"));
        Debug(config.psk);
        Debug(F("'..."));
        Debugflush();
        WiFi.begin(config.ssid, config.psk);
      } else {
        // Open network
        Debug(F("unsecure AP"));
        Debugflush();
        WiFi.begin(config.ssid);
      }

      // From https://github.com/esp8266/Arduino/issues/6308
      // You're only waiting for 5 seconds (10x 500 msec), which may be a bit short.
      // timeout = 25; // 25 * 200 ms = 5 sec time out
      timeout = 50; // 50 * 200 ms = 10 sec time out
      // 200 ms loop
      while ( ((ret = WiFi.status()) != WL_CONNECTED) && timeout )
      {
        // Orange LED
        LedON(BLU_LED_PIN);
        delay(50);
        LedOFF(BLU_LED_PIN);
        delay(150);
        --timeout;
      }
    }


    // connected ? disable AP, client mode only
    if (ret == WL_CONNECTED)
    {
      DebuglnF("connected!");
      // WiFi.mode(WIFI_STA); Ca ne sert à rien, c'est fait au début

      DebugF("IP address   : "); Debugln(WiFi.localIP().toString());
      DebugF("MAC address  : "); Debugln(WiFi.macAddress());
 #ifdef SYSLOG
    if (*config.syslog_host) {
      SYSLOGselected=true;
      // Create a new syslog instance with LOG_KERN facility
      syslog.server(config.syslog_host, config.syslog_port);
      syslog.deviceHostname(config.host);
      syslog.appName(APP_NAME);
      syslog.defaultPriority(LOG_KERN);
      memset(waitbuffer,0,sizeof(waitbuffer));
      pending=0;
      SYSLOGusable=true;
    } else {
      SYSLOGusable=false;
      SYSLOGselected=false;
    }
#endif
   
    // not connected ? start AP
    } else {
      char ap_ssid[32];
      DebuglnF("Error!");
      Debugflush();

      // STA+AP Mode without connected to STA, autoconnect will search
      // other frequencies while trying to connect, this is causing issue
      // to AP mode, so disconnect will avoid this

      // Disable auto retry search channel
      WiFi.disconnect(); 

      // SSID = hostname
      strcpy(ap_ssid, config.host );
      DebugF("Switching to AP ");
      Debugln(ap_ssid);
      Debugflush();
      
      // protected network
      if (*config.ap_psk) {
        DebugF(" with key '");
        Debug(config.ap_psk);
        DebuglnF("'");
        WiFi.softAP(ap_ssid, config.ap_psk);
      // Open network
      } else {
        DebuglnF(" with no password");
        WiFi.softAP(ap_ssid);
      }
      WiFi.mode(WIFI_AP_STA);

      DebugF("IP address   : "); Debugln(WiFi.softAPIP().toString());
      DebugF("MAC address  : "); Debugln(WiFi.softAPmacAddress());
    }
    
    // Version 1.0.7 : Use auto reconnect Wifi
#ifdef ESP8266
    // Ne semble pas exister pour ESP32
    WiFi.setAutoConnect(true);
#endif
    WiFi.setAutoReconnect(true);
    DebuglnF("auto-reconnect armed !");
      
	  
    // Set OTA parameters
    ArduinoOTA.setPort(config.ota_port);
    ArduinoOTA.setHostname(config.host);
    ArduinoOTA.setPassword(config.ota_auth);
    ArduinoOTA.begin();

    // just in case your sketch sucks, keep update OTA Available
    // Trust me, when coding and testing it happens, this could save
    // the need to connect FTDI to reflash
    // Usefull just after 1st connexion when called from setup() before
    // launching potentially buggy main()
    for (uint8_t i=0; i<= 10; i++) {
      LedON(BLU_LED_PIN);
      delay(100);
      LedOFF(BLU_LED_PIN);
      delay(200);
      ArduinoOTA.handle();
    }

  } // if setup

  return WiFi.status();
}

/*
 * boolean mqttConnect
 * 
 * return true if connection ok else false
 */

boolean mqttConnect() {
  boolean ret = false;

    DebugF("Connexion au serveur MQTT... ");

    if ( WiFi.status() == WL_CONNECTED){
      //DebugF ("execution tache MQTT / wifi connecté Init mqtt=");
      //Debugln (Mqtt_Init);
      if (!Mqtt_Init){
        MQTTclient.setServer(config.mqtt.host, config.mqtt.port);    //Configuration de la connexion au serveur MQTT
        MQTTclient.setCallback(Mqttcallback);  //La fonction de callback qui est executée à chaque réception de message  
        Mqtt_Init=1; 
      }
      ret = MQTTclient.connected();
      if (!ret) {
        //Debugln ("demande de connexion MQTT");
        ret = MQTTclient.connect(config.mqtt.topic, config.mqtt.user, config.mqtt.pswd);
      }
    }

    if (ret) {
      DebuglnF("OK");
    } else {
      DebuglnF("KO");
    }
    return(ret);
}

/* ======================================================================
Function: mqttStartupLogs (called one at startup, if mqtt activated)
Purpose : Send logs to mqtt ()
Input   : 
Output  : true if post returned OK
Comments: Send in {mqtt.topic}/log Thermostat Version, Local IP and Datetime
====================================================================== */
boolean mqttStartupLogs()
{
String topic,payload;
boolean ret = false;

  if (*config.mqtt.host && config.mqtt.freq != 0) {
    if (*config.mqtt.topic) {
        // Publish Startup
        topic= String(config.mqtt.topic);
        topic+= "/log";
        String payload = "WifiTher Startup V";
        payload += WIFITERM_VERSION;
        // And submit all to mqtt
        Debug("mqtt ");
        Debug(topic);
        Debug( " value ");
        Debug(payload);
        ret = MQTTclient.publish(topic.c_str(), payload.c_str() , true);      
        if (ret)
          Debugln(" OK");
        else
          Debugln(" KO");

        // Publish IP
        topic= String(config.mqtt.topic);
        topic+= "/log";
        payload = WiFi.localIP().toString();
        // And submit all to mqtt
        Debug("mqtt ");
        Debug(topic);
        Debug( " value ");
        Debug(payload);
        ret &= MQTTclient.publish(topic.c_str(), payload.c_str() , true);      
        if (ret)
          Debugln(" OK");
        else
          Debugln(" KO");
      
        //Publish Date Heure
        topic= String(config.mqtt.topic);
        topic+= "/log";
        struct tm timeinfo;
        if(ntp.getTime(timeinfo)){
          char buf[20];
          strftime(buf,sizeof(buf),"%FT%H:%M:%S",&timeinfo);
          payload = buf;
          // And submit all to mqtt
          Debug("mqtt ");
          Debug(topic);
          Debug( " value ");
          Debug(payload);
          ret &= MQTTclient.publish(topic.c_str(), payload.c_str() , true);      
          if (ret)
            Debugln(" OK");
          else
            Debugln(" KO");
        }
    } else {
      Debugln("mqtt TOPIC non configuré");
    }
  }
  return(ret);  
}

/* ======================================================================
Function: mqttPost (called by main sketch on timer, if activated)
Purpose : Do a http post to mqtt
Input   : 
Output  : true if post returned OK
Comments: -
====================================================================== */
boolean mqttPost(String i_sub_topic, String i_value)
{
String topic;
boolean ret = false;

  // Some basic checking
  if (config.mqtt.freq != 0) {
    Debug("mqttPost publish ");
    if (*config.mqtt.host) {
        
      if (*config.mqtt.topic) {
        topic = String(config.mqtt.topic);
        topic += "/";
        topic += i_sub_topic;
        Debug(topic);
        Debug( " value = '");
        Debug(i_value);
        // And submit to mqtt
        ret = MQTTclient.publish(topic.c_str(), i_value.c_str() , true);             
        if (ret)
          Debugln("' OK");
        else
          Debugln("' KO");
      } else {
        Debugln("config.mqtt.topic is empty");
      }
    } else {
       Debugln("config.mqtt.host is empty");    
    } // if host freq
  } // if 
  return ret;
}

/* ======================================================================
Function: Mqttcallback
Purpose : Déclenche les actions à la réception d'un message mqtt
          D'après http://m2mio.tumblr.com/post/30048662088/a-simple-example-arduino-mqtt-m2mio
Input   : topic
          payload
          length
Output  : - 
Comments: -
====================================================================== */
void Mqttcallback(char* topic, byte* payload, unsigned int length) {


 #ifdef DEBUG 
   Debug("Mqttcallback : Message recu =>  topic: ");
   Debug(String(topic));
   Debug(" | longueur: ");
   Debugln(String(length,DEC));
 #endif

  String mytopic = (char*)topic;
  payload[length] = '\0'; // Null terminator used to terminate the char array
  String message = (char*)payload;

  Debug("Message arrived on topic: [");
  Debug(mytopic);
  Debug("], ");
  Debugln(message);

  Debugln("Mqttcallback : à compléter ...");
  
  // if (mytopic==(String)config.mqtt.topic + "/write/config") {    
    // update config.thermostat.config
  //}
  //if (mytopic==(String)config.mqtt.topic + "/write/mode") {
    // update config.thermostat.mode
  //}
  //if (mytopic==(String)config.mqtt.topic + "/write/target_heat") {
    // update config.thermostat.t_manu_heat
  //}
  //if (mytopic==(String)config.mqtt.topic + "/write/target_cool") {
    // update config.thermostat.t_manu_cool
  //}
  
}


/* ======================================================================
Function: littleFSlistAllFilesInDir
Purpose : List au files in folder and subfolders
Input   : dir_path
Output  : - 
Comments: -
====================================================================== */
void littleFSlistAllFilesInDir(String dir_path)
{
#ifdef ESP8266
  Dir dir = LittleFS.openDir(dir_path);

    while(dir.next()) {
      if (dir.isFile()) {
        String fileName = dir_path + dir.fileName();
        size_t fileSize = dir.fileSize();
        Debugf("FS File: %s, size: %d\n", fileName.c_str(), fileSize);
      }
      if (dir.isDirectory()) {
        littleFSlistAllFilesInDir(dir_path + dir.fileName() + "/");
      }
    }

#else
  File root = LittleFS.open(dir_path.c_str());

  File file;
  
  if (root) {
    while ((file = root.openNextFile())) {
      if (file.isDirectory()) {
        littleFSlistAllFilesInDir(String(file.path()) + "/");
      } else {
        String fileName = dir_path + file.name();
        size_t fileSize = file.size();
        Debugf("FS File: %s, size: %d\n", fileName.c_str(), fileSize);
      }      
    }
  }
#endif  // ESP8266

}

/* ======================================================================
Function: setup
Purpose : Setup I/O and other one time startup stuff
Input   : -
Output  : - 
Comments: -
====================================================================== */
void setup()
{
  // Set CPU speed to 160MHz
#ifdef ESP8266
  // ESP8266
  system_update_cpu_freq(160);
#else
  //ESP32
  setCpuFrequencyMhz(160);
#endif

  // Init the serial TXD0
  DEBUG_SERIAL.begin(115200);

  delay(2000);
  
  Debugln("\r\n\r\n");

#ifdef SYSLOG
  for(int i=0; i<50; i++)
    lines[i]=0;
  in=-1;
  out=-1;

  SYSLOGselected=true;  //Par défaut, au moins stocker les premiers msg debug
  SYSLOGusable=false;   //Tant que non connecté, ne pas émettre sur réseau
#endif

  optval = "";

#if THER_SIMU
  optval += "SIMU, ";
#elif defined(THER_DS18B20)
  optval += "DS18B20, ";
#elif defined(THER_BME280)
  optval += "BME280, ";
#elif defined(THER_HTU21)
  optval += "HTU21, ";
#endif

#ifdef DEBUG
  optval += "DEBUG, ";
#else
  optval += "No DEBUG, ";
#endif

#ifdef SYSLOG
  optval += "SYSLOG";
#else
  optval += "No SYSLOG";
#endif


  Debugln(F("=============="));
  Debug(F("WifiTherm V"));
  Debugln(F(WIFITERM_VERSION));
  Debugln();
  Debugflush();

  // Clear our global flags
  config.config = 0;

  // Our configuration is stored into EEPROM
  EEPROM.begin(2048);  // Taille max de la config

  DebugF("Config size=");   Debug(sizeof(_Config));
  DebugF("  (mqtt=");       Debug(sizeof(_mqtt));
  DebugF("  thermostat=");  Debug(sizeof(_thermostat));
  DebugF("  jeedom=");      Debug(sizeof(_jeedom));
  DebugF("  httpRequest="); Debug(sizeof(_httpRequest));
  Debugln(')');
  Debugflush();

  // Check File system init 
  if (!LittleFS.begin())
  {
    // Serious problem
    Debugf("%s Mount failed\n", "LittleFS");
  } else {
   
    Debugf("%s Mount succesfull\n", "LittleFS");

    // LittleFS (ESP8266 or ESP32)
    littleFSlistAllFilesInDir("/");
    
    DebuglnF("");
  }

  // Read Configuration from EEP
  if (readConfig()) {
      DebuglnF("Good CRC, not set!");
  } else {
    // Reset Configuration
    ResetConfig();

    // save back
    saveConfig();

    DebuglnF("Reset to default");
  }

  relay1.begin(PIN_RELAY_1, false);   // non inverted relay

#ifdef RED_LED_PIN
  pinMode(RED_LED_PIN, OUTPUT); 
  LedRedOFF();
#endif

#ifdef BLU_LED_PIN
  pinMode(BLU_LED_PIN, OUTPUT); 
  LedBluOFF();
#endif 

  // start Wifi connect or soft AP
  WifiHandleConn(true);

  // start mDNS
  // Start mDNS at esp8266.local address
  if (!MDNS.begin(config.host)) {             
    Debugln("Error starting mDNS");
  } else {
    Debugf("mDNS started %s.local", config.host);
  }
  
#ifdef SYSLOG
  //purge previous debug message,
  if(SYSLOGselected) {
    if(in != out && in != -1) {
        //Il y a des messages en attente d'envoi
        out++;
        while( out <= in ) {
          process_line(lines[out]);
          free(lines[out]);
          lines[out]=0;
          out++;
        }
        DebuglnF("syslog buffer empty");
    }
  } else {
    DebuglnF("syslog not activated !");
  }
#endif

  // OTA callbacks
  ArduinoOTA.onStart([]() { 
    LedON(BLU_LED_PIN);
    DebuglnF("Update Started");
    ota_blink = true;
  });

  ArduinoOTA.onEnd([]() { 
    DebuglnF("Update finished restarting");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    if (ota_blink) {
      LedON(BLU_LED_PIN);
    } else {
      LedOFF(BLU_LED_PIN);
    }
    ota_blink = !ota_blink;
    //Serial.printf("Progress: %u%%\n", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    LedON(BLU_LED_PIN);
    Debugf("Update Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) DebuglnF("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) DebuglnF("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) DebuglnF("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) DebuglnF("Receive Failed");
    else if (error == OTA_END_ERROR) DebuglnF("End Failed");
    ESP.restart(); 
  });

  // Update sysinfo variable and print them
  UpdateSysinfo(true, true);

  server.on("/", handleRoot);
  server.on("/config_form.json", handleFormConfig);
  server.on("/prog_heat_form.json", handleFormProgHeat);
  server.on("/json", sendJSON);
  server.on("/tinfo.json", tinfoJSON);
  server.on("/prog_heat.json", prog_heatJSONTable);
  server.on("/system.json", sysJSONTable);
  server.on("/config.json", confJSONTable);
  server.on("/spiffs.json", spiffsJSONTable);
  server.on("/wifiscan.json", wifiScanJSON);
  server.on("/factory_reset", handleFactoryReset);
  server.on("/reset", handleReset);

  // handler for the hearbeat
  server.on("/hb.htm", HTTP_GET, [&](){
      server.sendHeader("Connection", "close");
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(200, "text/html", R"(OK)");
  });

  // handler for the /update form POST (once file upload finishes)
  server.on("/update", HTTP_POST, 
    // handler once file upload finishes
    [&]() {
      server.sendHeader("Connection", "close");
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(200, "text/plain", (Update.hasError())?"FAIL":"OK");
      ESP.restart();
    },
    // handler for upload, get's the sketch bytes, 
    // and writes them through the Update object
    [&]() {
      HTTPUpload& upload = server.upload();

      if(upload.status == UPLOAD_FILE_START) {
        uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
#ifdef ESP8266
        WiFiUDP::stopAll();
#else
        // xxxxxxxx ESP32 : je ne sais pas
      #pragma message "xxxxxxx WiFiUDP::stopAll() n'existe pas pour ESP32 : que mettre ?"
#endif
        Debugf("Update: %s\n", upload.filename.c_str());
        LedON(BLU_LED_PIN);
        ota_blink = true;

        //start with max available size
        if(!Update.begin(maxSketchSpace)) 
          Update.printError(DEBUG_SERIAL);

      } else if(upload.status == UPLOAD_FILE_WRITE) {
        if (ota_blink) {
          LedON(BLU_LED_PIN);
        } else {
          LedOFF(BLU_LED_PIN);
        }
        ota_blink = !ota_blink;
        Debug(".");
        if(Update.write(upload.buf, upload.currentSize) != upload.currentSize) 
          Update.printError(DEBUG_SERIAL);

      } else if(upload.status == UPLOAD_FILE_END) {
        //true to set the size to the current progress
        if(Update.end(true)) 
          Debugf("Update Success: %u\nRebooting...\n", upload.totalSize);
        else 
          Update.printError(DEBUG_SERIAL);


      } else if(upload.status == UPLOAD_FILE_ABORTED) {
        Update.end();
        DebuglnF("Update was aborted");
      }
      delay(0);
    }
  );

  // All other not known 
  server.onNotFound(handleNotFound);
  
  // serves all SPIFFS / LittleFS Web file with 24hr max-age control
  // to avoid multiple requests to ESP
  server.serveStatic("/font", LittleFS, "/font","max-age=86400"); 
  server.serveStatic("/js",   LittleFS, "/js"  ,"max-age=86400"); 
  server.serveStatic("/css",  LittleFS, "/css" ,"max-age=86400"); 
  server.begin();

  // Display configuration
  showConfig();

  Debugln(F("HTTP server started"));
 
  // Light off
  LedOFF(BLU_LED_PIN);

  // Update sysinfo every second
  Every_1_Sec.attach(1, Task_1_Sec);

  Every_1_Min.attach(60, Task_1_Min);

  // Init and get the time
  // configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  // configTzTime(config.tz, ntpServer);
  ntp.begin(config.tz,config.ntp_server);

  struct tm timeinfo;
  if(!ntp.getTime(timeinfo)){
    Debugln("Failed to obtain time");
  } else {
    char buf[20];
    strftime(buf,sizeof(buf),"%FT%H:%M:%S",&timeinfo);
    Debugln(buf);
  }
     
  // Mqtt Update if needed
  if (config.mqtt.freq) {
    Tick_mqtt.attach(config.mqtt.freq, Task_mqtt);
    mqttConnect();
    mqttStartupLogs();  //send startup logs to mqtt
  }
  
  // Jeedom Update if needed
  if (config.jeedom.freq) 
    Tick_jeedom.attach(config.jeedom.freq, Task_jeedom);

  // HTTP Request Update if needed
  if (config.httpReq.freq) 
    Tick_httpRequest.attach(config.httpReq.freq, Task_httpRequest);

  //=====================
  // Temperature sensors
  //=====================

#ifdef THER_DS18B20
  // DS18B20
  sensors_ds18B20.begin();


  if (!sensors_ds18B20.getAddress(DS18B20_Address, 0)) { 
    Debugln("Unable to find address for DS28B20 0");
    t_errors = t_error_reading_sensor;
    t_current_temp = -1270;         // -127°C
    t_current_hum = -1;
  } else {
    t_errors = t_error_none;
    t_current_temp = sensors_ds18B20.getTempC(DS18B20_Address) * 10;
    t_current_hum = -1;       // Pas d'humidité pour un DS18B20
  }
#endif

#ifdef THER_BME280
  // ESP8266 / Wemos D1 Mini : GPIO4 et GPIO5)
  // ESP32 
  Wire.begin(SDA, SCL);                                // Default SDA and SCL pins
  Debugf("SDA = GPIO %d, SCL = GPIO %d\n", SDA,SCL);
   
  Wire.setClock(100000);                             // Slow down the I2C bus for some BME280 devices

  bool status = sensors_bme280.begin(bme280_SensorAddress);

  if (!status) {
    Debugln("Could not find a valid BME280 sensor, check wiring, address and sensor ID!");
    Debugf("SensorIS id 0x%02x\n",bme280_SensorAddress);

    t_errors = t_error_reading_sensor;
    t_current_temp = -1270;         // -127°C
    t_current_hum = -1;
  } else {
    t_errors = t_error_none;
    t_current_temp = sensors_bme280.readTemperature() * 10;
    t_current_hum = sensors_bme280.readHumidity();
  }
#endif

#ifdef THER_HTU21
  Wire.begin(SDA, SCL);                                // Default SDA and SCL pins
  Debugf("SDA = GPIO %d, SCL = GPIO %d\n", SDA,SCL);

  Wire.setClock(100000);                             // Slow down the I2C bus comme pour BME280 devices

  // Quoi qu'on fasse, sensors_htu.begin() retoourne toujours false
  // Donc, on ignore le code retour
  // Par contre la lecture de temperature retourne bien NAN s'il n'y a pas de capteur ou s'il est en defaut
  // Alors, on ce sert de ça pour tester la présence du capteur

  // sensors_htu.begin() sans tester le code retour
  sensors_htu.begin();

  float intempC = sensors_htu.readTemperature();
 
  if (isnan(intempC )) {
    Debugln("Check circuit. HTU21D not found!");
    t_errors = t_error_reading_sensor;
    t_current_temp = -1270;         // -127°C
    t_current_hum = -1;
  } else {
    t_errors = t_error_none;
    t_current_temp = sensors_htu.readTemperature() * 10;
    t_current_hum = sensors_htu.readHumidity();
  }
#endif

}

/*
 * bool get_relais_status(int16_t i_temp, int16_t i_target_temp, int16_t i_hysteresis)
 * 
 * i_temp         : current temperature
 * i_target_temp  : current target temp
 * i_hysteresis   : hysteresis
 * 
 * return : false : relay off
 *          true  : relay on
 */
bool get_relais_status(int16_t i_temp, int16_t i_target_temp, int16_t i_hysteresis)
{
bool relay_status = false;

  switch (config.thermostat.mode) {
    case t_mode_off :
      relay_status = false;
      break;
    case t_mode_horsgel :
    case t_mode_heat :
      if (i_temp < i_target_temp - i_hysteresis)
        relay_status = true;
      if (i_temp > i_target_temp + i_hysteresis)
        relay_status = false;
      break;
    case t_mode_cool :
      if (i_temp < i_target_temp - i_hysteresis)
        relay_status = false;
      if (i_temp > i_target_temp + i_hysteresis)
        relay_status = true;
      break;
  }
  return(relay_status);   
}


/*
 * int get_active_prog_item(_program  t_prog[])
 * 
 * return prog_item (index in t_prog_heat [0..(CFG_THER_PROG_COUNT - 1)] )
 */
int get_active_prog_item(_program  t_prog[])
{
// Par defaut : pas de ligne de programme sélectionnée
int item = -1;
tm  timeinfo;

  if(!ntp.getTime(timeinfo)){
    Debugln("Unable to get ntp.getTime()");
    return(item);
  }

  // Calculate day flag
  int8_t day = (1 << timeinfo.tm_wday);

  // Calculate nb minuts from beginning of the day
  int16_t hhmm = timeinfo.tm_hour * 60 + timeinfo.tm_min; 

  // Debugf("day = 0x%02x, hhmm = %d\n", day, hhmm);

  for ( int i = 0; i < CFG_THER_PROG_COUNT; i++) {
    // Test days
    // Test h_begin, h_end
    // Debugf("days = 0x%02x h_begin %d h_end %d\n", t_prog[i].days, t_prog[i].h_begin, t_prog[i].h_end);

    // ==========================================
    // t_prog[i].days == 0 -> inactive item
    // ==========================================
    if ((t_prog[i].days & day) && (hhmm >= t_prog[i].h_begin) && (hhmm <= t_prog[i].h_end) ) {
      item = i;
      break;
    }
  }
  Debugf("get_active_prog_item : item = %d\n", item);
  return(item);
}

/*
 * int16_t get_target_temp_from_config()
 * 
 * return : target temp from thermostat config
 * 
 */
int16_t get_target_temp_from_config(int & o_prog_item)
{
// Par defaut la target du mode manual
int16_t  new_target_temp = config.thermostat.t_manu_heat;

  o_prog_item = -1;         // indéfini par defaut

  // set default target 
  if ( config.thermostat.mode == t_mode_cool)
    new_target_temp = config.thermostat.t_manu_cool;
  
  switch(config.thermostat.mode) {
    case t_mode_off :
      break;
    case t_mode_horsgel :
      new_target_temp = config.thermostat.t_horsgel;
      break;
    case t_mode_heat :
      switch (config.thermostat.config) {
        case t_config_manu :
          new_target_temp = config.thermostat.t_manu_heat;
          break;
        case t_config_program :
          new_target_temp = config.thermostat.t_prog_heat_default;          // default heat temp
          // get target temp from t_prog_heat
          int item = get_active_prog_item(config.thermostat.t_prog_heat);
          // return current prog_item
          o_prog_item = item;
          // return target temp
          if (item >= 0 && item < CFG_THER_PROG_COUNT)
            new_target_temp = config.thermostat.t_prog_heat[item].t_temp;
          break; 
      }
      break;
    case t_mode_cool :
      switch (config.thermostat.config) {
        case t_config_manu :
          new_target_temp = config.thermostat.t_manu_cool;
          break;
        case t_config_program :
          new_target_temp = config.thermostat.t_prog_cool_default;          // default cool temp
          // get target temp from t_prog_clim
          int item = get_active_prog_item(config.thermostat.t_prog_cool);
          // return current prog_item
          o_prog_item = item;
          // return target temp
          if (item >= 0 && item < CFG_THER_PROG_COUNT)
            new_target_temp = config.thermostat.t_prog_cool[item].t_temp;  
          break;
      }
      break;
  }

  Debugf("get_target_temp_from_config : new_target_temp = %d, prog_item = %d\n", new_target_temp, o_prog_item);
  
  return(new_target_temp);
}
/*
 * bool get_temp_hum_from_sensor( &o_temp, &o_hum)
 * o_temp : temperture get from sensor (en dixième de degré (°C ou °F)
 * o_hum  : humidity get from sensor ( 0-100%)
 * 
 * return : true  : ok
 *          false : defaut du capteur de temperature
 */
bool get_temp_hum_from_sensor(int16_t & o_temp, int16_t & o_hum)
{
bool ret = true;   

  Debugln("get_temp_hum_from_sensor ...");

#ifdef THER_SIMU
  // ===================================================
  // SIMULATION
  //====================================================
  float l_temperature = 20.2 + random(-15, 15) / 10.0; // Generate a random temperature value between 18.7° and 21.7°
  float l_humidity    = random(45, 55);                // Generate a random humidity value between 45% and 55%

  o_temp = l_temperature * 10;              // Temperature en dixième de degré : 175 = 17.5°C
  o_hum = l_humidity;                       // Humidité de 0 à 100%

#elif defined(THER_DS18B20)
  // ===================================================
  // Get temp from sensor DS18B20
  // ===================================================
  float intempC = sensors_ds18B20.getTempC(DS18B20_Address);
  Debugf( "DS16B20 temp = %f °C\n", intempC);
  o_temp = intempC * 10;
  if ( intempC == -127 ) {
    // La lecture du capteur DS18B20 à echoué
    ret = false;
  }
  o_hum = -1;    // DS18B20 n'a pas de capteur d'humidité
#elif defined(THER_BME280)
  // ===================================================
  // Capteur BME280
  // ===================================================
  // Le capteur est en defaut à l'initialisation dans le setup()
  // il faut le re-initialiser
  if (t_errors == t_error_reading_sensor) {
    // Try again init
    bool status = sensors_bme280.begin(bme280_SensorAddress);
    if (!status) {
      Debugln("Could not find a valid BME280 sensor, check wiring, address and sensor ID!");

      ret = false;
      o_temp = -1270;         // -127°C
      o_hum = -1;
    }
  }

  if ( status ) {
    // Get temp / hum for bmp280
    float intempC = sensors_bme280.readTemperature(); 
    Debugf( "BME280 temp = %f °C\n", intempC);

    // La valeur retournée est 0.0 si capteur absent alors ...
    // if ( isnan(intempC )) {   // Ne fonctionne pas si capteur absent
    // if ( intempC == NAN) {    // Ne fonctionne pas si capteur absent
    // On supposera que l'on a jamais un valeur 0.0 exactement ...
    if ( intempC == 0.0) {       // Le valeur retournée est 0.0 si capteur absent alors ...
      // Erreur de lecture du capteur
      o_temp = -1270;     // - 127°C  compatibilité avec DS18B20 
      ret = false;
    } else {
      o_temp =  intempC * 10; 
    }
    
    float inHumidity = sensors_bme280.readHumidity();
    Debugf( "BME280 hum = %f °C\n", inHumidity);
  
    // Même test pour l'humidité ...
    if ( inHumidity == 0.0 ) {
      o_hum = -1;
      ret = false;
    } else {
      o_hum = inHumidity;
    }
  }

#elif defined(THER_HTU21)
  // ============================================
  // Capteur HTU21
  // ============================================
  float intempC = sensors_htu.readTemperature();
  Debugf( "HTU21 temp = %f °C\n", intempC);

  if ( isnan(intempC )) {       // isnan fonctionne avec un capteur HTU21
    ret = false;
    o_temp = -1270;     // - 127°C  compatibilité avec DS18B20 
  } else {
    o_temp =  intempC * 10;     
  }

  float inHumidity = sensors_htu.readHumidity();
  Debugf( "HTU21 hum = %f °C\n", inHumidity);

  if ( isnan(inHumidity )) {       // isnan fonctionne avec un capteur HTU21
    ret = false;
    o_temp = -1270;     // - 127°C  compatibilité avec DS18B20    
  } else {
    o_hum = inHumidity;
  }

#else
  #error "You must define THER_SIMU or THER_DS18B20 or THER_BME280 or THER_HTU21"
#endif
  return(ret);  
}

/* ======================================================================
Function: loop
Purpose : infinite loop main code
Input   : -
Output  : - 
Comments: -
====================================================================== */
void loop()
{
bool     ret;
int16_t  new_temp;
int16_t  new_hum;
int16_t  new_target;

  // Do all related network stuff
  server.handleClient();
  ArduinoOTA.handle();

  // Only once task per loop, let system do it own task
  if (task_1_sec) { 
    UpdateSysinfo(false, false); 
    task_1_sec = false; 

  } else if (task_1_min) {
    // =====================================
    // Gestion du thermostat
    // =====================================

    // ========== read sensor ==============
    ret = get_temp_hum_from_sensor(new_temp, new_hum);
    // to be modified xxxxxxxxxx
    if ( ret == false)
      t_errors = t_error_reading_sensor;
    t_current_temp = new_temp;
    t_current_hum = new_hum;

    // ======= get target from config ======
    int new_prog_item = -1;
    new_target = get_target_temp_from_config(new_prog_item);
    if ( (new_target != t_target_temp) || (new_prog_item != t_current_prog_item)) {
      Debugln("######### target temp or num prog have changed");
      t_target_temp = new_target;
      t_current_prog_item = new_prog_item;
      mqttConnect();
      mqttPost(MQTT_THERMOSTAT_TARGET,tempConfigToDisplay(t_target_temp)); 
      mqttPost(MQTT_THERMOSTAT_PROGNUM,String(t_current_prog_item)); 
    }

    // ====== update relay status ==========
    bool relay_status = get_relais_status(new_temp,new_target, config.thermostat.hysteresis);

    t_relay_status = relay_status;

    // Update relay
    if (t_relay_status)
      relay1.On();
    else
      relay1.Off();
    
    task_1_min = false;
  } else if (task_mqtt) { 
    // -------------------------------
    // Envoi péroidique temp & hum par mqttPost()
    // -------------------------------
    //gestion connexion Mqtt
    mqttConnect();
    // Mqtt Publier les données; 
    mqttPost(MQTT_TEMPERATURE_TOPIC,tempConfigToDisplay(t_current_temp)); 
    mqttPost(MQTT_HUMIDITY_TOPIC,String(t_current_hum)); 
    
    task_mqtt=false; 
  } else if (task_jeedom) { 
    jeedomPost();  
    task_jeedom=false;
  } else if (task_httpRequest) { 
    httpRequest();  
    task_httpRequest=false;
  } 

  //delay(10);
}
