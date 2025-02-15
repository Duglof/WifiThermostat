# WifiThermostat
WifiThermostat for ESP8266 or ESP32
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

|    Module     | Module Pin |  ESP82666 <br> ESP12E/ESP12F | ESP8266 <br> WEMOS D1 Mini| ESP32 <br> ESP32 30 pin |  ESP32 <br> ESP32-C6 Mini |
|---------------|------------|--------------|---------------|---------------|---------------|
| HI-LINK       | -Vo        | G            | G            | GND           |  GND          |
| HI-LINK       | +Vo        | VIN          | 5V           | VIN           |  +5V          |
|       -       |            |              |              |               |               |
| BME/BMP280    | SCL        | GPIO 5 (D1)  | GPIO 5 (D1)  | GPIO 22       | GPIO 23 (D5)  |
| BME/BMP280    | SDA        | GPIO 4 (D2)  | GPIO 4 (D2)  | GPIO 21       | GPIO 22 (D4)  |
|      or       |            |              |              |               |               |
| GY-BME/BMP280 | SCL        | GPIO 5 (D1)  | GPIO 5 (D1)  | GPIO 22       | GPIO 23 (D5)  |
| GY-BME/BMP280 | SDA        | GPIO 4 (D2)  | GPIO 4 (D2)  | GPIO 21       | GPIO 22 (D4)  |
|      or       |            |              |              |               |               |
| BME 680       | SCK        | GPIO 5 (D1)  | GPIO 5 (D1)  | GPIO 22       | GPIO 23 (D5)  |
| BME 680       | SDI        | GPIO 4 (D2)  | GPIO 4 (D2)  | GPIO 21       | GPIO 22 (D4)  |
|      or       |            |              |              |               |               |
| DS18B20       |  DQ        | GPIO 4 (D2)  | GPIO 4 (D2)  | GPIO 15       | D10           |
|               |            |              |              |               |               |
| RELAY (NO)    |  IN1       | GPIO 12 (D6) | GPIO 12 (D6) | A5 (GPIO 33)  | idem ?        |

- HI-LINK : HLK-PM01 : INPUT 100-240VAC 50-60Hz : OUTPUT 5VDC 3W
- BME/BMP280 BME 680 : (GND -> GND ; VCC -> 3.3V) 
- RELAY   : 5V Relay Module (DC- -> -Vo ; DC+ -> +Vo)
  
Sous réserve de tests

## ESP8266 12E/12F

[esp8266 12e 12f](docs/esp12e-gpio.png)

## ESP8266 WEMOS D1 Mini

[Wemos D1 Mini](docs/WEMOS-D1-Mini.jpg)

## ESP32 30 Pins

[esp32 gpio](docs/ESP32-dev-kit-30pins-pinout.png)

## ESP32-C6 

[esp32-C6](docs/ESP-C6-Mini.png)

# Software
|       Nom     |         Module            | Version  |  Lien                                                                                           |   Commentaire   | Voir | 
|---------------|---------------------------|----------|-------------------------------------------------------------------------------------------------|-----------------|------|
| Arduino IDE   | Desktop                   | V1.8.18  |                                                                                                 | Pas version 2.x | (1)  |
| Ardiuno IDE   | Fichiers/Préférences      |          | http://arduino.esp8266.com/stable/package_esp8266com_index.json                                 | Pour ESP8266    | (2)  |
| Ardiuno IDE   | Fichiers/Préférences      |          | https://espressif.github.io/arduino-esp32/package_esp32_index.json                              | Pour ESP32      | (2)  |
| Arduino IDE   | Dossier Arduino/tools     | V0.5.0   | https://github.com/esp8266/arduino-esp8266fs-plugin/releases/download/0.5.0/ESP8266FS-0.5.0.zip | Pour ESP8266    | (3)  |
| Arduino IDE   | Dossier Arduino/tools     | V2.0.7   | https://github.com/lorol/arduino-esp32fs-plugin/releases/download/2.0.7/esp32fs.zip             | Pour ESP32      | (4)  |
| Arduino IDE   | Dossier Arduino/libraries |          | https://github.com/Pedroalbuquerque/ESP32WebServer                                              | Pour ESP32      | (5)  |
| Arduino IDE   | Dossier Arduino/libraries |          |                      Syslog-master.zip

- (1) ESP8266 et ESP32 Skech Data Upload ne fonctionnent pas sur version 2.x
- (2) Ajouter les deux même si vous ne l'utilisez pas
- (2) Séparer les lien pas une virgule
- (2) Ne pas oublier d'aller dans le gstionnaire de cartes et d'installer : esp8266 by ESP8266 community v3.1.2
- (2) Ne pas oublier d'aller dans le gstionnaire de cartes et d'installer : esp32 by Espressif Systems v3.1.1
- (2) Après avoir relancé Arduino IDE vous devriez avoir dans le menu Outils/Type de carte 'ESP8266 Boards (3.1.2)' et 'ESP32 Arduino'
- (3) A décompresser dans le répertoire du disque Arduino/tools qui est situé à côté du répertoire Arduino/libraries (Créer le répertoire tools s'il n'existe pas)
- (3) Après avoir relancé Arduino IDE vous devriez avoir dans le menu Outils 'ESP8266 Sketch Data Upload'
- (4) A décompresser dans le répertoire du disque Arduino/tools qui est situé à côté du répertoire Arduino/libraries (Créer le répertoire tools s'il n'existe pas)
- (4) Après avoir relancé Arduino IDE vous devriez avoir dans le menu Outils vous devez avoir 'ESP32 Sketch Data Upload'
- (5) A décompresser dans le répertoire du disque Arduino/libraries
