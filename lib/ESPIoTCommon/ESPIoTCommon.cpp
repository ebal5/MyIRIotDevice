#include "ESPIoTCommon.hpp"
#include "ESPIoTCommonValue.hpp"

#include <Arduino.h>
#include <WiFi.h>
#include <mbedtls/md.h>
#include <WiFiClientSecure.h>
#include <MQTT.h>
#include <ArduinoJson.h>

bool setupWiFi()
{
    Serial.println("WiFi begin");
    WiFi.begin();
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(500);
        if (10000 < millis())
            break;
    }
    Serial.println();
    if (WiFi.status() != WL_CONNECTED)
    {
        WiFi.mode(WIFI_STA);
        WiFi.beginSmartConfig();
        Serial.println("Waiting for SmartConfig");
        while (!WiFi.smartConfigDone())
        {
            delay(500);
            Serial.print("#");
            if (30000 < millis())
            {
                Serial.println();
                Serial.println("Reset");
                ESP.restart();
            }
        }
        Serial.println();
        Serial.println("Waiting for WiFi");
        while (WiFi.status() != WL_CONNECTED)
        {
            delay(500);
            Serial.print(".");
            if (60000 < millis())
            {
                Serial.println("");
                Serial.println("Reset");
                ESP.restart();
            }
        }
        Serial.println("");
        Serial.println("WiFi Connected.");
    }
    String BSSID = WiFi.BSSIDstr();
    String Mac = WiFi.macAddress();
    Serial.printf("BSSID: %s\n", BSSID.c_str());
    Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());
    return WiFi.isConnected();
}

void setupMqttIds(String groupID, String deviceID, MQTTClient *client)
{
    static unsigned long lastIdSend = 0;
    unsigned long now = millis();
    if (lastIdSend == 0 || now - lastIdSend >= 5000)
    {
        StaticJsonDocument<200> doc;
        char buffer[256];
        String test;
        doc["groupId"] = groupID;
        doc["devId"] = deviceID;
        doc["millis"] = now;
        size_t n = serializeJson(doc, buffer);
        // client->publish("identity/request", buffer);
        client->publish("identity/request", buffer, n);
        // client->publish("identity/request", "hello");
        Serial.println("Sending ID Request to Broker...");
        Serial.println(buffer);
        client->subscribe("identity/provide");
        lastIdSend = now;
    }
}

void reconnectMqtt(MQTTClient *client, const char *clientID, const char *user, const char *pass)
{
    Serial.print("checking wifi...");
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(1000);
    }

    Serial.print("\nconnecting...");

    while (!client->connect(clientID, user, pass))
    {
        Serial.print(".");
        delay(1000);
    }

    Serial.println("\nconnected!");
    client->subscribe("identity/provide");
}

bool setupMqtt(String server, const uint16_t port, MQTTClientCallbackSimple callback, WiFiClient *net, MQTTClient *client)
{
    client->onMessage(callback);
    client->begin(server.c_str(), port, *net);
    return true;
}

bool setupMqtts(String server, const uint16_t port, const char *CACert, MQTTClientCallbackSimple callback, WiFiClientSecure *net, MQTTClient *client)
{
    net->setCACert(CACert);
    client->onMessage(callback);
    client->begin(server.c_str(), port, *net);
    return true;
}

String toHash(String target)
{
    char payload[64];
    byte hashBuff[32];
    char hexBuff[3];
    String result;
    strcpy(payload, target.c_str());
    mbedtls_md_context_t ctx;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
    size_t payloadLength = strlen(payload);
    mbedtls_md_init(&ctx);
    mbedtls_md_starts(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0);
    mbedtls_md_starts(&ctx);
    mbedtls_md_update(&ctx, (const unsigned char *)payload, payloadLength);
    mbedtls_md_finish(&ctx, (unsigned char *)hashBuff);
    for (byte hash : hashBuff)
    {
        sprintf(hexBuff, "%02X", hash);
        result += hexBuff;
    }
    return result;
}

void createIDs(String *gid, String *did)
{
    createIDs(gid, did, DefaultPepper);
}

void createIDs(String *gid, String *did, String pepper)
{
    String BSSID = WiFi.BSSIDstr();
    String Mac = WiFi.macAddress();

    BSSID += pepper;
    Mac += pepper;
    *gid = toHash(BSSID);
    *did = toHash(Mac);
    Serial.printf("%10s : %s\n", "groupID", gid->c_str());
    Serial.printf("%10s : %s\n", "deviceID", did->c_str());
}

bool serialSetup()
{
    return serialSetup(DefaultBaudRate);
}

bool serialSetup(const uint32_t baudRate)
{
#if defined(ESP8266)
    Serial.begin(baudRate, SERIAL_8N1, SERIAL_TX_ONLY);
#else               // ESP8266
    Serial.begin(baudRate, SERIAL_8N1);
#endif              // ESP8266
    while (!Serial) // Wait for the serial connection to be establised.
    {
        delay(50);
    }
    return Serial ? true : false;
}