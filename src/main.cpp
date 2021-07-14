#include <ESPIoTCommon.hpp>
#include "certificate.h"
#include "secrets.h"

#include <IRrecv.h>
#include <IRsend.h>
#include <IRutils.h>
#include <IRtext.h>
#include <IRremoteESP8266.h>
#include <ArduinoJson.h>
#include <stdlib.h>

#define MAX_BUFFER_SIZE 16384

String deviceID, groupID;
uint32_t mqttGID = 0;
uint32_t mqttDID = 0;
WiFiClientSecure net;
MQTTClient client(MAX_BUFFER_SIZE); /* TODO: バッファサイズの調整． */

const uint16_t irRxPin = 14;
const uint16_t irTxPin = 33;
const uint16_t kCaptureBufferSize = 1024;

#if DECODE_AC
const uint8_t kTimeout = 50;
#else  // DECODE_AC
const uint8_t kTimeout = 15;
#endif // DECODE_AC
const uint16_t kMinUnknownSize = 12;
const uint8_t kTolerancePercentage = kTolerance; // kTolerance is normally 25%

IRrecv irrecv(irRxPin, kCaptureBufferSize, kTimeout, true);
decode_results results;
IRsend irsend(irTxPin);
String mySendTopic, myRecvTopic, myDumpTopic;

void irTxTask(void *arg);
void irRxTask();

void mqttCallBack(String &topic, String &payload)
{
  Serial.println("incoming: " + topic + " - " + payload);
  if (mySendTopic.compareTo(topic) == 0)
  {
    DynamicJsonDocument doc(MAX_BUFFER_SIZE);
    DeserializationError e = deserializeJson(doc, payload);
    if (e)
    {
      Serial.println("Deserialization Error");
      doc.clear();
      doc["status"] = "Failed";
      doc["topic"] = mySendTopic;
      doc["msg"] = "Deserialization failed in " + mySendTopic;
      doc["device"] = deviceID;
      client.publish("log/error");
      return;
    }
    uint16_t *rawData = (uint16_t *)malloc(sizeof(uint16_t) * 1500);
    String type = doc["type"].as<String>();
    rawData[0] = 0;
    if (type.compareTo("raw") == 0)
    {
      JsonArray signal = doc["signal"];
      int i = 1;
      for (JsonVariant v : signal)
      {
        rawData[i] = v.as<uint16_t>();
        i++;
      }
      rawData[0] = i-1;
    }
    else if (type.compareTo("command"))
    {
      // TODO: implement.
    }
    if (rawData[0] != 0)
    {
      xTaskCreatePinnedToCore(irTxTask, "ir send", 4096, (void*)rawData, 1, NULL, tskNO_AFFINITY);
    }
  }
  else if (myRecvTopic.compareTo(topic) == 0)
  {
    irRxTask();
  }
  else if (String("identity/provide").compareTo(topic) == 0)
  {
    gotMqttIds(groupID, deviceID, payload, &mqttGID, &mqttDID, &client);
    if (mqttGID != 0 && mqttDID != 0)
    {
      char myDevId[10];
      sprintf(myDevId, "%d", mqttDID);
      myRecvTopic = "ir/";
      myRecvTopic += myDevId;
      myDumpTopic = myRecvTopic;
      mySendTopic = myRecvTopic;
      myRecvTopic += "_recv";
      myDumpTopic += "_dump";
      mySendTopic += "_send";
      client.subscribe(myRecvTopic.c_str());
      client.subscribe(mySendTopic.c_str());
    }
  }
}

void setup()
{
  serialSetup();
  setupWiFi();
  createIDs(&groupID, &deviceID);
  setupMqtts(MQTT_SERVER, MQTT_PORT, test_ca_cert, mqttCallBack, &net, &client);
  String user = MQTT_USER;
  String pass = MQTT_PASS;
  reconnectMqtt(&client, deviceID.c_str(), user.c_str(), pass.c_str());

  assert(irutils::lowLevelSanityCheck() == 0);
  Serial.printf("\n" D_STR_IRRECVDUMP_STARTUP "\n", irRxPin);
  irrecv.setUnknownThreshold(kMinUnknownSize);
  irrecv.setTolerance(kTolerancePercentage);
  irsend.begin();
  irrecv.enableIRIn();
}

void irTxTask(void *arg)
{
  size_t n = ((uint16_t*)arg)[0];
  uint16_t *rawData = ((uint16_t*)arg + 1);
  irsend.sendRaw(rawData, n, 38);
  free(arg);
  vTaskDelete(NULL);
}

void irRxTask()
{
  if (irrecv.decode(&results))
  {
    DynamicJsonDocument doc(MAX_BUFFER_SIZE);
    if (results.overflow)
    {
      Serial.printf(D_WARN_BUFFERFULL "\n", kCaptureBufferSize);
      doc["status"] = "Failed";
      doc["msg"] = "Size up the Buffer lager.";
      char *jsonBuffer = (char *)malloc(MAX_BUFFER_SIZE);
      size_t n = serializeJson(doc, jsonBuffer, MAX_BUFFER_SIZE);
      client.publish(myDumpTopic.c_str(), jsonBuffer, n);
      free(jsonBuffer);
    }
    else
    {
      DynamicJsonDocument signal(MAX_BUFFER_SIZE);
      Serial.println(resultToHumanReadableBasic(&results));
      uint16_t *rawArray = resultToRawArray(&results);
      for (auto i = 0; i < results.rawlen - 1; i++)
      {
        signal.add(rawArray[i]);
      }
      doc["status"] = "Success";
      doc["signal"] = signal;
      char *jsonBuffer = (char *)malloc(MAX_BUFFER_SIZE);
      size_t n = serializeJson(doc, jsonBuffer, MAX_BUFFER_SIZE);
      client.publish(myDumpTopic.c_str(), jsonBuffer, n);
      free(jsonBuffer);
    }
  }
}

void loop()
{
  client.loop();

  if (!client.connected())
  {
    Serial.println("Not connected");
    reconnectMqtt(&client, deviceID.c_str(), MQTT_USER, MQTT_PASS);
  }
  else if (mqttGID == 0 && mqttDID == 0)
  {
    setupMqttIds(groupID, deviceID, &client);
  }
}