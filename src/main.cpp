#include <ESPIoTCommon.hpp>
#include "certificate.h"
#include "secrets.h"

#include <IRrecv.h>
#include <IRsend.h>
#include <IRutils.h>
#include <IRremoteESP8266.h>

String deviceID, groupID;
uint32_t mqttGID, mqttDID;
WiFiClientSecure net;
MQTTClient client(16384); /* TODO: バッファサイズの調整． */

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

// Use turn on the save buffer feature for more complete capture coverage.
IRrecv irrecv(irRxPin, kCaptureBufferSize, kTimeout, true);
decode_results results; // Somewhere to store the results
IRsend irsend(irTxPin);

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
  
  assert(irutils::lowLevelSanityCheck() == 0);
  Serial.printf("\n" D_STR_IRRECVDUMP_STARTUP "\n", irRxPin);
  irrecv.setUnknownThreshold(kMinUnknownSize);
  irrecv.setTolerance(kTolerancePercentage); 
  irsend.begin();
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