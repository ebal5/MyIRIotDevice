#if !defined(ESPIoTCommon_HPP)
#define ESPIoTCommon_HPP
#include <Arduino.h>
#include <MQTT.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

bool setupWiFi();
bool setupMqtt(String server, const uint16_t port, MQTTClientCallbackSimple callback, WiFiClient *net, MQTTClient *client);
bool setupMqtts(String server, const uint16_t port, const char *CACert, MQTTClientCallbackSimple callback, WiFiClientSecure *net, MQTTClient *client);
void reconnectMqtt(MQTTClient* client, const char* clientID, const char* user, const char* pass);
void setupMqttIds(String groupID, String deviceID, MQTTClient* client);
void gotMqttIds(String groupID, String deviceID, String payload, uint32_t* mqttGID, uint32_t *mqttDID, MQTTClient* client);
void createIDs(String* gid, String* did);
void createIDs(String* gid, String* did, String pepper);
bool serialSetup();
bool serialSetup(const uint32_t baudRate);

#endif // ESPIoTCommon_HPP
