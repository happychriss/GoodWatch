#include "Arduino.h"
#include "Audio.h"

extern bool b_audio_end_of_mp3 ;
extern Audio audio;


void audio_info(const char *info){
    Serial.print("info        "); Serial.println(info);
}

void audio_eof_mp3(const char *info){  //end of file
    Serial.print("eof_mp3     ");Serial.println(info);
    b_audio_end_of_mp3=true;
}

void InitAudio() {
    DPL("****************  Init Audio"); // todo PSRAM selection disabled hardcoded in audio.cpp
    audio.setPinout(I2S_NUM_1_BCLK, I2S_NUM_1_LRC, I2S_NUM_1_DOUT);
    audio.setVolume(18); // 0...21

    if(!SD.begin()){
        DPL("Card Mount Failed");
        return;
    }
    uint8_t cardType = SD.cardType();

    if(cardType == CARD_NONE){
        DPL("No SD card attached");
        return;
    }

    Serial.print("SD Card Type: ");
    if(cardType == CARD_MMC){
        DPL("MMC");
    } else if(cardType == CARD_SD){
        DPL("SDSC");
    } else if(cardType == CARD_SDHC){
        DPL("SDHC");
    } else {
        DPL("UNKNOWN");
    }
}