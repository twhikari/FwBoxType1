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

#include <Wire.h>
#include "FwBox.h"
#include "FwBox_PMSX003.h"
#include "SoftwareSerial.h"
#include <U8g2lib.h>
#include "FwBox_UnifiedLcd.h"
#include "FwBox_NtpTime.h"
#include "FwBox_TwWeather.h"
#include "FwBox_U8g2Widget.h"

#define DEVICE_TYPE 26
#define FIRMWARE_VERSION "1.0.2"

#define ANALOG_VALUE_DEBOUNCING 8

//
// Debug definitions
//
#define FW_BOX_DEBUG 0

#if FW_BOX_DEBUG == 1
  #define DBG_PRINT(VAL) Serial.print(VAL)
  #define DBG_PRINTLN(VAL) Serial.println(VAL)
  #define DBG_PRINTF(FORMAT, ARG) Serial.printf(FORMAT, ARG)
  #define DBG_PRINTF2(FORMAT, ARG1, ARG2) Serial.printf(FORMAT, ARG1, ARG2)
#else
  #define DBG_PRINT(VAL)
  #define DBG_PRINTLN(VAL)
  #define DBG_PRINTF(FORMAT, ARG)
  #define DBG_PRINTF2(FORMAT, ARG1, ARG2)
#endif

//
// Global variable
//
const char* WEEK_DAY_NAME[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"}; // Days Of The Week 

//
// LCD 1602
//
FwBox_UnifiedLcd* Lcd = 0;

//
// OLED 128x128
//
U8G2_SSD1327_MIDAS_128X128_1_HW_I2C* u8g2 = 0;

SoftwareSerial SerialSensor(13, 15); // RX:D7, TX:D8

FwBox_PMSX003 Pms(&SerialSensor);

String RemoteMessage = "RemoteMessage";

//
// The library for the Taiwan's weather.
//
FwBox_TwWeather TwWeather;
FwBox_WeatherResult WeatherResult;

String ValUnit[MAX_VALUE_COUNT];

unsigned long ReadingTime = 0;
unsigned long ReadingTimeWeather = 0;

int DisplayMode = 0;

void setup()
{
  Serial.begin(9600);

  //
  // Initialize the fw-box core (early stage)
  //
  fbEarlyBegin(DEVICE_TYPE, FIRMWARE_VERSION);

  FwBoxIns.setGpioStatusLed(LED_BUILTIN);
  pinMode(LED_BUILTIN, OUTPUT);

  //
  // Initialize the LCD1602
  //
  Lcd = new FwBox_UnifiedLcd(16, 2);
  if (Lcd->begin() != 0) {
    //
    // LCD1602 doesn't exist, delete it.
    //
    delete Lcd;
    Lcd = 0;
    DBG_PRINTLN("LCD1602 initialization failed.");
  }

  //
  // Detect the I2C address of OLED.
  //
  Wire.beginTransmission(0x78>>1);
  if (Wire.endTransmission() == 0) {
    u8g2 = new U8G2_SSD1327_MIDAS_128X128_1_HW_I2C(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);  /* Uno: A4=SDA, A5=SCL, add "u8g2.setBusClock(400000);" into setup() for speedup if possible */
    u8g2->begin();
    u8g2->enableUTF8Print();
    u8g2->setFont(u8g2_font_unifont_t_chinese1);  // use chinese2 for all the glyphs of "你好世界"
  }
  else {
    DBG_PRINTLN("U8G2_SSD1327_MIDAS_128X128_1_HW_I2C is not found.");
    u8g2 = 0;
  }

  //
  // Set the unit of the values before "display".
  //
  ValUnit[0] = "μg/m³";
  ValUnit[1] = "μg/m³";
  ValUnit[2] = "μg/m³";
  ValUnit[3] = "°C";
  ValUnit[4] = "%";

  //
  // Display the screen
  //
  display(analogRead(A0)); // Read 'A0' to change the display mode.

  //
  // Initialize the fw-box core
  //
  fbBegin(DEVICE_TYPE, FIRMWARE_VERSION);

  //
  // Initialize the PMSX003 Sensor
  //
  Pms.begin();

  //
  // Get the and unit of the values from fw-box server after "fbBegin".
  //
  for (int vi = 0; vi < 4; vi++) {
    if (FwBoxIns.getValUnit(vi).length() > 0)
      ValUnit[vi] = FwBoxIns.getValUnit(vi);
  }

  //
  // Init the library
  //
  String ps[2];
  int ac = FwBoxIns.getParameterArray(ps, 2);
  DBG_PRINTF("ParameterArray Count = %d\n", ac);
  if(ps[0].length() <= 0) {
    //
    // Default location
    //
    ps[0] = "新北市";
    ps[1] = "板橋區";
  }
  DBG_PRINTLN(ps[0]);
  DBG_PRINTLN(ps[1]);
  TwWeather.begin("CWB-01A3B3EB-21FC-46FA-B5DF-D8178A1A8437", ps[0], ps[1]);
  
  //
  // Sync NTP time
  //
  FwBox_NtpTimeBegin();

} // void setup()

void loop()
{
  if((millis() - ReadingTime) > 2000) {
    //
    // Read the sensors
    //
    if(read() == 0) { // Success
      if((Pms.pm1_0() == 0) && (Pms.pm2_5() == 0) && (Pms.pm10_0() == 0) && (Pms.temp() == 0) && (Pms.humi() == 0)) {
        DBG_PRINTLN("Invalid values");
      }
      else {
        FwBoxIns.setValue(0, Pms.pm1_0());
        FwBoxIns.setValue(1, Pms.pm2_5());
        FwBoxIns.setValue(2, Pms.pm10_0());
        FwBoxIns.setValue(3, Pms.temp());
        FwBoxIns.setValue(4, Pms.humi());
      }
    }

    ReadingTime = millis();
  }

  if ((ReadingTimeWeather == 0) || ((millis() - ReadingTimeWeather) > 60*60*1000) || (WeatherResult.Wx1.length() <= 0)) {
    if ((WiFi.status() == WL_CONNECTED)) {
      WeatherResult = TwWeather.read(year(), month(), day(), hour(), minute());
      //Serial.println("===SUCCESS===");
      if (WeatherResult.WxResult == true) {
        DBG_PRINTLN(WeatherResult.Wx1);
        DBG_PRINTLN(WeatherResult.Wx2);
        DBG_PRINTLN(WeatherResult.Wx3);
      }
      if (WeatherResult.TResult == true) {
        DBG_PRINTLN(WeatherResult.T1);
        DBG_PRINTLN(WeatherResult.T2);
        DBG_PRINTLN(WeatherResult.T3);
      }

      ReadingTimeWeather = millis();
    }
  }

  //
  // Display the screen
  //
  display(analogRead(A0)); // Read 'A0' to change the display mode.

  //
  // Run the handle
  //
  fbHandle();

} // END OF "void loop()"

uint8_t read()
{
  //
  // Running readPms before running pm2_5, temp, humi and readDeviceType.
  //
  if(Pms.readPms()) {
    if(Pms.readDeviceType() == FwBox_PMSX003::PMS5003T) {
      DBG_PRINTLN("PMS5003T is detected.");
      DBG_PRINT("PM1.0=");
      DBG_PRINTLN(Pms.pm1_0());
      DBG_PRINT("PM2.5=");
      DBG_PRINTLN(Pms.pm2_5());
      DBG_PRINT("PM10=");
      DBG_PRINTLN(Pms.pm10_0());
      DBG_PRINT("Temperature=");
      DBG_PRINTLN(Pms.temp());
      DBG_PRINT("Humidity=");
      DBG_PRINTLN(Pms.humi());
      return 0; // Success
    }
    else if(Pms.readDeviceType() == FwBox_PMSX003::PMS5003) {
      DBG_PRINTLN("PMS5003 is detected.");
      DBG_PRINT("PM1.0=");
      DBG_PRINTLN(Pms.pm1_0());
      DBG_PRINT("PM2.5=");
      DBG_PRINTLN(Pms.pm2_5());
      DBG_PRINT("PM10=");
      DBG_PRINTLN(Pms.pm10_0());
      return 0; // Success
    }
  }

  DBG_PRINTLN("PMS data format is wrong.");

  return 1; // Error
}

void display(int analogValue)
{
  //
  // Draw the LCD1602
  //
  if(Lcd != 0) {
    char buff[32];

    memset(&(buff[0]), 0, 32);
    sprintf(buff, "%.2fC %.2f%%", Pms.temp(), Pms.humi());

    //
    // Center the string.
    //
    Lcd->printAtCenter(0, buff);

    memset(&(buff[0]), 0, 32);
    sprintf(buff, "%d ug/m3", Pms.pm2_5());

    //
    // Center the string.
    //
    Lcd->printAtCenter(1, buff);
  }

  //
  // Draw the OLED
  //
  if (u8g2 != 0) {
    //
    // Change the display mode according to the value of PIN - 'A0'.
    //
    DisplayMode = getDisplayMode(4, analogValue);
    DBG_PRINTF("analogValue=%d\n", analogValue);
    DBG_PRINTF("DisplayMode=%d\n", DisplayMode);

    switch (DisplayMode) {
      case 1:
        OledDisplayType1();
        break;
      case 2:
        OledDisplayType2();
        break;
      case 3:
        OledDisplayType3();
        break;
      case 4:
        OledDisplayType4();
        break;
      default:
        OledDisplayType1();
        break;
    }
  }
}

void OledDisplayType1()
{
  u8g2->firstPage();
  do {
    int line_index = 0;
    u8g2->setFont(u8g2_font_unifont_t_chinese1);
    
    String line = "PM1.0 " + String(Pms.pm1_0()) + " " + ValUnit[0];
    u8g2->setCursor(TEXT_GAP, WORD_HEIGHT + TEXT_GAP + (LINE_HEIGHT * line_index));
    u8g2->print(line);
    line_index++;

    line = "PM2.5 " + String(Pms.pm2_5()) + " " + ValUnit[1];
    u8g2->setCursor(TEXT_GAP, WORD_HEIGHT + TEXT_GAP + (LINE_HEIGHT * line_index));
    u8g2->print(line);
    line_index++;

    line = "PM10  " + String(Pms.pm10_0()) + " " + ValUnit[2];
    u8g2->setCursor(TEXT_GAP, WORD_HEIGHT + TEXT_GAP + (LINE_HEIGHT * line_index));
    u8g2->print(line);
    line_index++;

    if(Pms.readDeviceType() == FwBox_PMSX003::PMS5003T) {
        line = "溫度 " + String(Pms.temp()) + " " + ValUnit[3];
        u8g2->setCursor(TEXT_GAP, WORD_HEIGHT + TEXT_GAP + (LINE_HEIGHT * line_index));
        u8g2->print(line);
        line_index++;
        line = "濕度 " + String(Pms.humi()) + " " + ValUnit[4];
        u8g2->setCursor(TEXT_GAP, WORD_HEIGHT + TEXT_GAP + (LINE_HEIGHT * line_index));
        u8g2->print(line);
        line_index++;
    }

    u8g2->setCursor(TEXT_GAP, WORD_HEIGHT + TEXT_GAP + (LINE_HEIGHT * line_index));
    u8g2->print(RemoteMessage);

    drawSmallIcons(u8g2, (WiFi.status() == WL_CONNECTED), (FwBoxIns.getServerStatus() == SERVER_STATUS_OK));
  }
  while (u8g2->nextPage());
}

void OledDisplayType2()
{
  u8g2->firstPage();
  do {
    drawPage128X128Wether(u8g2, &WeatherResult, (WiFi.status() == WL_CONNECTED), (FwBoxIns.getServerStatus() == SERVER_STATUS_OK));

    u8g2->setFont(u8g2_font_unifont_t_chinese1);
    u8g2->setCursor(0, SMALL_ICON_BOTTOM - LINE_HEIGHT+3);
    u8g2->print("空氣品質");
    u8g2->setCursor(0, SMALL_ICON_BOTTOM);
    if(Pms.pm2_5() < 15)
      u8g2->print("良好");
    else if(Pms.pm2_5() >= 15 && Pms.pm2_5() < 35)
      u8g2->print("普通");
    else if(Pms.pm2_5() >= 35 && Pms.pm2_5() < 54)
      u8g2->print("不佳");
    else if(Pms.pm2_5() >= 54 && Pms.pm2_5() < 150)
      u8g2->print("糟糕");
    else
      u8g2->print("危害");
  }
  while (u8g2->nextPage());
}

void OledDisplayType3()
{
  u8g2->firstPage();
  do {
    drawPage128X128Time(u8g2, &WeatherResult, (WiFi.status() == WL_CONNECTED), (FwBoxIns.getServerStatus() == SERVER_STATUS_OK));

    u8g2->setFont(u8g2_font_unifont_t_chinese1);
    u8g2->setCursor(0, SMALL_ICON_BOTTOM - LINE_HEIGHT+3);
    u8g2->print("空氣品質");
    u8g2->setCursor(0, SMALL_ICON_BOTTOM);
    if(Pms.pm2_5() < 15)
      u8g2->print("良好");
    else if(Pms.pm2_5() >= 15 && Pms.pm2_5() < 35)
      u8g2->print("普通");
    else if(Pms.pm2_5() >= 35 && Pms.pm2_5() < 54)
      u8g2->print("不佳");
    else if(Pms.pm2_5() >= 54 && Pms.pm2_5() < 150)
      u8g2->print("糟糕");
    else
      u8g2->print("危害");
  }
  while (u8g2->nextPage());
}

void OledDisplayType4()
{
  u8g2->firstPage();
  do {
    drawPage128X128Info(u8g2, FIRMWARE_VERSION, (WiFi.status() == WL_CONNECTED), (FwBoxIns.getServerStatus() == SERVER_STATUS_OK));
  }
  while (u8g2->nextPage());
}

int getDisplayMode(int pageCount,int analogValue)
{
  int page_width = 1024 / pageCount;

  for (int i = 0; i < pageCount; i++) {
    if (i == 0) {
      if (analogValue < (page_width*(i+1))-ANALOG_VALUE_DEBOUNCING) { // The value - '5' is for debouncing.
        return i + 1;
      }
    }
    else if (i == (pageCount - 1)) {
      if (analogValue >= (page_width*i)+ANALOG_VALUE_DEBOUNCING) { // The value - '5' is for debouncing.
        return i + 1;
      }
    }
    else {
      if ((analogValue >= (page_width*i)+ANALOG_VALUE_DEBOUNCING) && (analogValue < (page_width*(i+1))-ANALOG_VALUE_DEBOUNCING)) { // The value - '5' is for debouncing.
        return i + 1;
      }
    }
  }

  return 1; // default page
}

void PrintLcdDigits(int digits)
{
  if (digits < 10)
    Lcd->print('0');
  Lcd->print(digits);
}
