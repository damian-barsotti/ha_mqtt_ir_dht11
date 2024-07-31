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

#include <ArduinoJson.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ir_Samsung.h>
#include "src/esp8266_controllers/Wifi/Wifi.h"
#include "src/esp8266_controllers/HTReader/HTReader.h"
#include "src/esp8266_controllers/Mqtt/Mqtt.h"

#if __has_include("config_local.h")
#include "config_local.h" // File for testing outside git
#else
#include "config.h"
#endif

IRSamsungAc ac(kIrLed); // Set the GPIO used for sending messages.

#ifdef CONFIG_H
Wifi wifi(Serial, WIFI_SSID, WIFI_PASSWORD);
#else
Wifi wifi(Serial, WIFI_SSID, WIFI_PASSWORD, local_IP, gateway, subnet);
#endif
HTReader ht_sensor(
    DHTPIN, DHTTYPE, SLEEPING_TIME_IN_MSECONDS,
    temp_slope, temp_shift,
    humid_slope, humid_shift,
    n_reads);

std::array topics{MQTT_SENSOR_TOPIC,
                  MQTT_AC_POWER_SET_TOPIC,
                  MQTT_AC_MODE_SET_TOPIC,
                  MQTT_AC_TEMP_SET_TOPIC,
                  MQTT_AC_FAN_SET_TOPIC,
                  MQTT_AC_SWING_SET_TOPIC,
                  MQTT_AC_GET_TOPIC};

Mqtt mqtt(Serial, MQTT_SERVER_IP, MQTT_SERVER_PORT,
          MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD,
          MQTT_LOG_TOPIC,
          topics.data(), topics.size(),
          callback);

String header_log(char const *level, int n_log)
{
    return String(level) + " " + String(n_log) + ": ";
}

String header_log_info(int n_log)
{
    return header_log("INFO", n_log);
}

String header_log_warn(int n_log)
{
    return header_log("WARN", n_log);
}

int n_log_info = 0;
int n_log_warn = 0;

void logger_info(String msg)
{
    if (mqtt.log(header_log_info(n_log_info) + msg))
        n_log_info++;
}

void logger_warn(String msg)
{
    if (mqtt.log(header_log_warn(n_log_warn) + msg))
        n_log_warn++;
}

// function called to publish the temperature and the humidity
void publish_data_sensor(float p_temperature, float p_humidity)
{
    DynamicJsonDocument root(200);
    root["temperature"] = (String)p_temperature;
    root["humidity"] = (String)p_humidity;

    mqtt.publish(root, MQTT_SENSOR_TOPIC);
}

// function called to publish the AC state
void publish_ac_state()
{
    DynamicJsonDocument root(200);

    root["temperature"] = String(ac.getTemp());
    switch (ac.getFan())
    {
    case kSamsungAcFanAuto:
        root["fan"] = String(COMM_FAN_AUTO);
        break;

    case kSamsungAcFanLow:
        root["fan"] = String(COMM_FAN_LOW);
        break;

    case kSamsungAcFanMed:
        root["fan"] = String(COMM_FAN_MED);
        break;

    case kSamsungAcFanHigh:
        root["fan"] = String(COMM_FAN_HIGH);
        break;

    case kSamsungAcFanTurbo:
        root["fan"] = String(COMM_FAN_TURBO);
        break;

    default:
        break;
    }

    if (!ac.getPower())
        root["mode"] = String(COMM_OFF);
    else
        switch (ac.getMode())
        {
        case kSamsungAcAuto:
            root["mode"] = String(COMM_AUTO);
            break;

        case kSamsungAcCool:
            root["mode"] = String(COMM_COOL);
            break;

        case kSamsungAcDry:
            root["mode"] = String(COMM_DRY);
            break;

        case kSamsungAcFan:
            root["mode"] = String(COMM_FAN_ONLY);
            break;

        case kSamsungAcHeat:
            root["mode"] = String(COMM_HEAT);
            break;

        default:
            break;
        }

    if (ac.getSwing())
        root["swing"] = String("on");
    else
        root["swing"] = String("off");

    mqtt.publish(root, MQTT_AC_GET_TOPIC);
}

void print_ac_state()
{
    logger_info(ac.toString());
}

void ac_turn_on()
{
    ac.on();
    // ac.sendExtended();
    // ac.send();
    ac.sendOn();
    // print_ac_state();
}

void ac_turn_off()
{
    ac.off();
    // ac.sendExtended();
    // ac.send();
    ac.sendOff();
    // print_ac_state();
}

void ac_swing_off() { ac.setSwing(false); }
void ac_swing_on() { ac.setSwing(true); }

// function called when a MQTT message arrived
void callback(char *topic, byte *payload, unsigned int length)
{
    char command[length + 1];

    mqtt.announce_callback();

    if (strcmp(topic, MQTT_AC_GET_TOPIC) == 0 ||
        strcmp(topic, MQTT_LOG_TOPIC) == 0 ||
        strcmp(topic, MQTT_SENSOR_TOPIC) == 0)
        return;

    Serial.print("CB: Message arrived [");
    Serial.print(topic);
    Serial.print("] ");

    for (int i = 0; i < length; i++)
        command[i] = tolower((char)payload[i]);
    command[length] = '\0';

    Serial.println(command);

    if (strcmp(MQTT_AC_POWER_SET_TOPIC, topic) == 0)
    {

        if (strcmp(COMM_OFF, command) == 0)
        {
            Serial.println("Turn off the A/C ...");
            ac_turn_off();
        }
        else
        { // if (strcmp(COMM_ON, command) == 0) {
            Serial.println("Turn on the A/C ...");
            ac_turn_on();
        }
        print_ac_state();
        delay(1000);
    }
    else if (strcmp(MQTT_AC_MODE_SET_TOPIC, topic) == 0)
    {

        if (!ac.getPower() && strcmp(COMM_OFF, command) != 0)
        {
            Serial.println("Turn on the A/C ...");
            ac.on();
        }
        if (strcmp(COMM_AUTO, command) == 0)
        {
            Serial.println("Set the A/C mode to auto ...");
            ac.setMode(kSamsungAcAuto);
        }
        else if (strcmp(COMM_OFF, command) == 0)
        {
            Serial.println("Set the A/C mode to off ...");
            ac_turn_off();
        }
        else if (strcmp(COMM_HEAT, command) == 0)
        {
            Serial.println("Set the A/C mode to heat ...");
            ac.setMode(kSamsungAcHeat);
        }
        else if (strcmp(COMM_COOL, command) == 0)
        {
            Serial.println("Set the A/C mode to cooling ...");
            ac.setMode(kSamsungAcCool);
        }
        else if (strcmp(COMM_DRY, command) == 0)
        {
            Serial.println("Set the A/C mode to dry ...");
            ac.setMode(kSamsungAcDry);
        }
        else if (strcmp(COMM_FAN_ONLY, command) == 0)
        {
            Serial.println("Set the A/C mode to fan ...");
            ac.setMode(kSamsungAcFan);
        }
        else
            return;
    }
    else if (strcmp(MQTT_AC_TEMP_SET_TOPIC, topic) == 0)
    {

        int temp = round(String(command).toFloat());
        Serial.print("Set temperature to ");
        Serial.println(temp);
        ac.setTemp(temp);
    }
    else if (strcmp(MQTT_AC_FAN_SET_TOPIC, topic) == 0)
    {

        if (strcmp(COMM_FAN_AUTO, command) == 0)
        {
            Serial.println("Set the A/C fan to auto ...");
            ac.setFan(kSamsungAcFanAuto);
        }
        else if (strcmp(COMM_FAN_LOW, command) == 0)
        {
            Serial.println("Set the A/C fan to low ...");
            ac.setFan(kSamsungAcFanLow);
        }
        else if (strcmp(COMM_FAN_MED, command) == 0)
        {
            Serial.println("Set the A/C fan to medium ...");
            ac.setFan(kSamsungAcFanMed);
        }
        else if (strcmp(COMM_FAN_HIGH, command) == 0)
        {
            Serial.println("Set the A/C fan to high ...");
            ac.setFan(kSamsungAcFanHigh);
        }
        else if (strcmp(COMM_FAN_TURBO, command) == 0)
        {
            Serial.println("Set the A/C fan to turbo ...");
            ac.setFan(kSamsungAcFanTurbo);
        }
        else
            return;
    }
    else if (strcmp(MQTT_AC_SWING_SET_TOPIC, topic) == 0)
    {

        if (strcmp(COMM_OFF, command) == 0)
        {
            Serial.println("Turn off swing A/C ...");
            ac_swing_off();
        }
        else
        {
            Serial.println("Turn on swing A/C ...");
            ac_swing_on();
        }
    }
    else
        return;

    publish_ac_state();
    print_ac_state();

    if (!ac.getPower())
    {
        Serial.println("AC turned off not sending.");
        return;
    }
    else
        ac.send();

    delay(500);
}

void send_mqtt_connect_info(int attempt)
{

    if (LOG_MQTT_CONNECT)
    {
        logger_info(String("IP address: ") + wifi.localIP().toString());
        logger_info(String("MQTT SERVER IP: ") + MQTT_SERVER_IP + ":" + MQTT_SERVER_PORT);
        logger_info(String("MQTT CLIENT ID: ") + MQTT_CLIENT_ID);
        logger_info(String("temp slope: ") + temp_slope + String(", temp shift: ") + temp_shift + String(", humid slope: ") + humid_slope + String(", humid shift: ") + humid_shift);
        logger_info(String("Number of attempts: ") + String(attempt));
    }
}

void setup_ac()
{
    ac.begin();
    logger_info("Setting initial state for A/C.");
    ac.setFan(kSamsungAcFanAuto);
    ac.setMode(kSamsungAcFan);
    ac.setTemp(25);
    ac_swing_off();
    // ac.setBeep(true);
    ac_turn_off();
}

void setup()
{
    // init the serial
    Serial.begin(115200);
    // Take some time to open up the Serial Monitor
    delay(1000);

    // Restart ESP if max attempt reached
    if (!wifi.begin())
    {
        Serial.println("ERROR: max_attempt reached to WiFi connect");
        Serial.print("Waiting and Restaring");
        wifi.disconnect();
        delay(1000);
        ESP.restart();
    }

    Serial.println(String("IP: ") + wifi.localIP().toString());

    // init the MQTT connection
    if (!mqtt.begin() && mqtt.attempt() > mqtt_max_attempt)
    {
        Serial.println("MQTT error");
        Serial.print("Waiting and Restaring");
        wifi.disconnect();
        delay(1000);
        ESP.restart();
    }

    setup_ac();

    ht_sensor.begin();
    if (ht_sensor.error())
        logger_warn("Failed to read from DHT sensor!");
}

void loop()
{
    float t, h;

    Serial.print("-");
    if (mqtt.beginLoop())
    {
        if (ht_sensor.error())
            Serial.println("Failed to read from sensor!");

        if (ht_sensor.beginLoop())
        {
            Serial.println();
            publish_data_sensor(ht_sensor.getTemp(), ht_sensor.getHumid());
            print_ac_state();
            publish_ac_state();
        }

        if (ht_sensor.error())
            logger_warn("Failed to read from DHT sensor!");
    }
    else if (mqtt.attempt() > mqtt_max_attempt)
    {
        Serial.println("Cannot connect to mqtt");
        Serial.print("Waiting and Restaring");
        mqtt.reset();
        wifi.disconnect();
        delay(1000);
        ESP.restart();
    }

    delay(SLEEPING_TIME_IN_MSECONDS);
}
