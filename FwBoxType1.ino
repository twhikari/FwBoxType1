//
// Copyright (c) 2021 Fw-Box (https://fw-box.com)
// Author: Hartman Hsieh
//
// Description :
//   None
//
// Connections :
//   None
//
// Required Library :
//   None
//

#include "FwBox.h"
#include <Wire.h>

#define DEVICE_TYPE 1
#define FIRMWARE_VERSION "1.1.6"

void setup()
{
  Serial.begin(9600);

  //
  // Initialize the fw-box core
  //
  fbBegin(DEVICE_TYPE, FIRMWARE_VERSION);

} // void setup()

void loop()
{
  //
  // Run the handle
  //
  fbHandle();

} // END OF "void loop()"
