#include <ESPIoTCommon.hpp>
#include "certificate.h"
#include "secrets.h"

String deviceID, groupID;
uint32_t mqttGID, mqttDID;
WiFiClientSecure net;
MQTTClient client(16384);

void mqttCallBack(String &topic, String &payload)
{
  Serial.println("incoming: " + topic + " - " + payload);
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
}
  
void loop() {
  client.loop();

  if (!client.connected()) {
    Serial.println("Not connected");
    reconnectMqtt(&client, deviceID.c_str(), MQTT_USER, MQTT_PASS);
  }
  else if (mqttGID == 0 && mqttDID == 0)
  {
    setupMqttIds(groupID, deviceID, &client);
  }
}