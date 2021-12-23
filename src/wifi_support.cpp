//
// Created by development on 05.11.21.
//


#include "wifi_support.h"


// Wifi and Connectivity
// Wifi and Connectivity



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
    delay(500);

    DPL("Wifi Details:");
    DP("Wifi DNS: ");
    DPL(WiFi.dnsIP());
    DP("Wifi IP:");
    DPL(WiFi.localIP());

/*
    char ntpServerName[] = "mp3.ffh.de";
    IPAddress ntpServerIP;
    WiFi.hostByName(ntpServerName, ntpServerIP);
    DP("WDR IP:");
    DPL(ntpServerIP);

    uint32_t t = millis();
    WiFiClient        client;
    if(client.connect(ntpServerName, 80, 1000)) {
        uint32_t dt = millis() - t;
        DPF("Connected in: %i\n", dt);
    }*/
    DPL("DONE");

    DP("SNT Server Setup:");

    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0,  (char*) "pool.ntp.org");
    sntp_init();
    setenv("TZ", "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", 1);
    tzset();

    delay(500); //needed until time is available
    DPL("DONE");


}

void sendData(uint8_t *bytes, size_t count) {
    // send them off to the server
    //digitalWrite(LED_BUILTIN, HIGH);

    HTTPClient httpClientI2S;

    if(httpClientI2S.begin(I2S_SERVER_URL)) {
        httpClientI2S.addHeader("content-type", "application/octet-stream");
        httpClientI2S.POST(bytes, count);
        httpClientI2S.end();
    } else DPL("Http Client not connected!");

    // digitalWrite(LED_BUILTIN, LOW);
}
