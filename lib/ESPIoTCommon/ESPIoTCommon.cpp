#include "ESPIoTCommon.hpp"
#include "ESPIoTCommonValue.hpp"

#include <Arduino.h>
#include <WiFi.h>
#include <mbedtls/md.h>
#include <WiFiClientSecure.h>
#include <MQTTClient.h>
#include <ArduinoJson.hpp>


unsigned long lastIdSend = 0;

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

void setupMqttIds(String groupID, String deviceID)
{
    unsigned long now = millis();
    if (lastIdSend == 0 || now - lastIdSend >= 5000)
    {
        lastIdSend = now;
    }
}

void reconnectMqtt(MQTTClient client, String clientID, String user, String pass)
{
    while (!client.connected())
    {
        Serial.print("Attempting MQTT connection...");
        if (!client.connect(clientID.c_str(), user.c_str(), pass.c_str(), false))
        {
            Serial.print("failed, rc=");
            Serial.print(client.lastError());
            Serial.println(" try again in 2 seconds");
            delay(2000);
        }
        else
        {
            Serial.println("connected");
        }
    }
}

bool setupMqtt(String server, const uint16_t port, MQTTClientCallbackSimple callback, WiFiClient *net, MQTTClient *client)
{
    client->setHost(server.c_str(), (int)port);
    client->onMessage(callback);
    return true;
}

bool setupMqtts(String server, const uint16_t port, const char *CACert, MQTTClientCallbackSimple callback, WiFiClientSecure *net, MQTTClient *client)
{
    net->setCACert(CACert);
    client->setHost(server.c_str(), (int)port);
    client->onMessage(callback);
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