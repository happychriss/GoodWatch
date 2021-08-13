#define ENABLE_GxEPD2_GFX 1

#include <Arduino.h>
#include "support.h"
#include <global.h>

// Speech Recognition
#include "edgeimpulse/ff_command_set_inference.h"
#include "inference_sound.hpp"
#include "inference_parameter.h"
#include "driver/i2s.h"

// various

#include "data_acquisition.h"
#include <lwip/apps/sntp.h>
#include "Audio.h"

// custom
#include <display_support.hpp>
#include <apds_support.hpp>
#include <audio_support.hpp>
#include <wifi_support.hpp>

#include <support.h>
#include <paint_watch.h>

#undef DATA_ACQUISITION
#undef DISABLE_EPD


//********** Global Variables ***************************************************
// Wake Up Sensor
SparkFun_APDS9960 apds = SparkFun_APDS9960();

// EPD Display
BitmapDisplay bitmaps(display);

// Inference Global Data
//inference_t *tmp_inf= (inference_t*) heap_caps_malloc(sizeof(inference_t), MALLOC_CAP_SPIRAM);#

inference_t inference;


// inference_t inference;
bool record_ready = false;
signed short *sampleBuffer;
bool debug_nn = false; // Set this to true to see e.g. features generated from the raw signal
int print_results = -(EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW);
QueueHandle_t m_i2sQueue;


// Wifi and Connectivity
WiFiClient *wifiClientI2S = nullptr;
HTTPClient *httpClientI2S = nullptr;

// Audio playing with SD Card
bool b_audio_end_of_mp3 = false;
Audio audio;

RTC_DATA_ATTR bool b_watch_refreshed = false;
// RTC_DATA_ATTR int min_sim=1;

esp_sleep_wakeup_cause_t wakeup_reason;

void setup() {

    Serial.begin(115200);
    DPL("********** SETUP ***************");

    // Setup pin input and output
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(PIR_INT, INPUT);
    pinMode(DISPLAY_CONTROL, OUTPUT);
    pinMode(DISPLAY_POWER, OUTPUT);
    pinMode(BATTERY_VOLTAGE,INPUT);
    digitalWrite(DISPLAY_POWER, LOW);
    digitalWrite(DISPLAY_CONTROL, LOW);

    // Set timezone to Berlin Standard Time
    setenv("TZ", "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", 1);
    tzset();

}

/*void  PaintWatchTask(GxEPD2_GFX &display) {
    PaintWatch(display, true);
    vTaskDelete(NULL);
}*/


void loop() {

    struct tm timeinfo = {0};

    DPL("****** Main Loop ***********");
    wakeup_reason = print_wakeup_reason();
    int adc=analogRead(BATTERY_VOLTAGE);
    DPL(adc);
    double BatteryVoltage;
    BatteryVoltage= (adc  * 7.445)/4096;
    DP("Battery V: ");DPL(BatteryVoltage);

    // Normal Boot **************************************************************************
    if (wakeup_reason == ESP_SLEEP_WAKEUP_UNDEFINED) {
        SetupWifi_SNTP();
        PaintWatch(display, false, false);
    }


    // Proxi Hand Movement **************************************************************************
    if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0) {
        DPL("!!!! PIR Sensor Wakeup");


        // Initialize APDS-9960 (configure I2C and initial values)
        apds.init();
        apds.setProximityGain(PGAIN_1X);
        apds.enableProximitySensor(false);
        apds.enableLightSensor(false);
        uint16_t light_value=0;
        uint8_t proximity_data = 0;
        uint16_t ambient_light = 0;

        for (int i = 0; i < 70; i++) {
            // Read the proximity value
            apds.readProximity(proximity_data);
            apds.readAmbientLight(ambient_light);
            DP(i);
            DP("-");
            DP(proximity_data);
            DP("-");
            DPL(ambient_light);
            // Read the light levels (ambient, red, green, blue)
        }

        if (ambient_light!=0) {
            digitalWrite(DISPLAY_POWER, HIGH);
            pwm_up_down(true, pwmtable_16, 256/4, 50);
        }

        if (proximity_data > 100) {
            //        apds.clearProximityInt();

            PaintWatch(display, false, true);
        } else {
            PaintQuickTime(display, false);
        }

        if (ambient_light!=0) {
            delay(1000);

            pwm_up_down(false, pwmtable_16, 256/4, 20);
            digitalWrite(DISPLAY_POWER, LOW);
        }


        else {
            if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER) {
                DPL("Show current time");
                GetTimeNowString(&timeinfo);
                PaintWatch(display, true, false);

            }
        }

        // Give the PIR time to go down, before going to sleep
        while(digitalRead(PIR_INT) == true) {
            delay(100);
        }

    }
    // 5 min Watch Refresh  **************************************************************************
    if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER) {
        DPL("!!! RTC Wakeup after 5min");
        GetTimeNowString(&timeinfo);
        PaintWatch(display, true, false);
    }


    // ************ Deep Sleep *******************************
    Serial.println("Prepare deep sleep");
    apds_init_proximity();
//    apds.clearProximityInt();
    GetTimeNowString(&timeinfo);

#define SLEEP_SEC (5*60)

    int sleep_sec = SLEEP_SEC - ((timeinfo.tm_sec + timeinfo.tm_min * 60) % SLEEP_SEC);

    if (sleep_sec == 0) sleep_sec = sleep_sec;
    DP("Going to sleep now for (min/sec):");
    DP(sleep_sec / 60);
    DP(":");
    DPL(sleep_sec % 60);
    DP("Sleep Sec:");
    DPL(sleep_sec);

    esp_sleep_enable_timer_wakeup(sleep_sec * 1000 * 1000);
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_34, HIGH); //1 = High, 0 = Low
    SD.end();
    display.powerOff();
    esp_deep_sleep_start();
//    esp_light_sleep_start();

}

