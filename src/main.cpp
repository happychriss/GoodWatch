#define ENABLE_GxEPD2_GFX 1

#include <Arduino.h>
#include <time.h>
#include <RTClib.h>
#include "support.h"
#include <global.h>



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
#include <paint_alarm.h>



#undef DATA_ACQUISITION
#undef DISABLE_EPD


//********** Global Variables ***************************************************
// Wake Up Sensor
SparkFun_APDS9960 apds = SparkFun_APDS9960();

// EPD Display
BitmapDisplay bitmaps(display);

// Inference Global Data
//inference_t *tmp_inf= (inference_t*) heap_caps_malloc(sizeof(inference_t), MALLOC_CAP_SPIRAM);#




// inference_t inference;
bool record_ready = false;
signed short *sampleBuffer;
bool debug_nn = false; // Set this to true to see e.g. features generated from the raw signal

QueueHandle_t m_i2sQueue;


// Wifi and Connectivity
WiFiClient *wifiClientI2S = nullptr;
HTTPClient *httpClientI2S = nullptr;

// Audio playing with SD Card
bool b_audio_end_of_mp3 = false;
Audio audio;

RTC_DATA_ATTR bool b_watch_refreshed = false;
// RTC_DATA_ATTR int min_sim=1;
RTC_DS3231 rtc_watch;

esp_sleep_wakeup_cause_t wakeup_reason;

bool b_pir_wave =false;


#define CLOCK_INTERRUPT_PIN 39

void IRAM_ATTR Ext_INT1_ISR()
{
    b_pir_wave=true;// Do Something ...

}

void rtcTimeNow() {
    DateTime now = rtc_watch.now();
    char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(" (");
    Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
    Serial.print(") ");
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();
}

void setup() {

    Serial.begin(115200);
    DPL("********** SETUP ***************");

    // Setup pin input and output
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(PIR_INT, INPUT);
    pinMode(DISPLAY_CONTROL, OUTPUT);
    pinMode(DISPLAY_AND_SOUND_POWER, OUTPUT);
    pinMode(BATTERY_VOLTAGE,INPUT);
    digitalWrite(DISPLAY_AND_SOUND_POWER, LOW);
    digitalWrite(DISPLAY_CONTROL, LOW);


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
    rtc_watch.clearAlarm(1);
    rtc_watch.clearAlarm(2);
    rtc_watch.disableAlarm(1);
    rtc_watch.disableAlarm(2);

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
    double BatteryVoltage;
    BatteryVoltage= (adc  * 7.445)/4096;
    DP("Battery V: ");DPL(BatteryVoltage);

    // Normal Boot **************************************************************************
    if (wakeup_reason == ESP_SLEEP_WAKEUP_UNDEFINED) {
        DPL("!!!! Normal Boot - Time Setup");

        SetupWifi_SNTP();
        GetTimeNowString(&timeinfo, false); // returns local time from ESP, so RTC watch is running on local time.
        rtc_watch.adjust(DateTime(timeinfo.tm_year,timeinfo.tm_mon,timeinfo.tm_mday,timeinfo.tm_hour,timeinfo.tm_min, timeinfo.tm_sec));
//        rtc_watch.adjust(timeinfo); // update RTC with system time
        PaintWatch(display, false, false);
    } else {
            DP("!!!! Not booting - Time Setup");
            if (rtc_watch.lostPower()) {
                DPL("!!!!!RTC Watch lost power!!!!!!!!!!!!!!!!!!!!!!!!");
                SetupWifi_SNTP();
                DPL("Refresh RTC Time from Internet:");
                GetTimeNowString(&timeinfo, false);
            } else {
                DateTime rtc_now = rtc_watch.now();
                timeval tv;
                tv.tv_sec = rtc_now.unixtime();
                tv.tv_usec = 0;  // keine Mikrosekunden setzen
                settimeofday(&tv, NULL);
                GetTimeNowString(&timeinfo, true);
            }

    }

    if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT1) {
        DPL("!!!! RTC Alarm Clock Wakeup");
        digitalWrite(DISPLAY_AND_SOUND_POWER, HIGH);
//        pinMode(GPIO_NUM_34, INPUT_PULLUP);
        attachInterrupt(GPIO_NUM_34, Ext_INT1_ISR, HIGH);
        InitAudio();
        //    SerialKeyWait();
        audio.connecttoFS(SD, "/good_morning.mp3");
        //     audio.connecttohost("http://mp3.ffh.de/radioffh/hqlivestream.aac");

        DPL("Play the song!!!");
        while (!b_audio_end_of_mp3) {
            audio.loop();
            if (b_pir_wave) {
                DPL("End of Song, PIR was raised");
                break;
            }
            if (Serial.available()) { // put streamURL in serial monitor
                audio.stopSong();
                String r = Serial.readString();
                r.trim();
                if (r.length() > 5) audio.connecttohost(r.c_str());
                log_i("free heap=%i", ESP.getFreeHeap());
            }
        }
        detachInterrupt(GPIO_NUM_34);
        digitalWrite(DISPLAY_AND_SOUND_POWER, LOW);
        while(digitalRead(PIR_INT) == true) {
            delay(100);
        }

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
/*
            DP(i);
            DP("-");
            DP(proximity_data);
            DP("-");
            DPL(ambient_light);*/

            // Read the light levels (ambient, red, green, blue)
        }

        if (ambient_light==0) {
            digitalWrite(DISPLAY_AND_SOUND_POWER, HIGH);
            pwm_up_down(true, pwmtable_16, 256/4, 50);
        }

        if (proximity_data > 130) {
            ProgramAlarm(display);
            PaintWatch(display, false, false);
        } else if (proximity_data>20) {
            ActivateAlarm(display);
            PaintWatch(display, false, false);
        } else {
            PaintQuickTime(display, false);
        }

        if (ambient_light==0) {
            delay(1000);
            pwm_up_down(false, pwmtable_16, 256/4, 20);
            digitalWrite(DISPLAY_AND_SOUND_POWER, LOW);
        }


        else {
            if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER) {
                DPL("Show current time");
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
        GetTimeNowString(&timeinfo, true);
        PaintWatch(display, true, false);
    }


    // ************ Deep Sleep *******************************
    Serial.println("Prepare deep sleep");
    apds_init_proximity();
//    apds.clearProximityInt();
    GetTimeNowString(&timeinfo, true);

#define SLEEP_SEC (5*60)

    int sleep_sec = SLEEP_SEC - ((timeinfo.tm_sec + timeinfo.tm_min * 60) % SLEEP_SEC);

    if (sleep_sec == 0) sleep_sec = sleep_sec;
    DP("Going to sleep now for (min/sec):");
    DP(sleep_sec / 60);
    DP(":");
    DPL(sleep_sec % 60);
    DP("Sleep Sec:");
    DPL(sleep_sec);

    digitalWrite(DISPLAY_AND_SOUND_POWER, LOW);
    esp_sleep_enable_timer_wakeup(sleep_sec * 1000 * 1000);

    // Alarm from PIR
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_34, HIGH); //1 = High, 0 = Low

    // Alarm from RTC
    esp_sleep_enable_ext1_wakeup(0x8000000000, ESP_EXT1_WAKEUP_ALL_LOW); //1 = High, 0 = Low

    SD.end();
    display.powerOff();
    esp_deep_sleep_start();
//    esp_light_sleep_start();

}

