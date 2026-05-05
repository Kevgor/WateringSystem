#include <arduino.h>
#include <RTClib.h>
#include <EEPROM.h>
#include "ds3231ext.h"

#include "version.h"

// #define I2C_LCD
#ifdef I2C_LCD
#include <LiquidCrystal_I2C.h>
#endif

// Seven Setpoints per channel
// Four Channel(s)  BY-WW, BY-GH, FY-TM, FY-FY
const uint8_t NumberOfSetpoints = 8;
const uint8_t NumberOfChannels = 7;    

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

extern SettingData Settings[NumberOfChannels][NumberOfSetpoints];

struct tagToggleState {
  bool IsOn;
  //DateTime timeToggledOn;
};

typedef struct tagToggleState ToggleState;

extern ToggleState ToggleStateInfo[NumberOfChannels];

extern unsigned long ticks_maxPeriod;

#ifdef I2C_LCD
extern LiquidCrystal_I2C lcd;
extern boolean IsLCDEnabled;
#endif

extern boolean bFreezeAnnouncements;

// PIN ASSIGNMENTS
// 
const uint8_t voltagePin  = A2;
const uint8_t  RFCommRXPin   = 3;
const uint8_t  ReadFromEepromPin  = 7;
const uint8_t  WriteToEepromPin  = 6;
const uint8_t  outputPins[NumberOfChannels] = {12, 11, 10, 9, 8, 5, 4};
