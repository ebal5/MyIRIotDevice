#if !defined(ESPIoTCommon_HPP)
#define ESPIoTCommon_HPP
#include <Arduino.h>
#include <MQTTClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

bool setupWiFi();
bool setupMqtt(String server, const uint16_t port, WiFiClient* net, MQTTClient* client);
bool setupMqtts(String server, const uint16_t port, const char * CACert, WiFiClientSecure* net, MQTTClient* client);
void reconnectMqtt(MQTTClient client, String clientID, String user, String pass);
void setupMqttIds(String groupID, String deviceID);
void createIDs(String* gid, String* did);
void createIDs(String* gid, String* did, String pepper);
bool serialSetup();
bool serialSetup(const uint32_t baudRate);

#endif // ESPIoTCommon_HPP
