#define ENABLE_GxEPD2_GFX 1

#include <Arduino.h>
#include "support.h"
#include <global.h>

// Speech Recognition
#include "edgeimpulse/ff_command_set_inference.h"
#include "inference.hpp"
#include "inference_parameter.h"
#include "driver/i2s.h"

// various
#include <WiFi.h>
#include "WiFiCredentials.h"
#include "data_acquisition.h"
#include <lwip/apps/sntp.h>
#include "Audio.h"

// custom
#include <display_support.hpp>
#include <apds_support.hpp>
#include <audio_support.hpp>
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



void ei_print_config();

void setup() {
    char buf[128];
    pinMode(LED_BUILTIN, OUTPUT);

    // Prepare Display Light
    pinMode(APDS9960_INT, INPUT);
    pinMode(DISPLAY_CONTROL, OUTPUT);
    pinMode(DISPLAY_POWER, OUTPUT);
    digitalWrite(DISPLAY_POWER, LOW);
    digitalWrite(DISPLAY_CONTROL, LOW);
    rtc_gpio_pullup_en(GPIO_NUM_34);

    Serial.begin(115200);

    if (print_wakeup_reason() == ESP_SLEEP_WAKEUP_EXT0) {
        DPL("RTC Wakeup - Clear Proximity Interrupt");
        apds.clearProximityInt();
    } else {

        DPL("Starting Wifi");
        WiFi.disconnect();
        WiFi.mode(WIFI_STA);
        WiFi.begin(SSID, PASSWORD);
        WiFi.setSleep(false);
        if (WiFi.waitForConnectResult() != WL_CONNECTED) {
            Serial.println("Connection Failed! Rebooting...");
            delay(5000);
            ESP.restart();
        }
        DPL("Connected to WiFi");

        DPL("SNTP Server Setup ");
        sntp_setoperatingmode(SNTP_OPMODE_POLL);
        sntp_setservername(0, "pool.ntp.org");
        sntp_init();


        // indicator LED
//    pinMode(LED_BUILTIN, OUTPUT);

    }

    DPL("**************** Init Display");
//    digitalWrite(DISPLAY_POWER, HIGH);
//    pwm_up_down(true, pwmtable_16, 256, 5);

    display.init(115200);
    display.setTextColor(GxEPD_BLACK);
    display.fillScreen(GxEPD_WHITE);
    display.setFullWindow();
    display.firstPage();

    char strftime_buf[64]={0};
    GetTimeNowString(strftime_buf,64);

    PaintWatch(display);

/*
    do {
        display.fillScreen(GxEPD_WHITE);

//        display.fillCircle(MAX_X / 2, MAX_Y / 2,70,GxEPD_BLACK);

        display.setCursor(100, 100);
        display.print(strftime_buf);
    } while (display.nextPage());
*/
//    delay(1000);

    DPL("****************  Init Audio"); // todo PSRAM selection disabled hardcoded in audio.cpp

    InitAudio();
//    SerialKeyWait();
    audio.connecttoFS(SD, "/test2.wav");
//     audio.connecttohost("http://mp3.ffh.de/radioffh/hqlivestream.aac");
//    SerialKeyWait();
    DPL("**************** V1 Init Speech");
    DP("Main Loop - Running on Core:");
    DPL(xPortGetCoreID());
//    SerialKeyWait();
#ifdef DATA_ACQUISITION
    // launch WiFi
    // setup the HTTP Client
     wifiClientI2S = new WiFiClient();
     httpClientI2S = new HTTPClient();
#else
    ei_print_config();
    DPL("Starting Inversion Mode");
    run_classifier_init();
#endif

    // setup buffers and start CaptureSamples, ends in inference.buffers
    if (microphone_inference_start(EI_CLASSIFIER_SLICE_SIZE) == false) {
        ei_printf("ERR: Failed to setup audio sampling\r\n");
        return;
    }

    delay(100);
}


void loop() {


#ifdef DATA_ACQUISITION
    sendData(wifiClientI2S, httpClientI2S, I2S_SERVER_URL, (uint8_t *) &inference.buffers[inference.buf_select ^ 1][0], EI_CLASSIFIER_SLICE_SIZE * 2);
#else
    int value_idx = inference_get_category_idx();
    if (value_idx==INF_ERROR) {
        DPL("INF Error");
    } else {
        DP("Got Inference Result:");DPL(value_idx);
    }
#ifndef DISABLE_EPD


//            String number_result = String(ei_classifier_inferencing_categories[final_value_idx]);
    helloValue(display, value_idx, 0);
    //           helloFullScreenPartialMode(display);
    DPL("Play the song!!!");
while(!b_audio_end_of_mp3) {
    audio.loop();
    if (Serial.available()) { // put streamURL in serial monitor
        audio.stopSong();
        String r = Serial.readString();
        r.trim();
        if (r.length() > 5) audio.connecttohost(r.c_str());
        log_i("free heap=%i", ESP.getFreeHeap());
    }
}

    DPL("DONE: Play the song!!!");
    // Get Ready for DEEP SLEEP *******************************************************************
    apds_init_proximity();

    pwm_up_down(false, pwmtable_16, 256, 5);
    apds.clearProximityInt();
    Serial.println("Prepare deep sleep");
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_34, LOW); //1 = High, 0 = Low
    Serial.println("Going to sleep now");
    display.powerOff();
    esp_deep_sleep_start();

#endif


#endif //DATA_ACQUISITION


}

