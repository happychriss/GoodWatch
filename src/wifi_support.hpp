
#include <WiFi.h>
#include "WiFiCredentials.h"
#include <lwip/apps/sntp.h>



void SetupWifi_SNTP() {
    DP("Starting Wifi");
    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID, PASSWORD);
    WiFi.setSleep(false);
    uint8_t connect_result;

    for (int i=0;i<10;i++) {
        connect_result=WiFi.waitForConnectResult() ;
        if (connect_result==WL_CONNECTED) break;
        DP(".");
        delay(200);
    }

    if (connect_result!=WL_CONNECTED) {
        Serial.println("Connection Failed! Rebooting...");
        delay(5000);
        ESP.restart();
    }

    DPL(": DONE");
    DPL("Connected to WiFi, now SNT Server Setup");

    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
    setenv("TZ", "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", 1);
    tzset();
    delay(200);
    DPL("SNTP Server Setup-Done ");

    struct tm timeinfo{};
    GetTimeNowString(64, &timeinfo);
    DPL("*** WIFI & SNTP Setup Done");

}