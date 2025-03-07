
// **********************************************************************************
// ESP8266 / ESP32  Mqtt
// **********************************************************************************
//
// History : V1.00 2025-03-06 - First release
//           V1.01 2025-03-07
//              Add mqttClient.setKeepAlive(MQTT_KeepAlive_Timeout);
//              Add mqttClient.subscribe((String(topic) + "/set/#").c_str());
//                to receive command like topic/set/#
//                ex : Message arrived on topic CLIMATE : [CLIMATE/set/setmode], 2
// **********************************************************************************

#include "WifiTherm.h"
#include "mqtt.h"

/*
 * Constructor
 */
MQTT::MQTT() {
}

/*
 * Function: bool MQTT::Connect(_fn_mqttCallBack mqttCallback, char *host, uint16_t port, char *topic, char *user, char *pswd, bool subscribe)
 * Purpose : Connect to mqtt broker
 * Input   : _fn_mqttCallBack
 *         : host
 *         : port
 *         : topic
 *         : user
 *         : pswd
 *         : subscribe true to subscribe to receive message like topic/set/#
 *         
 * return true if connection ok else false
 */

bool MQTT::Connect(_fn_mqttCallBack mqttCallback, char *host, uint16_t port, char *topic, char *user, char *pswd, bool subscribe) {

  bool ret = false;


  DebugF("Connexion au serveur MQTT... ");

  if ( WiFi.status() == WL_CONNECTED){
    //DebugF ("execution tache MQTT / wifi connecté Init mqtt=");
    //Debugln (mtt_Init);
    if (!mtt_Init){
      // Suite au remplacement de
      // PubSubClient mqttClient(mqttWifiClient);
      // par
      // PubSubClient mqttClient;
      // il faut rajouter
      mqttClient.setClient(mqttWifiClient);

      mqttClient.setServer(host, port);       //Configuration de la connexion au serveur MQTT
      mqttClient.setCallback(mqttCallback);   //La fonction de callback qui est executée à chaque réception de message  
      mtt_Init = true; 
    }
    ret = mqttClient.connected();
    if (!ret) {
      Debugln (">>>> Connect again MQTT ...");
      mqttClient.setKeepAlive(MQTT_KeepAlive_Timeout);
      ret = mqttClient.connect(topic, user, pswd);
      if (ret && subscribe)
        mqttClient.subscribe((String(topic) + "/set/#").c_str());
    }
  }

  if (ret) {
    DebuglnF("OK");
  } else {
    DebuglnF("KO");
  }

  return(ret);
}

/*
 * bool MQTT::Publish (char *topic, char *payload , bool retained)
 */
bool MQTT::Publish (const char *topic, const char *payload , bool retained)
{
bool ret = false;

 ret = mqttClient.publish(topic, payload , retained);

 return(ret);
}

/*
 * void MQTT::Loop()
 * 
 * comment : must be called in sketch loop() every second to receive subscribe messages
 */
void MQTT::Loop()
{
  mqttClient.loop();
}
  
