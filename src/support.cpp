//
// Created by development on 29.06.21.
//

#include "Arduino.h"
#include "support.h"
#include "freertos/FreeRTOS.h"
#include <RTClib.h>
#include <sys/time.h>

extern RTC_DS1307 rtc_watch;


void GetTimeNowString(struct tm *timeinfo, boolean source_rtc) {
    time_t now;
    char strftime_buf[30]={};
    // Take time from ESP32
    if (!source_rtc) {
        time(&now);
        DP("Current *local* Time from Internet - ESP32: ");
        localtime_r(&now, timeinfo);
    }
    else {
        // Watch stores local time
        DateTime rtc_now = rtc_watch.now();
        timeval tv;
        tv.tv_sec = rtc_now.unixtime();
        tv.tv_usec = 0;  // keine Mikrosekunden setzen
        settimeofday(&tv, NULL);
        timeinfo->tm_hour=rtc_now.hour();
        timeinfo->tm_min=rtc_now.minute();
        timeinfo->tm_sec=rtc_now.second();
        timeinfo->tm_mday=rtc_now.day();
        timeinfo->tm_year=rtc_now.year();
        DP("Current *local* Time from RTC: ");
    }

    strftime(strftime_buf, sizeof(strftime_buf), "%c", timeinfo);
    DPL(strftime_buf);


}

int SerialKeyWait() {// Wait for Key
//    Serial.setDebugOutput(true);

    Serial.println("Hit a key to start...");
    Serial.flush();

    while (true) {
        int inbyte = Serial.available();
        delay(500);
        if (inbyte > 0) break;
    }

    return Serial.read();

}

esp_sleep_wakeup_cause_t print_wakeup_reason() {
    esp_sleep_wakeup_cause_t wakeup_reason;

    wakeup_reason = esp_sleep_get_wakeup_cause();

    switch (wakeup_reason) {
        case ESP_SLEEP_WAKEUP_EXT0 :
            DPL("Wakeup caused by external signal using RTC_IO");
            break;
        case ESP_SLEEP_WAKEUP_EXT1 :
            DPL("Wakeup caused by external signal using RTC_CNTL");
            break;
        case ESP_SLEEP_WAKEUP_TIMER :
            DPL("Wakeup caused by timer");
            break;
        case ESP_SLEEP_WAKEUP_TOUCHPAD :
            DPL("Wakeup caused by touchpad");
            break;
        case ESP_SLEEP_WAKEUP_ULP :
            DPL("Wakeup caused by ULP program");
            break;
        default :
            DPL("Normal Boot Process");
            break;
    }
    return wakeup_reason;
}

