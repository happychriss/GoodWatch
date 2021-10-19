#define ENABLE_GxEPD2_GFX 1

#include <Arduino.h>
#include <time.h>
#include <RTClib.h>
#include "support.h"
#include <global_hardware.h>
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
#include <rtc_support.h>

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
RtcData rtcData;


esp_sleep_wakeup_cause_t wakeup_reason;

bool b_pir_wave = false;


#define CLOCK_INTERRUPT_PIN 39

void IRAM_ATTR Ext_INT1_ISR() {
    b_pir_wave = true;// Do Something ...

}


void setup() {

    Serial.begin(115200);
    DPL("********** SETUP ***************");

    // Setup pin input and output
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(PIR_INT, INPUT);
    pinMode(DISPLAY_CONTROL, OUTPUT);
    pinMode(DISPLAY_AND_SOUND_POWER, OUTPUT);
    pinMode(BATTERY_VOLTAGE, INPUT);
    digitalWrite(DISPLAY_AND_SOUND_POWER, LOW);
    digitalWrite(DISPLAY_CONTROL, LOW);

    rtcInit();
    // Set timezone to Berlin Standard Time
    setenv("TZ", "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", 1);
    tzset();

}

/*void  PaintWatchTask(GxEPD2_GFX &display) {
    PaintWatch(display, true);
    vTaskDelete(NULL);
}*/


void loop() {

    DPL("****** Main Loop ***********");
    wakeup_reason = print_wakeup_reason();
    int adc = analogRead(BATTERY_VOLTAGE);
    double BatteryVoltage;
    BatteryVoltage = (adc * 7.445) / 4096;
    DP("Battery V: ");
    DPL(BatteryVoltage);

    // Normal Boot **************************************************************************
    if (wakeup_reason == ESP_SLEEP_WAKEUP_UNDEFINED) {
        DPL("!!!! Normal Boot - Time Setup");
        SetupWifi_SNTP();
        rtcSetRTCFromInternet();
        PaintWatch(display, false, false);
    } else {
        // Check RTC  Clock ****************************************************************
        DPL("!!!! Not booting - Time Setup");
        if (rtc_watch.lostPower()) {
            DPL("!!!!!RTC Watch lost power - Reset from Internet");
            SetupWifi_SNTP();
            rtcSetRTCFromInternet();
        } else {
            rtsSetEspTime(rtc_watch.now());
        }

    }

#define ALARM1_5_MIN 1
#define ALARM2_ALARM 2

    if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT1) {
        DPL("!!!! RTC Alarm Clock Wakeup");

        if (rtc_watch.alarmFired(ALARM1_5_MIN)) {
            DPL("*** Alarm2 Fired: Alarm for 5min refresh");
            DPL("!!! RTC Wakeup after 5min");
            rtc_watch.clearAlarm(ALARM1_5_MIN);
            rtc_watch.disableAlarm(ALARM1_5_MIN);
            rtsSetEspTime(rtc_watch.now());
            PaintWatch(display, true, false);
        }


        if (rtc_watch.alarmFired(ALARM2_ALARM)) {
            DPL("*** Alarm2 Fired: ALARM for Wakup");
            rtc_watch.clearAlarm(ALARM2_ALARM);
            rtc_watch.disableAlarm(ALARM2_ALARM);
            DeactivateOneTimeAlarm();

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
            while (digitalRead(PIR_INT) == true) {
                delay(100);
            }

        };




    }

    // Proxi Hand Movement **************************************************************************
    if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0) {
        DPL("!!!! PIR Sensor Wakeup");

        // Initialize APDS-9960 (configure I2C and initial values)
        apds.init();
        apds.setProximityGain(PGAIN_1X);
        apds.enableProximitySensor(false);
        apds.enableLightSensor(false);
        uint16_t light_value = 0;
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

        if (ambient_light == 0) {
            digitalWrite(DISPLAY_AND_SOUND_POWER, HIGH);
            pwm_up_down(true, pwmtable_16, 256 / 4, 50);
        }

        if (proximity_data > 130) { //hand is very close
            ProgramAlarm(display);
            PaintWatch(display, false, false);
        } else if (proximity_data > 20) { //hand a bit away
            ProgramAlarm(display);
            PaintWatch(display, false, false);
        } else {
            PaintQuickTime(display, false);
        }

        if (ambient_light == 0) {
            delay(1000);
            pwm_up_down(false, pwmtable_16, 256 / 4, 20);
            digitalWrite(DISPLAY_AND_SOUND_POWER, LOW);
        } else {
            if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER) {
                DPL("Show current time");
                PaintWatch(display, true, false);
            }
        }

        // Give the PIR time to go down, before going to sleep
        while (digitalRead(PIR_INT) == true) {
            delay(100);
        }

    }
/*    // 5 min Watch Refresh  **************************************************************************
    if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER) {
        DPL("!!! RTC Wakeup after 5min");

        if (rtc_watch.alarmFired(1)) {
            DPL("*** Alarm1 Fired");
        };

        if (rtc_watch.alarmFired(2)) {
            DPL("*** Alarm2 Fired");
        };

        rtc_watch.clearAlarm(1);
        rtc_watch.clearAlarm(2);
        rtc_watch.disableAlarm(1);
        rtc_watch.disableAlarm(2);

        rtsSetEspTime(rtc_watch.now());
        PaintWatch(display, true, false);
    }*/


    // ************ Deep Sleep *******************************
    Serial.println("Prepare deep sleep");
    apds_init_proximity();
//    apds.clearProximityInt();

#define SLEEP_SEC (5*60)

    /*   int sleep_sec = SLEEP_SEC - ((dt.second() + dt.minute() * 60) % SLEEP_SEC);

       if (sleep_sec == 0) sleep_sec = sleep_sec;
       DP("Going to sleep now for (min/sec):");
       DP(sleep_sec / 60);
       DP(":");
       DPL(sleep_sec % 60);
       DP("Sleep Sec:");
       DPL(sleep_sec);
   */
    digitalWrite(DISPLAY_AND_SOUND_POWER, LOW);

//    esp_sleep_enable_timer_wakeup(sleep_sec * 1000 * 1000);
    DateTime dt = rtc_watch.now();
    int next_wake_up_min = (((dt.minute()) / 5) * 5) + 5;
    if (next_wake_up_min == 60) next_wake_up_min = 0;
    DateTime alarm(0, 0, 0, 0, next_wake_up_min, 0);
    rtc_watch.setAlarm1(alarm, DS3231_A1_Minute);

    // Alarm from PIR
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_34, HIGH); //1 = High, 0 = Low

    // Alarm from RTC
    esp_sleep_enable_ext1_wakeup(0x8000000000, ESP_EXT1_WAKEUP_ALL_LOW); //1 = High, 0 = Low

    SD.end();
    display.powerOff();
    esp_deep_sleep_start();
//    esp_light_sleep_start();

}

