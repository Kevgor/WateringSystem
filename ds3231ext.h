#ifndef DS3231EXT_H
#define DS3231EXT_H

#include <RTClib.h>

class RTC_DS3231_Ext : public RTC_DS3231 {
public:
    bool isrunning();
    void start();
    void stop();
};

#endif