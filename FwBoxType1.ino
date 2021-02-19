//
// Copyright (c) 2020 Fw-Box (https://fw-box.com)
// Author: Hartman Hsieh
//
// Description :
//   None
//
// Connections :
//
// Required Library :
//

#include "FwBox.h"
#include <Wire.h>


#define DEVICE_TYPE 1
#define FIRMWARE_VERSION "1.1.3"

#define VAL_INDEX_LED 0


//
// Global variable
//

void setup()
{
  Wire.begin();
  Serial.begin(9600);

  pinMode(LED_BUILTIN, OUTPUT);

  //
  // Initialize the fw-box core (early stage)
  //
  fbEarlyBegin(DEVICE_TYPE, FIRMWARE_VERSION);

  //
  // Initialize the fw-box core
  //
  fbBegin(DEVICE_TYPE, FIRMWARE_VERSION);

  //
  // Setup MQTT subscribe callback
  //
  setRcvValueCallback(onReceiveValue);

  //WiFi.disconnect(); // HTM_DEBUG
} // void setup()

void loop()
{

  FwBoxIns.setValue(VAL_INDEX_LED, !digitalRead(LED_BUILTIN)); // This value is reverse

  //
  // Run the handle
  //
  fbHandle();

} // END OF "void loop()"

void onReceiveValue(int valueIndex, String* payload)
{
  Serial.print("onReceiveValue valueIndex = ");
  Serial.println(valueIndex);
  Serial.print("onReceiveValue *payload = ");
  Serial.println(*payload);

  if(valueIndex == VAL_INDEX_LED) {
    payload->toUpperCase();
    if(payload->equals("ON") == true)
    {
      digitalWrite(LED_BUILTIN, LOW); // This value is reverse
    }
    else
    {
      digitalWrite(LED_BUILTIN, HIGH); // This value is reverse
    }
  }
}
