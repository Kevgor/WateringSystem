#include <arduino.h>
#include <RTClib.h>
#include <EEPROM.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#include "wateringsystem.h"
#include "SerialMonitor.h"

//==============================================================================
// Serial/BlueTooth TerminalMonitor - Simple background task checks to see if
// the user is asking us to do anything, like various commands, update debug
// levels and the like.
//==============================================================================

// extern void PrintEEPROMSettingsInfoToSerial();
// extern void SaveFactorySettingsToEeprom();
extern void PrintDebugStatusInfoToSerial();
extern void PrintLiveSettingsInfoToSerial();
// extern void UploadFromEepromAndPrintLiveSettingsInfoToSerial();
extern void CreateRunTestPattern();
extern float voltageRead();
extern float solarVoltageRead();

#ifdef DHT
extern float readDHTSensor();
#endif 

void recvWithEndMarker();
void UpdateSetPoint(const char* receivedChars);
void UpdateTime(const char* receivedChars);

boolean newData = false;

const byte numChars = 20;
char receivedChars[numChars];   // an array to store the received data

void SerialMonitor(void)
{
  const char szBufHeader[] PROGMEM  = "Arduino Serial Monitor  - Make your Choice";

  const char szTHeader[] PROGMEM = "T - Toggle Channel [1-4] ON/OFF";
  const char szListHeader[] PROGMEM = "? - Show this List";
  const char szVHeader[] PROGMEM = "V - Read Input Supply Voltage";
  const char szCHeader[] PROGMEM = "C - Read Temperature Sensor";
  const char szRHeader[] PROGMEM = "R - Report Status";
  const char szPHeader[] PROGMEM = "P - Process Program  (PE, PL, PT, PUL)";
  const char szDHeader[] PROGMEM = "D - Disable  Channel [1-4] ON/OFF";
  const char szFHeader[] PROGMEM = "F - Freeze(Toggle) Announcements";
  const char szSHeader[] PROGMEM = "S - Change Setpoint";
  const char szZHeader[] PROGMEM = "Z - Change Time";
  const char szWHeader[] PROGMEM = "W - Write FactorySettings into EEPROM";
  const char szStartStopHeader[] PROGMEM = "! - Start/Stop Controller";

  if (g_fShowDebugPrompt) {
    // See if we need to output a prompt.
    Serial.println(szBufHeader);
    //Serial.println("D - Toggle debug on or off");
    Serial.println(szCHeader);
    Serial.println(szRHeader);
    Serial.println(szVHeader);
    Serial.println(szSHeader);
    Serial.println(szTHeader);
    Serial.println(szPHeader);
    Serial.println(szDHeader);
    Serial.println(szFHeader);
    Serial.println(szWHeader);
    Serial.println(szListHeader);
    Serial.println(szStartStopHeader);
    Serial.flush();
    g_fShowDebugPrompt = false;
  }

  // First check to see if there is any characters to process.
  recvWithEndMarker();
  if (newData)
  {
    //Serial.print("Serial Cmd Line: [");
    //Serial.print(String(receivedChars));
    //Serial.println("]");

    int ich = strlen(receivedChars);

    // So see what are command is.
    if (ich == 0)
    {
      // g_fShowDebugPrompt = true;
    }
    else if ((ich == 1) && ((receivedChars[0] == 'p') || (receivedChars[0] == 'P')))
    {
      // Just a plain 'P' will show the lives settings
      PrintLiveSettingsInfoToSerial();
      g_fDebugOutput = false;
    }
    else if ((ich == 2) && ((receivedChars[0] == 'p') || (receivedChars[0] == 'P')))
    {
      if ( (receivedChars[1] == 'e') || (receivedChars[1] == 'E') ) {
        //  PrintEEPROMSettingsInfoToSerial();
      }
      else if ( (receivedChars[1] == 'l') || (receivedChars[1] == 'L') )
      {
        PrintLiveSettingsInfoToSerial();
      }
      else if ( (receivedChars[1] == 't') || (receivedChars[1] == 'T') )
      {
        CreateRunTestPattern();
      }
      g_fDebugOutput = !g_fDebugOutput;
    }
    else if ((ich == 3) && ((receivedChars[0] == 'p') || (receivedChars[0] == 'P')))
    {
      if ( (receivedChars[1] == 'u') || (receivedChars[1] == 'U') ) {
        if ( (receivedChars[2] == 'l') || (receivedChars[2] == 'L') )
        {
          // UploadFromEepromAndPrintLiveSettingsInfoToSerial();
        }
      }
    }
    else if ((ich == 2) && ((receivedChars[0] == 't') || (receivedChars[0] == 'T')))
    {
      char nbuf[2];
      nbuf[0] = receivedChars[1];
      nbuf[1] = '\0';
      int ch = atoi(nbuf);
      if(ch > NumberOfChannels-1)
      {
        Serial.print(F("There are only "));
        Serial.print(NumberOfChannels);
        Serial.print(F(" Channels"));
        Serial.println();
      }
      else
      {
        // TODO really should only allow one channel to be toggled at
        // any time.
        // TODO maybe toggles should automatically turn off after
        // some predetermined time, like 5 minutes (so watering on a toggle
        // isn't forgotten, and goes on all day.)
        toggleStatus[ch] = !toggleStatus[ch];

        Serial.print(F("Toggle Channel: "));
        Serial.print(ch);
        Serial.println();
      }
      g_fDebugOutput = !g_fDebugOutput;
    }
    else if ((ich == 1) && ((receivedChars[0] == 't') || (receivedChars[0] == 'T')))
    {
      // Shortcut: toggle the last toggled channel
      // if no toggled channel, do nothing
      int channelToggled = -1;
      for(int i = 0; i < NumberOfChannels; i++)
      {
        if(toggleStatus[i]) {
          toggleStatus[i] = !toggleStatus[i];
          channelToggled = i;
          break;
        }
      }
      if(channelToggled >= 0)
      {
        Serial.print(F("Toggle Channel: "));
        Serial.print(channelToggled);
        Serial.println();
      }
      else {
        Serial.print(F("Nothing to Toggle"));
        Serial.println();
      }
      g_fDebugOutput = false;
    }
    else if ((ich == 2) && ((receivedChars[0] == 'd') || (receivedChars[0] == 'D')))
    {
      char nbuf[2];
      nbuf[0] = receivedChars[1];
      nbuf[1] = '\0';
      int ch = atoi(nbuf);
      if(ch > NumberOfChannels-1)
      {
        Serial.print(F("There are only "));
        Serial.print(NumberOfChannels);
        Serial.print(F(" Channels"));
        Serial.println();
      }
      else
      {
        disableMask[ch] = 1;

        Serial.print(F("Disable Channel: "));
        Serial.print(ch);
        Serial.println();
      }
      g_fDebugOutput = !g_fDebugOutput;
    }
    else if ((ich == 1) && ((receivedChars[0] == 'd') || (receivedChars[0] == 'D')))
    {
      // Just a plain 'D' will re-enable all channels
      int d_idx = 0;
      while(d_idx < NumberOfChannels) disableMask[d_idx++] = 0;

      Serial.print(F("All Channels Enabled"));
      Serial.println();

      g_fDebugOutput = false;
    }
    else if ((ich == 1) && (receivedChars[0] == '!'))
    {
      Serial.println(F("Turning System Off (Toggle)"));
      if(IsLCDEnabled)
      {
        lcd.noBacklight();
      }

      g_fShowDebugPrompt = false;
    }
    else if ((ich == 1) && ((receivedChars[0] == 'r') || (receivedChars[0] == 'R')))
    {
      PrintDebugStatusInfoToSerial();
      g_fShowDebugPrompt = false;
    }
    else if ((ich == 1) && ((receivedChars[0] == 'w') || (receivedChars[0] == 'W')))
    {
      // SaveFactorySettingsToEeprom();
      g_fDebugOutput = !g_fDebugOutput;
    }
    else if ((ich == 1) && ((receivedChars[0] == 'f') || (receivedChars[0] == 'F')))
    {
      g_fShowDebugPrompt = false;
      Serial.println(F("Freeze Announcements (toggle)"));
      bFreezeAnnouncements = !bFreezeAnnouncements;
    }
    else if ( ((receivedChars[0] == 's') || (receivedChars[0] == 'S')))
    {
      g_fShowDebugPrompt = false;
      Serial.println(F("[Change Setpoint]"));
      UpdateSetPoint(receivedChars);
    }
    else if ((ich == 1) && ((receivedChars[0] == 'm') || (receivedChars[0] == 'M')))
    {
      g_fShowDebugPrompt = false;  // Do not redraw all the options
    }
    else if ((ich == 1) && ((receivedChars[0] == 'v') || (receivedChars[0] == 'V')))
    {
      g_fShowDebugPrompt = false;  // Do not redraw all the options
      Serial.print(F("Read Input Voltage: "));
      float voltage =  voltageRead();
      Serial.print(voltage);
      Serial.println(F(" V"));

      if(IsLCDEnabled)
      {
        lcd.setCursor(0,0);
        lcd.print(F("Voltage: "));
        lcd.print(voltage);
        lcd.print(F(" V"));
      }
    }
    else if ((ich == 1) && ((receivedChars[0] == 'c') || (receivedChars[0] == 'C')))
    {
      g_fShowDebugPrompt = false;  // Do not redraw all the options

      #ifdef DHT
      Serial.print(F("Read Temperature: "));
      float temperature = readDHTSensor();
      Serial.print(temperature);
      Serial.println(F(" C"));
      #else 
      Serial.print(F("Temperature Sensor Not Installed"));
      #endif
    }
    else if ((ich == 1) && ((receivedChars[0] == 'b') || (receivedChars[0] == 'B')))
    {
      g_fShowDebugPrompt = false;  // Do not redraw all the options
      // Serial.println("Read Battery Voltage");
    }
    else if ((ich == 1) && ((receivedChars[0] == 'l') || (receivedChars[0] == 'L')))
    {
      g_fShowDebugPrompt = false;  // Do not redraw all the options
    }
    else if ( ((receivedChars[0] == 'z') || (receivedChars[0] == 'Z')))
    {
      g_fShowDebugPrompt = false;  // Do not redraw all the options
      Serial.println(F("[Change Time]"));
      UpdateTime(receivedChars);
    }
    else if ((ich == 1) && ((receivedChars[0] == '?') ))
    {
      g_fShowDebugPrompt = true;
    }
    newData = false;
  }
}

void recvWithEndMarker()
{
  static byte ndx = 0;
  char endMarker = '\n';
  char rc;

  while (Serial.available() > 0 && newData == false)
  {
    rc = Serial.read();

    if (rc != endMarker)
    {
      receivedChars[ndx] = rc;
      ndx++;
      if (ndx >= numChars)
      {
        ndx = numChars - 1;
      }
    }
    else
    {
      receivedChars[ndx] = '\0'; // terminate the string
      ndx = 0;
      newData = true;
    }
  }
}

// Update a setpoint and store to EEPROM
void UpdateSetPoint(const char* receivedChars)
{
  char szchspBuf[5] PROGMEM;
  char szTimeBuf[10] PROGMEM;
  char szDurationBuf[5] PROGMEM;
  byte ch, sp, duration;
  byte thehour, theminute, thesec;

  Serial.println(receivedChars);
  // Example: S3&0#8:39:00?80 Channel 3,  Setpoint 0, Time 08:39:00, Duration(secs) 80
  char* chspTok = strtok((char*)receivedChars, "#");
  char* chspptr = chspTok;
  int offset = strlen(chspTok); // offset should point at '#'
  strcpy(szchspBuf, ++chspptr);
  char* ptr = strtok(szchspBuf, "&");
  ch = atoi(ptr);
  ptr = &szchspBuf[2];
  sp = atoi(ptr);
  ptr = (char*) &receivedChars[offset + 1];
  char* tptr = strtok(ptr, "?");
  strcpy(szTimeBuf, tptr);
  char* dptr = strtok(NULL, "");
  duration = atoi(dptr);
  char* tmptr = strtok(szTimeBuf, ":");
  thehour = atoi(tmptr);
  tmptr = strtok(NULL, ":");
  theminute = atoi(tmptr);
  tmptr = strtok(NULL, ":");
  thesec = atoi(tmptr);

  //  Serial.print("Channel: ");
  //  Serial.println(ch);
  //  Serial.print("Setpoint: ");
  //  Serial.println(sp);
  //  Serial.print("Hours: ");
  //  Serial.println(thehour);
  //  Serial.print("Minutes: ");
  //  Serial.println(theminute);
  //  Serial.print("Seconds: ");
  //  Serial.println(thesec);
  //  Serial.print("Duration: ");
  //  Serial.println(duration);
  //  Serial.flush();
  //  delay(10);

  Settings[ch][sp].Hours = thehour;
  Settings[ch][sp].Minutes = theminute;
  Settings[ch][sp].Seconds = thesec;
  Settings[ch][sp].Duration = duration;
  Settings[ch][sp].Channel = ch;

  // also write to EEPROM
  int eeadress = 0;
  for (int isp = 0; isp < NumberOfSetpoints; isp++)
  {
    for (int ich = 0; ich < NumberOfChannels; ich++)
    {
      if (ich == ch  && isp == sp) {
        EEPROM.put(eeadress , Settings[ch][sp] );
      }
      eeadress += sizeof(SettingData);
    }
  }
}

// Update time to RTC
//
void UpdateTime(const char* receivedChars){
  char szchspBuf[5] PROGMEM;
  char szTimeBuf[10] PROGMEM;
  char szDurationBuf[5] PROGMEM;
  byte ch, sp, duration;
  byte thehour, theminute, thesec;

  Serial.println(receivedChars);
  // Example: S#8:39:00   - Time 08:39:00 to be written to RTC
  char* tsTok = strtok((char*)receivedChars, "#");
  char* tsptr = tsTok;
  int offset = strlen(tsTok); // offset should point at '#'
  strcpy(szTimeBuf, ++tsptr);
  char* tmptr = strtok(szTimeBuf, ":");
  thehour = atoi(tmptr);
  tmptr = strtok(NULL, ":");
  theminute = atoi(tmptr);
  tmptr = strtok(NULL, ":");
  thesec = atoi(tmptr);

  Serial.print("Hours: ");
  Serial.println(thehour);
  Serial.print("Minutes: ");
  Serial.println(theminute);
  Serial.print("Seconds: ");
  Serial.println(thesec);
  Serial.flush();
  delay(10);


}

