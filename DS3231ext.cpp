#include "ds3231ext.h"
#include <Wire.h>

bool RTC_DS3231_Ext::isrunning() {
    Wire.beginTransmission(0x68);
    Wire.write(0x00);
    Wire.endTransmission();

    Wire.requestFrom(0x68, 1);
    uint8_t sec = Wire.read();
    return !(sec & 0x80);
}

// void RTC_DS3231_Ext::start() {
//     Wire.beginTransmission(0x68);
//     Wire.write(0x00);
//     Wire.endTransmission();

//     Wire.requestFrom(0x68, 1);
//     uint8_t sec = Wire.read() & 0x7F;

//     Wire.beginTransmission(0x68);
//     Wire.write(0x00);
//     Wire.write(sec);
//     Wire.endTransmission();
// }

// void RTC_DS3231_Ext::stop() {
//     Wire.beginTransmission(0x68);
//     Wire.write(0x00);
//     Wire.endTransmission();

//     Wire.requestFrom(0x68, 1);
//     uint8_t sec = Wire.read() | 0x80;

//     Wire.beginTransmission(0x68);
//     Wire.write(0x00);
//     Wire.write(sec);
//     Wire.endTransmission();
// }
