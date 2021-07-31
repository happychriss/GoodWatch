//
// Created by development on 29.06.21.
//

#include "Arduino.h"
#include "support.h"
#include "freertos/FreeRTOS.h"


void GetTimeNowString(int size, struct tm *timeinfo) {

    time_t now;
    char strftime_buf[64]={0};
    time(&now);
    localtime_r(&now, timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", timeinfo);
    DP("!!!!! Current Time: ");
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

