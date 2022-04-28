#include <arduino.h>

#include <Wire.h>
#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>
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

extern LiquidCrystal_I2C lcd;
extern boolean IsLCDEnabled;

extern boolean bFreezeAnnouncements;

const int voltagePin = A2;

const int RFCommRXPin = 3;

const int ReadFromEepromPin = 7;
const int WriteToEepromPin = 6;
