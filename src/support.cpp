//
// Created by development on 29.06.21.
//

#include "Arduino.h"
#include "support.h"
#include "freertos/FreeRTOS.h"


void GetTimeNowString(char  strftime_buf[], int size) {

    time_t now;

    struct tm timeinfo;

    time(&now);
// Set timezone to China Standard Time
    setenv("TZ", "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", 1);
    tzset();

    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, size, "%c", &timeinfo);
    DP("Current Time:");
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
            Serial.println("Wakeup caused by external signal using RTC_IO");
            break;
        case ESP_SLEEP_WAKEUP_EXT1 :
            Serial.println("Wakeup caused by external signal using RTC_CNTL");
            break;
        case ESP_SLEEP_WAKEUP_TIMER :
            Serial.println("Wakeup caused by timer");
            break;
        case ESP_SLEEP_WAKEUP_TOUCHPAD :
            Serial.println("Wakeup caused by touchpad");
            break;
        case ESP_SLEEP_WAKEUP_ULP :
            Serial.println("Wakeup caused by ULP program");
            break;
        default :
            Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
            break;
    }
    return wakeup_reason;
}