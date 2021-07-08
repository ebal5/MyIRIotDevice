#include <Arduino.h>
#include <ESPIoTCommon.hpp>

String deviceID, groupID;

void setup() {
  // put your setup code here, to run once:
  serialSetup();
  setupWiFi();
  createIDs(&groupID, &deviceID);
}

void loop() {
  // put your main code here, to run repeatedly:
}