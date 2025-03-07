
// **********************************************************************************
// ESP8266 / ESP32  Mqtt
// **********************************************************************************
//
// History : V1.00 2025-03-06 - First release
//           V1.01 2025-03-07
//              Add mqttClient.setKeepAlive(MQTT_KeepAlive_Timeout);
//              Add mqttClient.subscribe((String(topic) + "/set/#").c_str());
//        
// **********************************************************************************

#ifndef MQTT_H
#define MQTT_H

// Time in seconds a client connection is keptconnected if no messages are exchanged (server calculate time * 1.5)
#define MQTT_KeepAlive_Timeout     3600


#ifdef ESP8266
  // ESP8266
  #include <ESP8266WiFi.h>
#elif defined(ESP32)
  // ESP32
  #include <WiFi.h>
#else
  #error "ce n'est ni un ESP8266 ni un ESP32"
#endif

// PubSubClient V3.0.2 : https://github.com/hmueller01/pubsubclient3/releases/tag/v3.0.2
#include <PubSubClient.h>

typedef void (*_fn_mqttCallBack) (char* topic, byte* payload, unsigned int length);

class MQTT {
public:
    MQTT();

  bool Connect(void (*_fn_mqttCallBack)(char* topic, byte* payload, unsigned int length), char *host, uint16_t port, char *topic, char *user, char *pswd, bool subscribe);

  bool Publish (const char *topic, const char *payload , bool retained);

  // Must be called in loop() ( every second )
  void Loop();
  
private:
  WiFiClient mqttWifiClient;

//  PubSubClient mqttClient(mqttWifiClient);
  PubSubClient mqttClient;
  
  boolean mtt_Init=false;

};    

#endif // MQTT_H
