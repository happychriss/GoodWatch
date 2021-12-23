//
// Created by development on 17.10.21.
//

#include "rtc_support.h"
#include <Arduino.h>
#include <sys/time.h>
#include "support.h"
#include <global_hardware.h>

#include <my_RTClib.h>
#include <global_display.h>

extern RTC_DS3231 rtc_watch;

String DateTimeString(DateTime dt) {
    char str_format[]="hh:mm:ss - DDD, DD.MM.YYYY";
    String str_time_now=dt.toString(str_format);
    return str_time_now;
}





/*| specifier | output                                                 |
|-----------|--------------------------------------------------------|
| YYYY      | the year as a 4-digit number (2000--2099)              |
| YY        | the year as a 2-digit number (00--99)                  |
| MM        | the month as a 2-digit number (01--12)                 |
| MMM       | the abbreviated English month name ("Jan"--"Dec")      |
| DD        | the day as a 2-digit number (01--31)                   |
| DDD       | the abbreviated English day of the week ("Mon"--"Sun") |
| AP        | either "AM" or "PM"                                    |
| ap        | either "am" or "pm"                                    |
| hh        | the hour as a 2-digit number (00--23 or 01--12)        |
| mm        | the minute as a 2-digit number (00--59)                |
| ss        | the second as a 2-digit number (00--59)                |*/


void espPrintTimeNow() {
    struct tm timeinfo={};
    time_t now;
    time(&now);
    localtime_r(&now, &timeinfo);
    DP("Current *local* Time from Internet - ESP32: ");
    char strftime_buf[30]={};
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    DPL(strftime_buf);

}

void rtcInit() {
    // initializing the rtc
    if(!rtc_watch.begin()) {
        Serial.println("Couldn't find RTC!");
        Serial.flush();
        abort();
    }

    // stop oscillating signals at SQW Pin
    // otherwise setAlarm1 will fail
    rtc_watch.writeSqwPinMode(DS3231_OFF);
    rtc_watch.disable32K();

    DPL("RTC Init: OK");
}

void rtcSetRTCFromInternet() {
    struct tm timeinfo={};
    time_t now;
    char strftime_buf[30]={};
    time(&now);
    localtime_r(&now, &timeinfo);
    DP("Internet Time: ");
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    DPL(strftime_buf);
    rtc_watch.adjust(DateTime(timeinfo.tm_year+1900,timeinfo.tm_mon+1,timeinfo.tm_mday,timeinfo.tm_hour,timeinfo.tm_min, timeinfo.tm_sec));

}

void rtsSetEspTime(DateTime dt) {
    timeval tv;
    tv.tv_sec = dt.unixtime();
    tv.tv_usec = 0;  // keine Mikrosekunden setzen
    settimeofday(&tv, NULL);
}


/**************************************************************************/
/*!
    @brief  Get the current date/time
    @return DateTime object with the current date/time
*/
/**************************************************************************/
static uint8_t bcd2bin(uint8_t val) { return val - 6 * (val >> 4); }

