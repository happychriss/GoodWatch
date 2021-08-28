//
// Created by development on 13.08.21.
//

#include "paint_alarm.h"
#include <Fonts/FreeSans12pt7b.h>
#include "edgeimpulse/ff_command_set_inference.h"
#include "inference_sound.hpp"
#include "driver/i2s.h"
#include <RTClib.h>
#include <EepromAT24C32.h>

#define ALARM_NUMBERS_STORAGE 7// number of different alarms stored
#define ALARM_NUMBERS_DISPLAY 5
#define TIME_EMPTY "--:--"
//  { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "j", "n" };
#define ANSWER_YES 10
#define ANSWER_NO 11

inference_t inference;

extern RTC_DS3231 rtc_watch;

EepromAt24c32<TwoWire> RtcEeprom(Wire);

struct st_rtcData {
    uint32_t crc32;   // 4 bytes
    char rtc_alarms[ALARM_NUMBERS_STORAGE][6];
};


void PaintAlarmScreen(GxEPD2_GFX &d, const st_rtcData &rtcData, const char title[]);

uint32_t calculateCRC32(const uint8_t *data, size_t length) {
    uint32_t crc = 0xffffffff;
    while (length--) {
        uint8_t c = *data++;
        for (uint32_t i = 0x80; i > 0; i >>= 1) {
            bool bit = crc & 0x80000000;
            if (c & i) {
                bit = !bit;
            }

            crc <<= 1;
            if (bit) {
                crc ^= 0x04c11db7;
            }
        }
    }

    return crc;
}

bool getRTCData(st_rtcData *rtcData) {
    bool rtcValid = false;
    RtcEeprom.GetMemory(0, (uint8_t *) rtcData, sizeof(st_rtcData));
    DPF("RTC CR32: %i\n", rtcData->crc32);
    for (int i = 0; i < ALARM_NUMBERS_STORAGE; i++) {
        DPF("RTC Alarm[%i]: %s\n", i, rtcData->rtc_alarms[i]);
    }

    // Calculate the CRC of what we just read from RTC memory, but skip the first 4 bytes as that's the checksum itself.
    uint32_t crc = calculateCRC32(((uint8_t *) rtcData) + 4, sizeof(st_rtcData) - 4);
    DPF("Calc CR32: %i\n", crc);
    if (crc == rtcData->crc32) {
        rtcValid = true;
        DPL("Valid RTC");
        DPL(crc);

    } else {
        DPL("**** InValid RTC - Reinit Alams ***");
        for (int i = 0; i < ALARM_NUMBERS_STORAGE; i++) {
            strcpy(rtcData->rtc_alarms[i], TIME_EMPTY);
        }

    }
    return rtcValid;
}

bool writeRTCData(st_rtcData *rtcData) {

    DPL("Incoming buffer:");
    for (int i = 0; i < ALARM_NUMBERS_STORAGE; i++) {
        DPF("RTC Alarm[%i]: %s\n", i, rtcData->rtc_alarms[i]);
    }

    rtcData->crc32 = calculateCRC32(((uint8_t *) rtcData) + 4, sizeof(st_rtcData) - 4);
    DP("RTC-Calc: ");
    DPL(rtcData->crc32);

    RtcEeprom.WriteMemoryBuffer((uint8_t *) rtcData, sizeof(st_rtcData));
    DPL("******** Read Buffer ********");
    RtcEeprom.GetMemory(0, (uint8_t *) rtcData, sizeof(st_rtcData));
    DPF("RTC CR32: %i\n", rtcData->crc32);
    for (int i = 0; i < ALARM_NUMBERS_STORAGE; i++) {
        DPF("RTC Alarm[%i]: %s\n", i, rtcData->rtc_alarms[i]);
    }
}

void PL(GxEPD2_GFX &d, int line, int column, const char text[], bool b_partial, bool b_clear) {


    int lh = PT12_HEIGHT; //height of a line

    int ox = PT12_HEIGHT / 4; //offset from border
    int oy = PT12_HEIGHT / 4;

    int x = ox + column * PT12_WIDTH;
    int y = oy + line * lh;

    if (b_partial) {

        DPF("Partial Print line: %i, column:%i, Text: %s\n", line, column, text);
        int16_t tbx, tby;
        uint16_t tbw, tbh;
        d.getTextBounds(text, 0, 0, &tbx, &tby, &tbw, &tbh);

        if ((strlen(text)==0) or b_clear){
            b_clear=true;
            tbw=MAX_X-x;
            DPL("....clear text");
        }else {
            tbw=tbw+PT12_WIDTH;
        }

        d.setPartialWindow(x, y - PT12_HEIGHT, tbw, PT12_HEIGHT+6);
        d.firstPage();
        do {
            d.setCursor(x, y);
            if (b_clear) d.fillRect(x, y - PT12_HEIGHT, tbw,MAX_X-x,GxEPD_WHITE);
            d.print(text);
        } while (d.nextPage());
    } else {
        d.setCursor(x, y);
        d.print(text);
    }

}

//***************************************************************************************************************+

#define TIME_COLUMN 8 // which column the time to display
#define CONFIRM_LINE 11 // which line to show confirmation message
#define MESSAGE_LINE 1 // which line to show welcome message, instructions
#define MESSAGE_ANSWER_COLUMN  15
#define ERROR_CHAR '?'

void showDate(const char* txt, const DateTime& dt) {
    Serial.print(txt);
    Serial.print(' ');
    Serial.print(dt.year(), DEC);
    Serial.print('/');
    Serial.print(dt.month(), DEC);
    Serial.print('/');
    Serial.print(dt.day(), DEC);
    Serial.print(' ');
    Serial.print(dt.hour(), DEC);
    Serial.print(':');
    Serial.print(dt.minute(), DEC);
    Serial.print(':');
    Serial.print(dt.second(), DEC);

    Serial.println();
}

void ActivateAlarm(GxEPD2_GFX &d) {
    st_rtcData rtcData;

    DPL("******** Activate Alarm ********");

    RtcEeprom.Begin();
    getRTCData(&rtcData);

    DPF("Starting Inversion Mode - running on core: %i\n", xPortGetCoreID());
    DPL("Config:");
    ei_print_config();
    DPL("Starting Inversion Mode");
    run_classifier_init();
    // setup buffers and start CaptureSamples, ends in inference.buffers
    if (microphone_inference_start(EI_CLASSIFIER_SLICE_SIZE) == false) {
        ei_printf("ERR: Failed to setup audio sampling\r\n");
        return;
    }
    d.refresh();
    d.init(0, false);
    d.setFont(&FreeSans12pt7b);

    d.firstPage();
    PaintAlarmScreen(d, rtcData, "Alarm an! [1..5]");

    // Inference
    int value_idx = -1;
    while (value_idx < 0) {
        value_idx = inference_get_category_idx(); //  { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "j", "n" };
    }

    if (value_idx == ANSWER_NO) {
        return;
    }

    if (value_idx<ALARM_NUMBERS_DISPLAY) {
        DateTime rtc_now = rtc_watch.now();

        const char *str_alarm=rtcData.rtc_alarms[value_idx-1];
        int alarm_hour= ((int) str_alarm[0]-'0')*10+(int) str_alarm[1]-'0';
        int alarm_min= ((int) str_alarm[3]-'0')*10+(int) str_alarm[4]-'0';
        DPF("Wake up on: %i:%i\n",alarm_hour,alarm_min);

        bool day_plus=false;
        if (alarm_hour*60+alarm_min<=rtc_now.hour()*60+rtc_now.minute()) day_plus= true;
        DPL("Adding an extra day!");

        DateTime alarm(rtc_now.year(),rtc_now.month(),rtc_now.day(),alarm_hour,alarm_min,0);
        if (day_plus) alarm=alarm + TimeSpan(1, 0, 0, 0);

#ifdef MYDEBUG
        showDate("Wakeup Time:",alarm);
#endif

        TimeSpan diff_time =alarm-rtc_now;

        char str_wakeup[50];
        sprintf(str_wakeup, "Alarm[%c]: %s in %ih %imin? (j/n)?", value_idx + '0', str_alarm, diff_time.hours(), diff_time.minutes());
        DP("Resultstring:");DP(str_wakeup);DPL("<-END");

        PL(d, CONFIRM_LINE, 1, str_wakeup, true, true);

        int value_idx = -1;
        while (value_idx < 0) {
            value_idx = inference_get_category_idx(); //  { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "j", "n" };
        }

        if (value_idx == ANSWER_NO) {
            return;
        }

        rtc_watch.clearAlarm(1);
        rtc_watch.clearAlarm(2);
        rtc_watch.disableAlarm(1);

/*        // schedule an alarm 10 seconds in the future
        if(!rtc_watch.setAlarm1(
                rtc_watch.now() + TimeSpan(10),
                DS3231_A1_Second // this mode triggers the alarm when the seconds match. See Doxygen for other options
                )) {
            DPL("Error, alarm wasn't set!");
            PL(d, CONFIRM_LINE, 1, "Error Setting Alarm", true, true);
            delay(10000);
        }else {
            DPL("Alarm will happen in 10 seconds!");
        }*/

    if(!rtc_watch.setAlarm2(alarm,DS3231_A2_Hour)){
            DPL("!!!!!!!! ERROR SETTING ALARM");
            PL(d, CONFIRM_LINE, 1, "Error Setting Alarm", true, true);
            delay(10000);
            return;
        }
    }


}

void ProgramAlarm(GxEPD2_GFX &d) {

    st_rtcData rtcData;

    DPL("******** Program Alarm ********");

    RtcEeprom.Begin();
    getRTCData(&rtcData);

/*    const char my_string[]="aaaaaaaaaabbbbbbbbbbccccccccccddddddddddeeeeeeeeee";

    char test_string[100];

    DPL("******** Write Buffer ********");
    RtcEeprom.WriteMemoryBuffer(( uint8_t *)my_string,strlen(my_string));
    DPL("******** Read Buffer ********");
    RtcEeprom.GetMemory(0, ( uint8_t *)test_string, strlen(my_string));
    DP("Result:");DP(test_string);DPL("<-END");*/



    // ******************+ Use Voice detection to set-up alarm *******************************************

    DPF("Starting Inversion Mode - running on core: %i\n", xPortGetCoreID());
    DPL("Config:");
    ei_print_config();
    DPL("Starting Inversion Mode");
    run_classifier_init();
    // setup buffers and start CaptureSamples, ends in inference.buffers
    if (microphone_inference_start(EI_CLASSIFIER_SLICE_SIZE) == false) {
        ei_printf("ERR: Failed to setup audio sampling\r\n");
        return;
    }

    d.refresh();
    d.init(0, false);
    d.setFont(&FreeSans12pt7b);

    d.firstPage();


    // Lines for the time
    const int lin[] = {3, 4, 6, 7, 9};

    bool b_break = false;

    do {
        PaintAlarmScreen(d, rtcData, "Alarm festlegen [1..6]:");

        // ****************** Setup different alarms ************************************************++


        enum time_set_t {
            leave, set, hour1, hour2, min1, min2, done
        };
        char time_str[7] = TIME_EMPTY;
        int set_value;
        bool b_valid_time = false;

        for (int tx_int = set; tx_int != done; tx_int++) {
            DPF("State: %i\n", tx_int);
            auto tx = static_cast<time_set_t>(tx_int);

            // Inference
            int value_idx = -1;
            while (value_idx < 0) {
                value_idx = inference_get_category_idx(); //  { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "j", "n" };
            }
            char label = value_idx + '0';
            DPF("Inference-Result[%i] - IDX: %i\n", tx_int, value_idx);

            if (value_idx == ANSWER_NO) {
                tx_int--;
                DPF("Answer was NO---going one step back, now at: %i\n", tx_int);
                tx = static_cast<time_set_t>(tx_int);
                if (tx == leave) {
                    b_break = true;
                    break;
                }
            }

            if (tx == set) {

                DP("Option No: "); DPL(label);

                // Show result of selected option in MESSAGE_LINE
                if (value_idx <= ALARM_NUMBERS_DISPLAY) {
                    const char c[2] ={label,'\0'};
                    set_value = value_idx;
                    PL(d, MESSAGE_LINE, MESSAGE_ANSWER_COLUMN, c, true, true);
                    PL(d, lin[set_value - 1], TIME_COLUMN, TIME_EMPTY, true, true);
                } else {
                    const char c[2] ={ERROR_CHAR,'\0'};
                    PL(d, MESSAGE_LINE, MESSAGE_ANSWER_COLUMN, c, true, true);
                    DPL("Input validation - ERROR");
                    tx_int--;
                }

            }

            if (tx >= hour1 && tx <= min2) {

                DPF("Set Time Digits for alarm: %i:%s\n", set_value, time_str);

                bool b_error = false;
                switch (tx) {
                    case hour1  :
                        if (value_idx > 2) {
                            b_error = true;
                            time_str[0] = ERROR_CHAR;
                        } else
                            time_str[0] = label;
                        break;
                    case hour2 :
                        if (value_idx > 9) {
                            b_error = true;
                            time_str[1] = ERROR_CHAR;
                        } else
                            time_str[1] = label;
                        break;
                    case min1 :
                        if (value_idx > 5) {
                            b_error = true;
                            time_str[3] = ERROR_CHAR;
                        } else
                            time_str[3] = label;
                        break;
                    case min2 :
//                        if (value_idx != 0 && value_idx != 5) {
                            if (value_idx >9) {
                            b_error = true;
                            time_str[4] = ERROR_CHAR;
                        } else {
                            time_str[4] = label;
                            b_valid_time = true;
                            b_break = true;
                        }
                        break;
                }

                PL(d, lin[set_value - 1], TIME_COLUMN, time_str, true, true);

                if (b_error) {
                    DPL("Input validation - ERROR");
                    tx_int--;
                } else {
                    DP("Current Alarm Time: ");
                    DPL(time_str);

                }


            }

            if (b_break) {
                DPL("Lef the loop!");

                break;
            }

        }

        // end of foor loop - time entered


        if (b_valid_time) {
            DP("Final Alarm Time: ");
            DPL(time_str);
            strcpy(rtcData.rtc_alarms[set_value - 1], time_str);
            writeRTCData(&rtcData);
            b_valid_time=false;
        }

        PL(d, CONFIRM_LINE, 1, "Fertig? [ja/nein]", true, true);

        int value_idx = -1;
        while (value_idx < 0) {
            value_idx = inference_get_category_idx(); //  { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "j", "n" };
        }

        if (value_idx == ANSWER_NO) {
            PL(d, CONFIRM_LINE, 1, "", true, true);

        } else {
            break;
        }

    } while (true);

    microphone_inference_end();
}

void PaintAlarmScreen(GxEPD2_GFX &d, const st_rtcData &rtcData, const char title[]) {
    do {
        //        display.setCursor(x1, y1);
        d.fillScreen(GxEPD_WHITE);
        d.setTextColor(GxEPD_BLACK);

        PL(d, MESSAGE_LINE, 1, title, false, true);

        int i=3;
        PL(d, i, 1, "1: Einmal", false, true);
        PL(d, i++, TIME_COLUMN, rtcData.rtc_alarms[0], false, true);


        PL(d, i, 1, "2: Einmal", false, true);
        PL(d, i++, TIME_COLUMN, rtcData.rtc_alarms[1], false, true);

/*            PL(d, i, 1, "3: Einmal", false, true);
        PL(d, i++, TIME_COLUMN, rtcData.rtc_alarms[2], false, true);*/

        i++;
        PL(d, i, 1, "3: Mo-Fr", false, true);
        PL(d, i++, TIME_COLUMN, rtcData.rtc_alarms[2], false, true);

        PL(d, i, 1, "4: Mo-Fr", false, true);
        PL(d, i++, TIME_COLUMN, rtcData.rtc_alarms[3], false, true);

        i++;
        PL(d, i, 1, "5: Bike", false, true);
        PL(d, i, TIME_COLUMN, rtcData.rtc_alarms[4], false, true);


    } while (d.nextPage());

}
