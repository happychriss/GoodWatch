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
    pinMode(APDS9960_INT, INPUT);
    pinMode(DISPLAY_CONTROL, OUTPUT);
    pinMode(DISPLAY_POWER, OUTPUT);
    digitalWrite(DISPLAY_POWER, LOW);
    digitalWrite(DISPLAY_CONTROL, LOW);
    rtc_gpio_pullup_en(GPIO_NUM_34);

    // Set timezone to Berlin Standard Time
    setenv("TZ", "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", 1);
    tzset();

    delay(400);

}

/*void  PaintWatchTask(GxEPD2_GFX &display) {
    PaintWatch(display, true);
    vTaskDelete(NULL);
}*/


void loop() {


    DPL("****** Main Loop ***********");
    wakeup_reason=print_wakeup_reason();

    // Normal Boot **************************************************************************
    if (wakeup_reason==ESP_SLEEP_WAKEUP_UNDEFINED) {
        SetupWifi_SNTP();
        PaintWatch(display, false, false);
     }

    // Proxi Hand Movement **************************************************************************
    if (wakeup_reason==ESP_SLEEP_WAKEUP_EXT0) {
        DPL("!!!! Proximity Wakeup");

        apds.clearProximityInt();
        digitalWrite(DISPLAY_POWER, HIGH);
        pwm_up_down(true, pwmtable_16, 256, 5);
        PaintWatch(display, false, true);
        delay(5000);
        pwm_up_down(false, pwmtable_16, 256, 5);

    }

    // 5 min Watch Refresh  **************************************************************************
    if (wakeup_reason==ESP_SLEEP_WAKEUP_TIMER) {
        DPL("!!! RTC Wakeup after 5min");
        PaintWatch(display, true, false);
    }


    // ************ Deep Sleep *******************************
    Serial.println("Prepare deep sleep");
    apds_init_proximity();
    apds.clearProximityInt();

    time_t now;
    time(&now);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

#define SLEEP_MIN 5
    int sleep_sec = (SLEEP_MIN-(timeinfo.tm_min%SLEEP_MIN))*60+(60-timeinfo.tm_sec);

    if (sleep_sec==0) sleep_sec=SLEEP_MIN*60;
    DP("Going to sleep now for (min/sec):");DP(sleep_sec/60);DP(":");DPL(sleep_sec%60);

    esp_sleep_enable_timer_wakeup(sleep_sec*1000*1000);
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_34, LOW); //1 = High, 0 = Low

    display.powerOff();
    esp_deep_sleep_start();

}

