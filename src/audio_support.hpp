#include "Arduino.h"
#include "Audio.h"
#include "audio_support.h"


void audio_info(const char *info) {
    Serial.print("info        ");
    Serial.println(info);
}

void audio_eof_mp3(const char *info) {  //end of file
    Serial.print("eof_mp3     ");
    Serial.println(info);
    b_audio_end_of_mp3 = true;
}

void fade_in(void *parameter) {
    for (int i = 0; i < 21; i++) {
        audio.setVolume(i);
        vTaskDelay(pdMS_TO_TICKS(1000));
        DPF("INT volume: %i\n", i);
        if (b_pir_wave) break;
    }
    vTaskDelete(NULL);
}

void PlayWakeupSong() {

    if (!SPIFFS.begin(true, "/spiffs", 5, NULL)) {
        DPL("!!!!!  SPIFF Mount Failed !!!!!!!!!!!");
    }

    audio.setPinout(I2S_NUM_1_BCLK, I2S_NUM_1_LRC, I2S_NUM_1_DOUT);
    audio.setVolume(0); // 0...21
    audio.forceMono(true);

    xTaskCreate(
            fade_in,              /* Task function. */
            "fade_in",            /* String with name of task. */
            10000,                     /* Stack size in words. */
            NULL,       /* Parameter passed as input of the task */
            1,                         /* Priority of the task. */
            NULL);                     /* Task handle. */


    //    SerialKeyWait();


    audio.connecttoFS(SPIFFS, "/good_morning_short.mp3");
    //     audio.connecttohost("http://mp3.ffh.de/radioffh/hqlivestream.aac");

    DPL("Play the song!!!");
    while (!b_audio_end_of_mp3) {

        audio.loop();

        if (b_pir_wave) {
            DPL("End of Song, PIR was raised");
            audio.setVolume(0); // 0...21
            delay(200);
            b_pir_wave = false;
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
    delay(500);
    DPL("DONE");
}

