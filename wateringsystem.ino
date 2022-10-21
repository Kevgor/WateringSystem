#include <LiquidCrystal.h>

#include <Wire.h>
#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>

#include "wateringsystem.h"
#include "serialmonitor.h"

#include "radio.h"

boolean g_fShowDebugPrompt = true;
boolean g_fDebugOutput = false;

void ResetAllSettings();
void PrintLCDInfo(DateTime timenow);
void PrintInfoToSerial(DateTime timenow);
void PrintTimeInfoToSerial(DateTime timenow);
bool anyOtherToggles(int ch);
const SettingData & GetFactorySetting(int ch, int sp); 

RTC_DS1307 rtc;

DateTime now;

LiquidCrystal_I2C lcd(0x27,16,2);
boolean IsLCDEnabled = false;;

// This one is not const.
SettingData Settings[5][8];

// All off
byte settingsStatus[5][8] = {{0, 0, 0, 0, 0, 0, 0, 0 }, {0, 0, 0, 0, 0, 0, 0, 0 }, {0, 0, 0, 0, 0, 0, 0, 0 }, {0, 0, 0, 0, 0, 0, 0, 0 }, {0, 0, 0, 0, 0, 0, 0, 0 } };

bool disableMask[5] = {false, false, false,  false, false};
bool toggleStatus[5] = {false, false, false, false, false};
bool channelStatus[5] = {false, false, false, false, false};

ToggleState ToggleStateInfo[5];

bool bDailyReset = false;

bool bFreezeAnnouncements = false;
int announcePeriod = 20000;
unsigned long ticks_now = 0;

int announcePeriod2 = 5000;
unsigned long ticks_now2 = 0;

// Hour and Minute when Setup was run.
int starthour, startminute;

boolean WriteToEepromOnly = false;
boolean IsRTCRunning = false;

const unsigned outputPins[5] = {12, 11, 10, 9, 8};

void setup() {
  // Initialise RF Receiver, the IO and ISR
  // InitializeRadio();
    
  pinMode(ReadFromEepromPin, INPUT);
  pinMode(WriteToEepromPin, INPUT);

  Wire.begin();        // join i2c bus (address optional for master)
  Serial.begin(9600);  // start serial for output

  Serial.print(F("Hello Watering System Autumn\n"));

  if ( digitalRead(WriteToEepromPin) == 1 )
  {
    Serial.print(F("WriteToEepromPin  is HIGH\n"));
  }
  else 
  {
    Serial.print(F("WriteToEepromPin  is LOW\n"));
  }

  if ( digitalRead(ReadFromEepromPin) == 1 )
  {
    Serial.print(F("ReadFromEepromPin  is HIGH\n"));
  }
  else 
  {
    Serial.print(F("ReadFromEepromPin  is LOW\n"));
  }

  int eeadress = 0;

/*
  if ( digitalRead(WriteToEepromPin) == 1 )
  {
    WriteToEepromOnly = true;

    for (int sp = 0; sp < NumberOfSetpoints; sp++)
    {
      for (int ch = 0; ch < NumberOfChannels; ch++)
      {
        const SettingData fsetting = GetFactorySetting(sp,ch);
        EEPROM.put(eeadress , fsetting );
        eeadress += sizeof(SettingData);
      }
    }
  }

  eeadress = 0;
  if ( digitalRead(ReadFromEepromPin) == 1 )
  {
    for (int sp = 0; sp < NumberOfSetpoints; sp++)
    {
      for (int ch = 0; ch < NumberOfChannels; ch++)
      {
        EEPROM.get(eeadress , Settings[sp][ch]);
        eeadress += sizeof(SettingData);
      }
    }
  }

  
  if (digitalRead(WriteToEepromPin) == 0 && digitalRead(ReadFromEepromPin) == 0)
  {
    for (int sp = 0; sp < NumberOfSetpoints; sp++)
    {
      for (int ch = 0; ch < NumberOfChannels; ch++)
      {
        Settings[ch][sp] =  GetFactorySetting(ch,sp);
      }
    }
  }

  */

  for (int sp = 0; sp < NumberOfSetpoints; sp++)
  {
    for (int ch = 0; ch < NumberOfChannels; ch++)
    {
        Settings[ch][sp] =  GetFactorySetting(ch,sp);
    }
  }

  // Uncomment this line to run a test pattern  
  // CreateRunTestPattern();
  
  for (int ch = 0; ch < NumberOfChannels; ch++)
  {
    pinMode(outputPins[ch], OUTPUT);
    digitalWrite(outputPins[ch], LOW);
  }

  boolean rtcContinue = true; 
  if (!rtc.begin()) {
    Serial.println(F("Couldn't find RTC"));
    rtcContinue = false;
    // while (1);
  }
  
  if (!rtc.isrunning() ) {
    IsRTCRunning = false;
    Serial.println(F("RTC is NOT running!"));
  }
  else {
    IsRTCRunning = true;
    Serial.println(F("RTC is running!"));
  }
  
  // These 2 lines sets the RTC with an explicit date & time, for example to setup initial time
  // DateTime xnow(__DATE__, __TIME__);
  // rtc.adjust(xnow);

  // DateTime xnow(2022, 8, 22, 13, 19,0);
  // rtc.adjust(xnow);

  if(IsLCDEnabled) 
  {
    lcd.begin(16,2);
    lcd.backlight();
  }
  
  if(IsRTCRunning) {
    now = rtc.now();
    starthour = now.hour();
    startminute = now.minute();

    PrintTimeInfoToSerial(now);
    // PrintLCDInfo(now);
  }
  else 
  {
    DateTime now(__DATE__, __TIME__);
    PrintTimeInfoToSerial(now);
  }

//  if (WriteToEepromOnly)
//  {
//    PrintEEPROMSettingsInfoToSerial();
//  }
}


// char RFRxMessageBuffer[40];
// uint8_t RFRxBuflen = 40;

void loop() 
{
  if ( !IsRTCRunning )
  {
    // Nothing we can do...
    return;
  }
  
  now = rtc.now();
    
  //if (WriteToEepromOnly) {
  //  return;
  //}

  // If its 7 am lets reset all state and settings for a clean daily start.
//  if(now.hour() == 7 && now.minute() < 3 && !bDailyReset)
//  {
//    bDailyReset = true;
//    ResetAllSettings();
//  }

  SerialMonitor();

  // Get data from remote sensor
  // memset(RFRxMessageBuffer, '\0', RFRxBuflen);
  // if (vw_get_message((uint8_t *) RFRxMessageBuffer, &RFRxBuflen)) // Non-blocking
  // {
  //     // All good, check message for Rain Advisory  
  // }
  
  // Test for Sprinkling intervals
  for (int sp = 0; sp < NumberOfSetpoints; sp++)
  {
    for (int ch = 0; ch < NumberOfChannels; ch++)
    {
      // if disable mask for a channel is on, don't bother
      // turning on. 
      if (!disableMask[ch] && settingsStatus[ch][sp] == 0)
      {
        if ( Settings[ch][sp].Duration > 0 ) // Skip if duration = 0;
        {
          if ((now.hour() ==  Settings[ch][sp].Hours)
              && (now.minute() == Settings[ch][sp].Minutes)
              && (now.second() >= Settings[ch][sp].Seconds )
              && (now.second() <= Settings[ch][sp].Seconds + 2) )
          {
            settingsStatus[ch][sp] = 1;
            channelStatus[ch] = true;
            digitalWrite(outputPins[ch], HIGH);
          }
        }
      }
    }
  }

  for (int sp = 0; sp < NumberOfSetpoints; sp++)
  {
    for (int ch = 0; ch < NumberOfChannels; ch++)
    {
      if(disableMask[ch]) 
      {
        settingsStatus[ch][sp] = 0;
        channelStatus[ch] = false;
        digitalWrite(outputPins[ch], LOW);
      }
      else if (!disableMask[ch] && settingsStatus[ch][sp] == 1)
      {
        // Duration is in seconds, but for crossing over minute boundaries
        // we do a modulo thing
        int calcSecondsSetpoint = Settings[ch][sp].Seconds;
        int calcMinutesSetpoint = Settings[ch][sp].Minutes;
        int calcHoursSetpoint = Settings[ch][sp].Hours;

        // Duration can be at max 255 so something less than 5 minutes
        int durInMinutes = Settings[ch][sp].Duration / 60;
        int durSeconds = Settings[ch][sp].Duration % 60;

        int durHourOverflow = 0;
        if (calcMinutesSetpoint + durInMinutes > 60) {
          durHourOverflow = 1; // Increment the Hour
          durInMinutes = (calcMinutesSetpoint + durInMinutes) - 60;
          calcMinutesSetpoint = 0;
        }

        int durMinutesOverflow = 0;
        if (calcSecondsSetpoint + durSeconds > 60)
        {
          durMinutesOverflow = 1;
          calcSecondsSetpoint = (calcSecondsSetpoint + durSeconds) - 60;
          durSeconds = 0;
        }

        if ((now.hour() ==  calcHoursSetpoint + durHourOverflow)
            && (now.minute() >=  calcMinutesSetpoint + durInMinutes + durMinutesOverflow )
            && (now.second() >=  calcSecondsSetpoint + durSeconds ))
        {
          settingsStatus[ch][sp] = 0;
          channelStatus[ch] = false;
          digitalWrite(outputPins[ch], LOW);
        }
      }
    }
  }

  for (int ch = 0; ch < NumberOfChannels; ch++)
  { 
    // if we are disabled for this channel, do nothing
    if(!disableMask[ch])
    {
      // If channelstatus is true for a channel, the program has it on
      // so do not process toggles
      if (!channelStatus[ch])
      {
        if (toggleStatus[ch] /*&& !anyOtherToggles(ch)*/ )
        {
          digitalWrite(outputPins[ch], HIGH);
        }
        else if (!toggleStatus[ch])
        {
          digitalWrite(outputPins[ch], LOW);
        }
      }
    }
  }
  
  if (!bFreezeAnnouncements) 
  {
    if ( millis() > ticks_now + announcePeriod )
    {
      ticks_now = millis();
      PrintInfoToSerial(now);
      if(IsLCDEnabled) 
      {
        PrintLCDInfo(now);
      }
    }
  }
}

const char string_SUN[] PROGMEM = "Sunday"; 
const char string_M[] PROGMEM = "Monday";
const char string_T[] PROGMEM = "Tuesday";
const char string_W[] PROGMEM = "Wednesday";
const char string_TH[] PROGMEM = "Thursday";
const char string_F[] PROGMEM = "Friday";
const char string_SAT[] PROGMEM = "Saturday";

const char *const daysOfTheWeek[] PROGMEM = {string_SUN, string_M, string_T, string_W, string_TH, string_F, string_SAT};

void PrintTimeInfoToSerial(DateTime timenow)
{
  char dowBuffer[13];
  Serial.print(timenow.year(), DEC);
  Serial.print(F("/"));
  Serial.print(timenow.month(), DEC);
  Serial.print(F("/"));
  Serial.print(timenow.day(), DEC);
  Serial.print(F(" ( "));
  strcpy_P(dowBuffer, (char *)pgm_read_word(&(daysOfTheWeek[timenow.dayOfTheWeek()])));
  Serial.print(dowBuffer);
  Serial.print(F(" ) "));
  Serial.print(timenow.hour(), DEC);
  Serial.print(F(":"));
  if (timenow.minute() < 10)
    Serial.print(F("0"));
  Serial.print(timenow.minute(), DEC);
  Serial.print(F(":"));
  if (timenow.second() < 10)
    Serial.print(F("0"));
  Serial.print(timenow.second(), DEC);
  Serial.println();
}

const char string_CH0[] PROGMEM = "BY-BY"; 
const char string_CH1[] PROGMEM = "BY-GH";
const char string_CH2[] PROGMEM = "FY-TM";
const char string_CH3[] PROGMEM = "FY-FY";
const char string_CH4[] PROGMEM = "FY-FF";

const char *const channelNames[] PROGMEM = {string_CH0, string_CH1, string_CH2, string_CH3, string_CH4};

void PrintInfoToSerial(DateTime timenow)
{
  char chnameBuf[8];
    
  PrintTimeInfoToSerial(timenow);
  
  Serial.print(F("All Settings: "));
  Serial.println();

  for (int ch = 0; ch < NumberOfChannels; ch++)
  {
    strcpy_P(chnameBuf, (char *)pgm_read_word(&(channelNames[ch])));
    Serial.print(F("Channel: ")); Serial.print(ch); Serial.print(F(" : ")); Serial.print(chnameBuf); Serial.print(F(" : "));
    if(!disableMask[ch] ) 
    {
      for (int sp = 0; sp < NumberOfSetpoints; sp++)
      {
        Serial.print(settingsStatus[ch][sp]);
      }
    }
    else 
    {
      Serial.print(F("Disabled"));
    }
    
    Serial.println();
  }
}

/*

void SaveFactorySettingsToEeprom(){
  DateTime timenow = rtc.now();  
  PrintTimeInfoToSerial(timenow);

  Serial.println(F("Save Factory Settings into EEPROM:"));
}

void UploadFromEepromAndPrintLiveSettingsInfoToSerial(){
  DateTime timenow = rtc.now();  
  PrintTimeInfoToSerial(timenow);

  Serial.println(F("Upload Settings From EEPROM:"));
  
  int eeadress = 0;
  for (int sp = 0; sp < NumberOfSetpoints; sp++)
  {
    for (int ch = 0; ch < NumberOfChannels; ch++)
    {
      EEPROM.get(eeadress , Settings[ch][sp]);
      eeadress += sizeof(SettingData);
    }
  }

  // Now write something out
  for (int ch = 0; ch < NumberOfChannels; ch++)
  {
    Serial.print(F("Channel: "));
    Serial.println(ch);
    Serial.println();
    Serial.flush();
    delay(10);

    for (int sp = 0; sp < NumberOfSetpoints; sp++)
    {
      Serial.print(F("Setting #: "));
      Serial.print(sp);
      Serial.print(F(" Data is: "));
     
      Serial.print((int) Settings[ch][sp].Hours);
      Serial.print(F(" "));
      Serial.print((int) Settings[ch][sp].Minutes);
      Serial.print(F(" "));
      Serial.print((int) Settings[ch][sp].Seconds);
      Serial.print(F(" duration:  "));
      Serial.print((int) Settings[ch][sp].Duration);
      Serial.println();
      Serial.flush();    
      delay(10);
    }
  }
}

void PrintEEPROMSettingsInfoToSerial()
{
  DateTime timenow = rtc.now();  
  PrintTimeInfoToSerial(timenow);

  Serial.println(F("All Settings From EEPROM:"));
  
  SettingData EEPROMSettings[5][8] PROGMEM;

  int eeadress = 0;
  for (int sp = 0; sp < NumberOfSetpoints; sp++)
  {
    for (int ch = 0; ch < NumberOfChannels; ch++)
    {
      EEPROM.get(eeadress , EEPROMSettings[ch][sp]);
      eeadress += sizeof(SettingData);
    }
  }

  // Now write something out
  for (int ch = 0; ch < NumberOfChannels; ch++)
  {
    Serial.print(F("Channel: "));
    Serial.println(ch);
    Serial.println();
    Serial.flush();
    delay(10);

    for (int sp = 0; sp < NumberOfSetpoints; sp++)
    {
      Serial.print(F("Setting #: "));
      Serial.print(sp);
      Serial.print(F(" Data is: "));
     
      Serial.print((int) EEPROMSettings[ch][sp].Hours);
      Serial.print(F(" "));
      Serial.print((int) EEPROMSettings[ch][sp].Minutes);
      Serial.print(F(" "));
      Serial.print((int) EEPROMSettings[ch][sp].Seconds);
      Serial.print(F(" duration:  "));
      Serial.print((int) EEPROMSettings[ch][sp].Duration);
      Serial.println();
      Serial.flush();    
      delay(10);
    }
  }
}

*/

void PrintLiveSettingsInfoToSerial()
{
  // DateTime timenow = rtc.now();
  PrintTimeInfoToSerial(now);

  Serial.print(F("All Live Settings: "));
  Serial.println();
  Serial.flush();
  delay(10);

  // Now write something out
  for (int ch = 0; ch < NumberOfChannels; ch++)
  {
    Serial.print(F("Channel: "));
    Serial.print(ch);
    Serial.println();
    Serial.flush();
    delay(10);
    
    for (int sp = 0; sp < NumberOfSetpoints; sp++)
    {
      Serial.print(F("Setting #: "));
      Serial.print(sp);
      Serial.print(F(" Data is: "));
      Serial.flush();
     
      Serial.print((int) Settings[ch][sp].Hours);
      Serial.print(F(" "));
      Serial.print((int) Settings[ch][sp].Minutes);
      Serial.print(F(" "));
      Serial.print((int) Settings[ch][sp].Seconds);
      Serial.print(F(" duration:  "));
      Serial.print((int) Settings[ch][sp].Duration);
      Serial.println();
      Serial.flush();    
      delay(10);
    }
  }
}


void PrintDebugStatusInfoToSerial()
{
  // DateTime timenow = rtc.now();
  PrintTimeInfoToSerial(now);

  Serial.print(F("DEBUG All Status: "));
  Serial.println();
  delay(10);

  // Now write something out
  for (int ch = 0; ch < NumberOfChannels; ch++)
  {
    Serial.print("Channel: ");
    Serial.print(ch);
    
    Serial.print(F(" "));
    Serial.print(F("ChannelStatus: "));
    Serial.print(channelStatus[ch]);

    Serial.print(F(" "));
    Serial.print(F("Toggle Status: "));
    Serial.print(toggleStatus[ch]);

    Serial.print(F(" "));
    Serial.print(F("DisableMask Status: "));
    Serial.print(disableMask[ch]);

    Serial.print(F(" "));
    Serial.print(F("SettingsStatus Array: "));
    for (int sp = 0; sp < NumberOfSetpoints; sp++)
    {
        Serial.print(settingsStatus[ch][sp]);
    }

    Serial.println();
    Serial.flush();    
    delay(10);
  }

  Serial.print(F("FreezeAnnouncements: "));
  Serial.println(bFreezeAnnouncements);

  Serial.println(F("END DEBUG"));
  Serial.flush();    
  delay(10);
}

bool anyOtherToggles(int ch) {
  bool anyOtherToggles = false;
  for (int i = 0; i < NumberOfChannels; i++) {
    if (i == ch) {
      continue;
    }
    anyOtherToggles |= toggleStatus[i];
  }
  return anyOtherToggles;
}

void ResetAllSettings() 
{
  for (int ch = 0; ch < NumberOfChannels; ch++)
  {
    pinMode(outputPins[ch], OUTPUT);
    digitalWrite(outputPins[ch], LOW);

    toggleStatus[ch] = false;
    channelStatus[ch] = false;
    disableMask[ch] = false;

    for (int sp = 0; sp < NumberOfSetpoints; sp++)
    {
     settingsStatus[ch][sp] = false;
    }
  }
}

void PrintLCDInfo(DateTime timenow)
{
    lcd.setCursor(0,0);
    lcd.print(F("                "));
    lcd.setCursor(0,0);
    lcd.print(F("DATE: "));
    lcd.print(timenow.year(), DEC);
    lcd.print('/');
    if(timenow.month() < 10)
      lcd.print('0');
    lcd.print(timenow.month(), DEC);
    lcd.print('/');
    if(timenow.day() < 10)
      lcd.print('0');
    lcd.print(timenow.day(), DEC);
    lcd.setCursor(0,1);
    lcd.print(F("TIME: "));
    if(timenow.hour() < 10)
      lcd.print('0');
    lcd.print(timenow.hour(), DEC);
    lcd.print(':');
    if(timenow.minute() <10)
      lcd.print('0');
    lcd.print(timenow.minute(), DEC);
    lcd.print(':');
    if(timenow.second() <10)
      lcd.print('0');
    lcd.print( timenow.second(), DEC);
    lcd.setCursor(0,0);
}

// const SettingData factorySettings[5][8]
// = {
//   { {9, 15, 0, 120, 0}, {11, 05, 0, 140, 0}, {13, 10, 0, 140, 0}, {14, 30, 0, 160, 0}, {16, 20, 0, 140, 0}, {17, 30, 0, 140, 0},{18, 35, 0, 120, 0}, {19, 35, 0, 0, 0} },
//   { {9,  0, 0, 120, 1}, {10, 42, 0, 140, 1}, {12, 00, 0, 140, 1},  {13, 45, 0, 160, 1}, {14, 45, 0, 140, 1}, {16, 10, 0, 140, 1}, {18, 15, 0, 120, 1}, {19, 15, 0, 0, 1}  },
//   { {9, 05, 0, 120, 2 }, {10, 48, 0, 160, 2}, {12, 05, 0, 160, 2},  {13, 55, 0, 180, 2},  {15, 00, 0, 180, 2}, {16, 25, 0, 180, 2}, {18, 20, 0, 140, 2}, {19, 20, 0, 0, 2}  },
//   { {8, 45, 0, 120, 3 }, {10, 15, 0, 140, 3}, {12, 20, 0, 160, 3}, {14, 10, 0, 160, 3}, {15, 35, 0, 160, 3},  {16, 45, 0, 140, 3}, {18, 00, 0, 120, 3}, {19, 00, 0, 0, 3} },
//   { {8, 55, 0, 120, 4 }, {10, 30, 0, 140, 4}, {12, 30, 0, 140, 4}, {14, 25, 0, 160, 4}, {15, 55, 0, 140, 4},  {16, 55, 0, 140, 4}, {18, 55, 0, 120, 4}, {19, 55, 0, 0, 4} }
// };

const SettingData autumnFactorySettings[5][8]
= {
  { {9, 15, 0, 0, 0}, {11, 05, 0, 120, 0}, {13, 10, 0, 120, 0}, {14, 30, 0, 120, 0}, {16, 20, 0, 0, 0}, {17, 30, 0, 0, 0},{18, 35, 0, 0, 0}, {19, 35, 0, 0, 0} },
  { {9,  0, 0, 0, 1}, {10, 42, 0, 120, 1}, {12, 00, 0, 120, 1},  {13, 45, 0, 120, 1}, {14, 45, 0, 60, 1}, {16, 10, 0, 0, 1}, {18, 15, 0, 0, 1}, {19, 15, 0, 0, 1}  },
  { {9, 05, 0, 0, 2 }, {10, 48, 0, 120, 2}, {12, 05, 0, 120, 2},  {13, 55, 0, 120, 2},  {15, 00, 0, 0, 2}, {16, 25, 0, 0, 2}, {18, 20, 0, 0, 2}, {19, 20, 0, 0, 2}  },
  { {8, 45, 0, 0, 3 }, {10, 15, 0, 120, 3}, {12, 20, 0, 120, 3}, {14, 10, 0, 120, 3}, {15, 35, 0, 0, 3},  {16, 45, 0, 0, 3}, {18, 00, 0, 0, 3}, {19, 00, 0, 0, 3} },
  { {8, 55, 0, 0, 4 }, {10, 30, 0, 120, 4}, {12, 30, 0, 120, 4}, {14, 25, 0, 120, 4}, {15, 55, 0, 0, 4},  {16, 55, 0, 0, 4}, {18, 55, 0, 0, 4}, {19, 55, 0, 0, 4} }
};

const SettingData & GetFactorySetting(int ch, int sp) 
{
  return autumnFactorySettings[ch][sp];
} 

void CreateRunTestPattern() 
{
  DateTime timenow = rtc.now();

  byte startHour = timenow.hour();
  byte startMinute = timenow.minute() + 1;
  byte startSecond = 0;
  const byte duration = 5;

  for (int sp = 0; sp < NumberOfSetpoints; sp++)
  {
    for (int ch = 0; ch < NumberOfChannels; ch++)
    {
      Settings[ch][sp].Hours = startHour;
      Settings[ch][sp].Minutes = startMinute;
      Settings[ch][sp].Seconds = startSecond + ch * (duration + 5);
      Settings[ch][sp].Duration = duration;
      Settings[ch][sp].Channel = ch;

      settingsStatus[ch][sp] = 0;
    }
    startMinute++;
  }

  for (int ch = 0; ch < NumberOfChannels; ch++)
  {
    toggleStatus[ch] = 0;
    channelStatus[ch] = 0;
      
    digitalWrite(outputPins[ch], LOW);
  }

  announcePeriod = 3000;
}
