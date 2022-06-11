#ifndef CONFIG_H
#define CONFIG_H

// Wifi: SSID and password
const PROGMEM char* WIFI_SSID = "[Redacted]";
const PROGMEM char* WIFI_PASSWORD = "[Redacted]";

// If defined use dhcp
// #ifdef FIXED_IP
//const IPAddress local_IP(0,0,0,0);
// const IPAddress local_IP(192, 168, 0, 53);

// If neded set your Static IP address
// Set your Gateway IP address
// const IPAddress gateway(192, 168, 0, 1);
// const IPAddress subnet(255, 255, 255, 0);
// Optional DNS
// IPAddress primaryDNS(8, 8, 8, 8);   //optional
// IPAddress secondaryDNS(8, 8, 4, 4); //optional

// MQTT: ID, server IP, port, username and password
const PROGMEM char* MQTT_CLIENT_ID = "home_sensor1_dht11";
const PROGMEM char* MQTT_SERVER_IP = "192.168.0.5";
const PROGMEM uint16_t MQTT_SERVER_PORT = 1883;
const PROGMEM char* MQTT_USER = "[Redacted]";
const PROGMEM char* MQTT_PASSWORD = "[Redacted]";

// MQTT: topics
const PROGMEM char* MQTT_SENSOR_TOPIC = "home/living/ac/sensor";
const PROGMEM char* MQTT_LOG_TOPIC = "home/living/ac/log";
const PROGMEM char* MQTT_AC_GET_TOPIC = "home/living/ac/get";

const PROGMEM char* MQTT_AC_POWER_SET_TOPIC = "home/living/ac/power/set";
const PROGMEM char* MQTT_AC_MODE_SET_TOPIC = "home/living/ac/mode/set";
const PROGMEM char* MQTT_AC_TEMP_SET_TOPIC = "home/living/ac/temp/set";
const PROGMEM char* MQTT_AC_FAN_SET_TOPIC = "home/living/ac/fan/set";
const PROGMEM char* MQTT_AC_SWING_SET_TOPIC = "home/living/ac/swing/set";


const PROGMEM char* COMM_ON = "on";
const PROGMEM char* COMM_OFF = "off";
const PROGMEM char* COMM_AUTO = "auto";
const PROGMEM char* COMM_HEAT = "heat";
const PROGMEM char* COMM_COOL = "cool";
const PROGMEM char* COMM_DRY = "dry";
const PROGMEM char* COMM_FAN_ONLY = "fan_only";

const PROGMEM char* COMM_FAN_AUTO = "auto";
const PROGMEM char* COMM_FAN_LOW = "low";
const PROGMEM char* COMM_FAN_MED = "medium";
const PROGMEM char* COMM_FAN_HIGH = "high";
const PROGMEM char* COMM_FAN_TURBO = "turbo";

// sleeping time
const uint16_t SLEEPING_TIME_IN_MSECONDS = 500; 

// number of readings to take average
const uint16_t N_AVG_SENSOR = 12;

// send data configuratio connection to mqtt
const bool LOG_MQTT_CONNECT = false;


// To adjust reads of DHT11
/*
const float temp_slope = 1.0f;
const float temp_shift = 0.0f;
const float humid_slope = 1.0f;
const float humid_shift = 0.0f;
*/
const float temp_slope = 1.029f;
const float temp_shift = -5.01f;
const float humid_slope = 0.899f;
const float humid_shift = 21.285f;

#ifdef  ARDUINO_ESP8266_GENERIC
#define D2 4
#define D3 0
#define D4 2
#define D9 3
#endif

const uint8_t DHTPIN = D4;
const uint8_t DHTTYPE = DHT11;

const uint16_t kIrLed = D2;  // ESP8266 GPIO pin to use. Recommended: 4 (D2).
//const uint16_t kIrLed = D9;

#endif
