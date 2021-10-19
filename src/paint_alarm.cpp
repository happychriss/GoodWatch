//
// Created by development on 13.08.21.
//

#include "paint_alarm.h"
#include <Fonts/FreeSans12pt7b.h>
#include "edgeimpulse/ff_command_set_inference.h"
#include "inference_sound.hpp"
#include "driver/i2s.h"
#include <RTClib.h>
#include "rtc_support.h"
#include "global_display.h"
#include <rtc_support.h>



//  { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "j", "n" };
#define ANSWER_YES 10
#define ANSWER_NO 11
#define ANSWER_SET 9

struct str_watch_config wc[ALARM_NUMBERS_DISPLAY] = {
        {{1, 1, 1, 1, 1, 1, 1}, single,    3}, // every day of the week
        {{1, 1, 1, 1, 1, 1, 1}, single,    4}, // every day of the week
        {{0, 0, 0, 1, 1, 1, 1}, repeating, 6}, // MO-FR
        {{0, 1, 1, 1, 1, 1, 0}, repeating, 7},  // MO-FR
        {{0, 1, 1, 1, 1, 1, 0}, bike,      9}, // Bike MO-FR

};


inference_t inference;

extern RTC_DS3231 rtc_watch;
extern RtcData rtcData;


// This functions checks for an alarm, if it should be set for the next day..
DateTime CorrectNextDay(DateTime input_time, int i) {

    DPL("** Correct Time **");
    DPF("Input Time[%i]: ", i);
    DPL(DateTimeString(input_time));

    DateTime res = input_time;

    DateTime tmp_now = rtc_watch.now();
    int curr_day = tmp_now.dayOfTheWeek();

    int next_day = curr_day + 1;
    if (next_day > 6) next_day = 0;

    if (input_time <= tmp_now) {
        for (int c = 1; c < 8; c++) {
            DPF("i: %i c: %i, next_day: %i, watch_config: %i\n", i, c, next_day, wc[i].days[next_day]);
            if (wc[i].days[next_day] == 1) {
                res = input_time + TimeSpan(c * 24 * 60 * 60);
                break;
            }
            next_day++;
            if (next_day > 6) next_day = 0;
        }

    }

    DP("Result Time: ");
    DPL(DateTimeString(res));
    return res;
}

int SortTimes(void const *a, void const *b) {
    const struct strct_alarm *ad = (struct strct_alarm *) a;
    const struct strct_alarm *bd = (struct strct_alarm *) b;
    return (int) (ad->time.secondstime() - bd->time.secondstime());
}

int DeactivateOneTimeAlarm() {

    DateTime tmp_now = rtc_watch.now();
    DP("Check alarm to deactivate, base time: ");DPL(DateTimeString(tmp_now));

    int index=-1;
    for (int i = 0; i < ALARM_NUMBERS_DISPLAY; i++) {
        if (rtcData.d.alarms[i].active && wc[i].type==single) {
            DPF("Checking [%i]: %s:", i, rtcData.str_short(i).c_str());
            if (rtcData.d.alarms[i].time == tmp_now) {
                DPL("Found");
                index=i;
                break;
            } else DPL("-");
        }

    }
    return index;
}

int GetNextAlarmIndex() {
    int i;
    DateTime tmp_now = rtc_watch.now();

    DPL("Input:");
    for (int i = 0; i < ALARM_NUMBERS_DISPLAY; i++) {
        DPF("RTC Alarm[%i]: %s- State: %i\n", i, rtcData.str_long(i).c_str(), rtcData.d.alarms[i].active);
    }
    qsort(rtcData.d.alarms, ALARM_NUMBERS_DISPLAY, sizeof(strct_alarm), SortTimes);
    DPL("PostSort:");
    for (int i = 0; i < ALARM_NUMBERS_DISPLAY; i++) {
        DPF("RTC Alarm[%i]: %s\n", i, rtcData.str_long(i).c_str());
    }

    // Find first relevant alarm
    int active_alarm = -1;


    for (i = 0; i < ALARM_NUMBERS_DISPLAY; i++) {
        if ((rtcData.d.alarms[i].active) && (rtcData.d.alarms[i].time > tmp_now)) {
            active_alarm = i;
            break;
        }
    }
    DPF("Next active alarm [%i]: %s\n", i, rtcData.str_short(i).c_str());

    return active_alarm;

}


DateTime updateAlarmTime(const char *str_alarm, int i) {
    DateTime tmp_time;
    int alarm_hour = ((int) str_alarm[0] - '0') * 10 + (int) str_alarm[1] - '0';
    int alarm_min = ((int) str_alarm[3] - '0') * 10 + (int) str_alarm[4] - '0';
    DPF("Wake up on: %i:%i\n", alarm_hour, alarm_min);
    DateTime tmp_now = rtc_watch.now();
    tmp_time = DateTime(tmp_now.year(), tmp_now.month(), tmp_now.day(), alarm_hour, alarm_min);
    return CorrectNextDay(tmp_time, i);

}

void PaintAlarmScreen(GxEPD2_GFX &d, const char title[]);


void PL(GxEPD2_GFX &d, int line, int column, String text, bool b_partial, bool b_clear) {


    int lh = PT12_HEIGHT; //height of a line

    int ox = PT12_HEIGHT / 4; //offset from border
    int oy = PT12_HEIGHT / 4;

    int x = ox + column * PT12_WIDTH;
    int y = oy + line * lh;


    if (b_partial) {

        DPF("Partial Print line: %i, column:%i, Text: %s\n", line, column, text.c_str());
        int16_t tbx, tby;
        uint16_t tbw, tbh;
        d.getTextBounds(text.c_str(), 0, 0, &tbx, &tby, &tbw, &tbh);

        if (text.length() == 0 or b_clear) {
            b_clear = true;
            tbw = MAX_X - x;
            DPL("....clear text");
        } else {
            tbw = tbw + PT12_WIDTH;
        }

        d.setPartialWindow(x, y - PT12_HEIGHT, tbw, PT12_HEIGHT + 6);
        d.firstPage();
        do {
            d.setCursor(x, y);
            if (b_clear) d.fillRect(x, y - PT12_HEIGHT, tbw, MAX_X - x, GxEPD_WHITE);
            d.print(text.c_str());
        } while (d.nextPage());
    } else {
        d.setCursor(x, y);
        d.print(text.c_str());
    }

}

//***************************************************************************************************************+





// just a trick to skip the ":" in the time string and convert from tx to index in the string
int si(int tx) {
    int tmp_tx = tx;
    if (tx > hour2) tmp_tx++;
    tmp_tx--;
    return tmp_tx;
}

bool BuildTimeString(const int tx, int value, char *time) {

    bool b_valid = true;
    char label = value + '0';

    if ((value == ANSWER_NO) || (value == ANSWER_YES)) {
        label = EMPTY_PROMPT;
    }

    // smaller 9 means, I am a number and not a command
    if (value < 10) {
        if ((tx == hour1 && value > 2) || (tx == min1 && value > 5) || ((tx == hour2) && ((time[0] - '0') * 10 + value > 24))) {
            label = ERROR_CHAR;
            b_valid = false;
        }
    }

    time[si(tx)] = label;

    // Clean up all previos value
    for (int i = tx + 1; i <= min2; i++) {
        time[si(i)] = EMPTY_PROMPT;
    }

    return b_valid;
}


void ProgramAlarm(GxEPD2_GFX &d) {

    DPL("******** Program Alarm ********");

    rtcData.getRTCData();

    d.refresh();
    d.init(0, false);
    d.setFont(&FreeSans12pt7b);
    d.firstPage();

    InitVoiceCommands();

    do {
        PaintAlarmScreen(d, "An/Aus [1..6] - Set [9]");

        char time_str[7] = TIME_EMPTY;
        bool b_valid_time = false;
        int value_idx = 0;
        int selected_alarm = 0;

        value_idx = GetVoiceCommand();

        if (value_idx == ANSWER_SET) {

            //**************************************************************************************
            // ****************** Activate Alarms ************************************************++
            //**************************************************************************************

            DPL("************+ Activate, switch alarm on/off");
            PL(d, MESSAGE_LINE, MESSAGE_ANSWER_COLUMN, "An/Aus [1..5]", true, true);

            while (true) {
                value_idx = GetVoiceCommand();
                if (value_idx == ANSWER_YES) {
                    break;
                }
                if (value_idx <= ALARM_NUMBERS_DISPLAY) {
                    rtcData.d.alarms[value_idx].active = !rtcData.d.alarms[value_idx].active;
                    if (rtcData.d.alarms[value_idx].active) {
                        PL(d, wc[value_idx - 1].dl, WAKE_HOURS_COLUMN, "( off )", true, true);
                    } else {

                        DateTime alarm = rtcData.d.alarms[value_idx - 1].time;
                        TimeSpan diff_time = alarm - rtc_watch.now();

                        char str_wakeup[50];
                        sprintf(str_wakeup, "(%ih %imin)", diff_time.hours() + (24 * diff_time.days()), diff_time.minutes());
                        PL(d, wc[value_idx - 1].dl, WAKE_HOURS_COLUMN, str_wakeup, true, true);

                        DP("Resultstring:");
                        DP(str_wakeup);
                        DPL("<-END");

                    }

                }
            }


        } else if (value_idx <= ALARM_NUMBERS_DISPLAY) {

            selected_alarm = value_idx - 1;
            DPF("************+ Change Alarm Time for alarm:%i\n", selected_alarm);

            //**************************************************************************************
            // ****************** Change Alarms on Display ***************************************++
            //**************************************************************************************

            // Clear old time from Display
            PL(d, wc[selected_alarm].dl, TIME_COLUMN, TIME_START, true, true);

            int tx = hour1;
            int tx_old;

            do {
                tx_old = tx;

                value_idx = GetVoiceCommand();

                if (value_idx == ANSWER_NO) {
                    tx--;
                    if (tx > leave) BuildTimeString(tx, INPUT_PROMPT - '0', time_str);
                } else {

                    if (BuildTimeString(tx, value_idx, time_str)) {
                        // Answer was valid, next step - otherwhise stay at the same character
                        tx++;
                        if (tx < done) BuildTimeString(tx, INPUT_PROMPT - '0', time_str);
                    }
                }

                DPF("State: %i -> %i VoiceCommand: %i  - TimeString:%s \n", tx_old, tx, value_idx, time_str);
                PL(d, wc[selected_alarm].dl, TIME_COLUMN, time_str, true, true);

            } while ((tx != done) && (tx != leave));
            if (tx == done) b_valid_time = true;
        } else {
            DPL("Input validation - ERROR");
        }

        //**************************************************************************************
        // ****************** Update RTC Clock ***************************************++
        //**************************************************************************************

        if (b_valid_time) {

            rtcData.d.alarms[selected_alarm].time = updateAlarmTime(time_str, selected_alarm);
            rtcData.d.alarms[selected_alarm].active = true;
            rtcData.d.alarms[selected_alarm].valid = true;
            DPF("Final Alarm Time: %s\n", rtcData.str_long(selected_alarm).c_str());

            rtcData.writeRTCData();
            b_valid_time = false;
        }

        //**************************************************************************************
        // ***************** Are we done? ************************************************

        PL(d, CONFIRM_LINE, 1, "Fertig? [ja/nein]", true, true);
        DPL("done - PL");
        value_idx = GetVoiceCommand();
//        int value_idx=ANSWER_YES;
        DPL("done - GetVoice");
        if (value_idx == ANSWER_NO) {
            PL(d,
               CONFIRM_LINE, 1, "", true, true);

        } else {
            break;
        }


    } while (true);


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

    int next_alarm = GetNextAlarmIndex();

    if (next_alarm >= 0) {
        DateTime alarm = rtcData.d.alarms[next_alarm].time;
        DPF("************ Setting Alarm[%i]: ", next_alarm);
        DPL(DateTimeString(alarm));

        if (!rtc_watch.setAlarm2(alarm, DS3231_A2_Date
        )) {

            DPL("!!!!!!!! ERROR SETTING ALARM");
            PL(d,
               CONFIRM_LINE, 1, "Error Setting Alarm", true, true);
            delay(10000);
            return;
        }

        microphone_inference_end();

    }
}

void PLAlarm(GxEPD2_GFX &d, int index, String print_text, bool b_partial, bool b_clear) {

    PL(d, wc[index].dl, 1, print_text, b_partial, b_clear);
    PL(d, wc[index].dl, TIME_COLUMN, rtcData.str_short(index), b_partial, b_clear);

    if ((rtcData.d.alarms[index].active) && (rtcData.d.alarms[index].valid)) {
        TimeSpan diff_time = rtcData.d.alarms[index].time - rtc_watch.now();
        char str_wakeup[50] = {0};
        sprintf(str_wakeup, "(%ih %imin)", diff_time.hours(), diff_time.minutes());
        PL(d, wc[index].dl, WAKE_HOURS_COLUMN, str_wakeup, b_partial, b_clear);
        DPF("Active alarm[%i] with:%s\n", index, str_wakeup);
    }

}

void PaintAlarmScreen(GxEPD2_GFX &d, const char title[]) {

    do {
        //        display.setCursor(x1, y1);
        d.fillScreen(GxEPD_WHITE);
        d.setTextColor(GxEPD_BLACK);

        PL(d, MESSAGE_LINE, 1, title, false, true);

        PLAlarm(d, 0, "1: Einmal", false, true);
        PLAlarm(d, 1, "2: Einmal", false, true);

        PLAlarm(d, 2, "3: Mo-Fr", false, true);
        PLAlarm(d, 3, "4: Mo-Fr", false, true);

        PLAlarm(d, 4, "5: Bike", false, true);


    } while (d.nextPage());

}
