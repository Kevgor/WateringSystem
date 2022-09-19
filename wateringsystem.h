#include <arduino.h>

#include <Wire.h>
#include <EEPROM.h>

#ifdef I2C_LCD
#include <LiquidCrystal_I2C.h>
#endif

#include <RTClib.h>

// Seven Setpoints per channel
// Four Channel(s)  BY-WW, BY-GH, FY-TM, FY-FY
const int NumberOfSetpoints = 8;
const int NumberOfChannels = 5;    

struct tagSettingData {
  byte Hours;  // 24 hour clock
  byte Minutes; // Minutes
  byte Seconds; //
  byte Duration; // duration in seconds max 255 s
  byte Channel; // channel
};

typedef struct tagSettingData SettingData;

// We store this whole data sructure in Arduino EEPROM
// 5 bytes (struct), x 5 bytes (channels), x 8 bytes daily settings
// this is 200 bytes

extern SettingData Settings[5][8];

struct tagToggleState {
  bool IsOn;
  DateTime timeToggledOn;
};

typedef struct tagToggleState ToggleState;

extern ToggleState ToggleStateInfo[5];

#ifdef I2C_LCD
extern LiquidCrystal_I2C lcd;
extern boolean IsLCDEnabled;
#endif

extern boolean bFreezeAnnouncements;

// PIN ASSIGNMENT DIFFERENT ON ESP32 than on Arduino UNO
const int voltagePin = A2;
// pin 39 on ESP32?

#ifdef RADIO
const int RFCommRXPin = 3;
#endif

const int ReadFromEepromPin = 7;
const int WriteToEepromPin = 6;

// 8,7 for ESP32?

extern float batteryVoltageRead();
