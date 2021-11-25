/*
 *   MQTT Sensor - Temperature and Humidity (DHT22) for Home-Assistant - NodeMCU (ESP8266)
 *   https://home-assistant.io/components/sensor.mqtt/
 * 
 *   Libraries :
 *    - ESP8266 core for Arduino : https://github.com/esp8266/Arduino
 *    - PubSubClient : https://github.com/knolleary/pubsubclient
 *    - DHT : https://github.com/adafruit/DHT-sensor-library
 *    - ArduinoJson : https://github.com/bblanchon/ArduinoJson
 * 
 *   Sources :
 *    - File > Examples > ES8266WiFi > WiFiClient
 *    - File > Examples > PubSubClient > mqtt_auth
 *    - File > Examples > PubSubClient > mqtt_esp8266
 *    - File > Examples > DHT sensor library > DHTtester
 *    - File > Examples > ArduinoJson > JsonGeneratorExample
 *    - http://www.jerome-bernard.com/blog/2015/10/04/wifi-temperature-sensor-with-nodemcu-esp8266/
 * 
 *   Schematic :
 *    - https://github.com/mertenats/open-home-automation/blob/master/ha_mqtt_sensor_dht22/Schematic.png
 *    - DHT22 leg 1 - VCC
 *    - DHT22 leg 2 - D1/GPIO5 - Resistor 4.7K Ohms - GND
 *    - DHT22 leg 4 - GND
 *    - D0/GPIO16 - RST (wake-up purpose)
 * 
 *   Configuration (HA) :
sensor 1:
    platform: mqtt
    name: "Temperature ESP8266 1"
    state_topic: "home/esp8266_1/sensor"
    unit_of_measurement: "°C"
    device_class: "temperature"
    value_template: "{{ value_json.temperature }}"

sensor 2:
    platform: mqtt
    name: "Humidity ESP8266 1"
    state_topic: "home/esp8266_1/sensor"
    unit_of_measurement: "%"
    device_class: "humidity"
    value_template: "{{ value_json.humidity }}"

sensor 3:
    platform: mqtt
    name: "Logger ESP8266 1"
    state_topic: "home/esp8266_1/log"#    
    value_template: "{{ value_json.log }}"

Damian Barsotti - v1.0 02.2021
Based on:
 *   Samuel M. - v1.1 - 08.2016
 *   https://github.com/mertenats/open-home-automation
 * 
 */

#include <ESP8266WiFi.h>

#define MQTT_VERSION MQTT_VERSION_3_1_1
#include <PubSubClient.h>

#include <ArduinoJson.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ir_Samsung.h>
#include <DHT.h>

#include "config_local.h" // File for testing outside git
#include "config.h"


DHT dht(DHTPIN, DHTTYPE);
WiFiClient wifiClient;
PubSubClient client(wifiClient);
IRSamsungAc ac(kIrLed);     // Set the GPIO used for sending messages.

void publish(DynamicJsonDocument root, const char* topic){
    
    String data;
    
    serializeJson(root, data);
    
    Serial.print("Publish in topic "); Serial.println(topic);
    
    int res = client.publish(topic, data.c_str(), true);
    
    serializeJson(root, Serial);
    Serial.println();

    // if (res != 1)
    //     Serial.println(String("Publish result: ") + String(res));  
}

String header_log(char* level, int n_log){
    return String(level) + " " + String(n_log) + ": ";
}

String header_log_info(int n_log){
    return header_log("INFO", n_log);
}

String header_log_warn(int n_log){
    return header_log("WARN", n_log);
}

// function to log throught Serial and mqtt topic
bool logger(String msg){

    Serial.println(msg);
    if (client.connected()) {
        DynamicJsonDocument root(msg.length()+17);
        root["log"] = msg;
        publish(root, MQTT_LOG_TOPIC);
        return true;
    } else {
        Serial.print("ERROR: failed MQTT connection in logger, rc=");
        Serial.println(client.state());
        return false;
    }
}

int n_log_info = 0;
int n_log_warn = 0;

void logger_info(String msg){
    if (logger(header_log_info(n_log_info) + msg))
        n_log_info++;
}

void logger_warn(String msg){
    if (logger(header_log_warn(n_log_warn) + msg))
        n_log_warn++;
}

// function called to publish the temperature and the humidity
void publish_data_sensor(float p_temperature, float p_humidity) {
    DynamicJsonDocument root(200);
    root["temperature"] = (String)p_temperature;
    root["humidity"] = (String)p_humidity;
    
    publish(root, MQTT_SENSOR_TOPIC);
}

// function called to publish the AC state
void publish_ac_state() {
    DynamicJsonDocument root(200);

    root["temperature"] = String(ac.getTemp());
    switch (ac.getFan())
    {
    case kSamsungAcFanAuto:
        root["fan"] = String("auto");
        break;
    
    case kSamsungAcFanLow:
        root["fan"] = String("low");
        break;
    
    case kSamsungAcFanMed:
        root["fan"] = String("medium");
        break;
    
    case kSamsungAcFanHigh:
        root["fan"] = String("high");
        break;
    
    case kSamsungAcFanTurbo:
        root["fan"] = String("turbo");
        break;
    
    default:
        break;
    }
    
    if (!ac.getPower())
        root["mode"] = String("off");
    else
        switch (ac.getMode())
        {
        case kSamsungAcAuto:
            root["mode"] = String("auto");
            break;

        case kSamsungAcCool:
            root["mode"] = String("cool");
            break;

        case kSamsungAcDry:
            root["mode"] = String("dry");
            break;

        case kSamsungAcFan:
            root["mode"] = String("fan");
            break;

        case kSamsungAcHeat:
            root["mode"] = String("heat");
            break;

        default:
            break;
        }

    if (ac.getSwing())
        root["swing"] = String("on");
    else
        root["swing"] = String("off");

    publish(root, MQTT_AC_GET_TOPIC);
}

void print_ac_state() {
  logger_info(ac.toString());
}

void ac_turn_on() {
    ac.on();
    // ac.sendExtended();
    // ac.send();
    ac.sendOn();
    //print_ac_state();
}

void ac_turn_off() {
    ac.off();
    //ac.sendExtended();
    //ac.send();
    ac.sendOff();
    //print_ac_state();
}

void ac_swing_off() { ac.setSwing(false); }
void ac_swing_on()  { ac.setSwing(true); }

// function called when a MQTT message arrived
void callback(char* topic, byte* payload, unsigned int length) {
    char command[length+1];
    
    if (strcmp(topic, MQTT_AC_GET_TOPIC) == 0)
        return;

    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    
    for (int i = 0; i < length; i++)
        command[i] = tolower((char)payload[i]);
    command[length] = '\0';
    
    Serial.println(command);

    if (strcmp(MQTT_AC_POWER_SET_TOPIC, topic) == 0) {
      
        if (strcmp(COMM_OFF, command) == 0) {
            Serial.println("Turn off the A/C ...");
            ac_turn_off();
        } else { //if (strcmp(COMM_ON, command) == 0) {
            Serial.println("Turn on the A/C ...");
            ac_turn_on();
        }
        print_ac_state();
        delay(1000);

    } else
    if (strcmp(MQTT_AC_MODE_SET_TOPIC, topic) == 0) {
    
        if (!ac.getPower() && strcmp(COMM_OFF, command) != 0) {
            Serial.println("Turn on the A/C ...");
            ac.on();
        }
        if (strcmp(COMM_AUTO, command) == 0) {
            Serial.println("Set the A/C mode to auto ...");
            ac.setMode(kSamsungAcAuto);
        } else 
        if (strcmp(COMM_OFF, command) == 0) {
            Serial.println("Set the A/C mode to off ...");
            ac_turn_off();
        } else 
        if (strcmp(COMM_HEAT, command) == 0) {
            Serial.println("Set the A/C mode to heat ...");
            ac.setMode(kSamsungAcHeat);
        } else 
        if (strcmp(COMM_COOL, command) == 0) {
            Serial.println("Set the A/C mode to cooling ...");
            ac.setMode(kSamsungAcCool);
        } else 
        if (strcmp(COMM_DRY, command) == 0) {
            Serial.println("Set the A/C mode to dry ...");
            ac.setMode(kSamsungAcDry);
        } else 
        if (strcmp(COMM_FAN_ONLY, command) == 0) {
            Serial.println("Set the A/C mode to fan ...");
            ac.setMode(kSamsungAcFan);
        } else 
          return;

    } else
    if (strcmp(MQTT_AC_TEMP_SET_TOPIC, topic) == 0) {
      
      int temp = round(String(command).toFloat());
      Serial.print("Set temperature to "); Serial.println(temp);
      ac.setTemp(temp);      

    } else
    if (strcmp(MQTT_AC_FAN_SET_TOPIC, topic) == 0) {

        if (strcmp(COMM_FAN_AUTO, command) == 0) {
            Serial.println("Set the A/C fan to auto ...");
            ac.setFan(kSamsungAcFanAuto);
        } else 
        if (strcmp(COMM_FAN_LOW, command) == 0) {
            Serial.println("Set the A/C fan to low ...");
            ac.setFan(kSamsungAcFanLow);
        } else 
        if (strcmp(COMM_FAN_MED, command) == 0) {
            Serial.println("Set the A/C fan to medium ...");
            ac.setFan(kSamsungAcFanMed);
        } else 
        if (strcmp(COMM_FAN_HIGH, command) == 0) {
            Serial.println("Set the A/C fan to high ...");
            ac.setFan(kSamsungAcFanHigh);
        } else 
        if (strcmp(COMM_FAN_TURBO, command) == 0) {
            Serial.println("Set the A/C fan to turbo ...");
            ac.setFan(kSamsungAcFanTurbo);
        } else 
          return;

    } else
    if (strcmp(MQTT_AC_SWING_SET_TOPIC, topic) == 0) {

        if (strcmp(COMM_OFF, command) == 0) {
            Serial.println("Turn off swing A/C ...");
            ac_swing_off();
        } else {
            Serial.println("Turn on swing A/C ...");
            ac_swing_on();
        }

    } else
        return;
    
    publish_ac_state();
    print_ac_state();

    if (!ac.getPower()) {
        Serial.println("AC turned off not sending.");
        return;
    } else
        ac.send();

    delay(500);
}

void send_mqtt_connect_info(int attempt){

    if (LOG_MQTT_CONNECT){
      logger_info(String("IP address: ") + WiFi.localIP().toString());
      logger_info(String("MQTT SERVER IP: ") + MQTT_SERVER_IP + ":" + MQTT_SERVER_PORT);
      logger_info(String("MQTT CLIENT ID: ") + MQTT_CLIENT_ID);
      logger_info(String("temp slope: ") + temp_slope + String(", temp shift: ") + temp_shift
        + String(", humid slope: ") + humid_slope + String(", humid shift: ") + humid_shift);
      logger_info(String("Number of attempts: ") + String(attempt));
    }
}

bool mqtt_connect() {
    // Loop until we're reconnected
    static const int max_attempt = 10;
    int attempt = 0;
    
    while (attempt < max_attempt && !client.connected()) {
        Serial.println("INFO: Attempting MQTT connection...");
        // Attempt to connect
        if (! client.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD)) {
            Serial.print("ERROR: failed, rc="); Serial.println(client.state());
            Serial.println("DEBUG: try again in 2 seconds");
            // Wait 2 seconds before retrying
            delay(2000);
        }
        attempt++;
    }

    if (attempt < max_attempt){
        send_mqtt_connect_info(attempt);
        client.subscribe(MQTT_AC_POWER_SET_TOPIC);
        client.subscribe(MQTT_AC_MODE_SET_TOPIC);
        client.subscribe(MQTT_AC_TEMP_SET_TOPIC);
        client.subscribe(MQTT_AC_FAN_SET_TOPIC);
        client.subscribe(MQTT_AC_SWING_SET_TOPIC);
        client.subscribe(MQTT_AC_GET_TOPIC);
        logger_info(String("MQTT connect in ") + attempt + " attempts");
    } else
        Serial.println("ERROR: max_attempt reached to MQTT connect");
    
    return attempt < max_attempt;
}

bool read_sensors(float &t, float &h){
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  // Read temperature as Celsius (the default)
  t = dht.readTemperature();
  h = dht.readHumidity();

  if (isnan(h) || isnan(t)) {
      delay(2000);
      h = 0.0; t = 0.0; 
      return false;
  } else {
      // adjust DHT11
      h = humid_slope * h + humid_shift;
      t = temp_slope * t + temp_shift; // Read temperature as C
      return true;
  }
}

bool setup_wifi() {

    // init the WiFi connection
    Serial.println();
    Serial.print("INFO: Connecting to ");
    Serial.println(WIFI_SSID);
    WiFi.mode(WIFI_STA);
    
    //  Enable light sleep
    wifi_set_sleep_type(LIGHT_SLEEP_T);
    
    if (local_IP != IPAddress(0,0,0,0) && !WiFi.config(local_IP, gateway, subnet)){
        Serial.println("STA Failed to configure fixed IP");
        return false;    
    }

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    static const int max_attempt = 100;
    int attempt = 0;
    while (attempt < max_attempt && WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        attempt++;
    }
    Serial.println("");

    return (attempt < max_attempt);
}

void setup_ac() {
    ac.begin();
    logger_info("Setting initial state for A/C.");
    ac.setFan(kSamsungAcFanAuto);
    ac.setMode(kSamsungAcFan);
    ac.setTemp(25);
    ac_swing_off();
    //ac.setBeep(true);
    ac_turn_off();
}

int last_read_time;

void setup() {
    // init the serial
    Serial.begin(115200);
    //Take some time to open up the Serial Monitor
    delay(1000);
    
    dht.begin();
    
    // Restart ESP if max attempt reached
    if (!setup_wifi()){
        Serial.println("ERROR: max_attempt reached to WiFi connect");
        Serial.print("Waiting and Restaring");
        WiFi.disconnect();
        delay(1000);
        ESP.restart();
    }

    // init the MQTT connection
    client.setServer(MQTT_SERVER_IP, MQTT_SERVER_PORT);
    client.setCallback(callback);
    mqtt_connect();

    setup_ac();

    last_read_time = 0;
}

int n_sensor_reads = 0;
float ac_t = 0.0, ac_h = 0.0;

void loop() {
    float t, h;

    // Serial.println("Inside loop");
    if (client.connected() || mqtt_connect()) {
        client.loop();
        
        // Average reads
        if (read_sensors(t, h)){
            //Serial.print("read temp: "); Serial.print(t);
            //Serial.print("read humid: "); Serial.println(h);            
            n_sensor_reads++;
            ac_t += t;
            ac_h += h;
        }

        if (last_read_time >= READ_SENSOR_TIME_IN_SECONDS*1000){
            if (n_sensor_reads != 0)
                // Average reads
                publish_data_sensor(ac_t/n_sensor_reads, ac_h/n_sensor_reads);
            else
                logger_warn("Failed to read from DHT sensor!");

            ac_t=0.0; ac_h=0.0;
            n_sensor_reads = 0;

            print_ac_state();
            publish_ac_state();
            last_read_time = 0;
        } 
    }

    last_read_time += SLEEPING_TIME_IN_MSECONDS;
    delay(SLEEPING_TIME_IN_MSECONDS);
}
