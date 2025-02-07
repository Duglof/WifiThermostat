# WifiThermostat
WfiThermostat for ESP8266 or ESP32
- Serveur Web pour la configuration et le suivi
- Données stockées dans le système de fichier LittleFS 
- Configuration du réseau Wifi en se connectant au thermostat avec un téléphone mobile
- Paramétrage de la Time zone pour avoir la bonne heure
- Gestion automatique de l'heure été/hivers via la Time Zone configurée
- Paramètrage du serveur de temps (ntp)
- 28 plages horaires au total
- Chaque plage peut être attribuée à 1 ou plusieurs jours (jusqu'à 7)
  - Une plage peut être affectée à un seul jour
  - Une plage peut être affectée du Lundi au Vendredi
  - Une plage peut être affectée au Week End
  - Pour un plage on coche les jours concernés
- Envoi possible des données à un serveur MQTT
- Envoi possible d'un requète http
- Mise à jour via OTA (Wifi) 

# Hardware

|    Module     | Module Pin | ESP8266 ESP12E/ESP12S | ESP32 30 pin  |   ESP32 C6   |
|---------------|------------|-----------------------|---------------|--------------|
| BME/BMP280    | SCL        | GPIO 5 (D1)           | GPIO 22       | GPIO 23 (D5) |
| BME/BMP280    | SDA        | GPIO 4 (D2)           | GPIO 21       | GPIO 22 (D4) |
|      or       |            |                       |               |              |
| GY-BME/BMP280 | SCL        | GPIO 5 (D1)           | GPIO 22       | GPIO 23 (D5) |
| GY-BME/BMP280 | SDA        | GPIO 4 (D2)           | GPIO 21       | GPIO 22 (D4) |
|      or       |            |                       |               |              |
| BME 680       | SCK        | GPIO 5 (D1)           | GPIO 22       | GPIO 23 (D5) |
| BME 680       | SDI        | GPIO 4 (D2)           | GPIO 21       | GPIO 22 (D4) |
|      or       |            |                       |               |              |
| DS18B20       |  DQ        | GPIO 4 (D2)           | GPIO 15       | D10          |
|               |            |                       |               |              |
| RELAY NO      |            | GPIO 12 (D6)          |               |              |

Sous réserve de tests
