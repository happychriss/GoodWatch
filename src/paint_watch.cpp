//
// Created by development on 25.07.21.
//
#include <Arduino.h>
#include <StreamString.h>

#include <ctime>

#include "paint_watch.h"
#include "support.h"
#include "custom_fonts/FreeSansNumOnly70.h"
#include "rtc_support.h"
#include "paint_alarm.h"
#include <Fonts/FreeSans24pt7b.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <my_RTClib.h>
#include <paint_support.h>


//
// Created by development on 25.07.21.
//9

#define RIN 110
#define RIN_EMPTY 129 //empty marker
#define ROUT 130
#define SEP_IN 110
#define SEP_OUT 150


struct st_pwin {
    int x0;
    int y0;
    int x1;
    int y1;
    bool simulate;
};

extern RTC_DS3231 rtc_watch;

void printText(GxEPD2_GFX &my_display, char * text, uint line) {

    //Serial.println("helloWorld");
    my_display.setRotation(1);
    my_display.setFont(&FreeSans24pt7b);
    my_display.setTextColor(GxEPD_BLACK);
    int16_t tbx, tby; uint16_t tbw, tbh;
    my_display.getTextBounds(text, 0, 0, &tbx, &tby, &tbw, &tbh);
    // center bounding box by transposition of origin:
    uint16_t x = ((my_display.width() - tbw) / 2) - tbx;
    uint16_t y = ((my_display.height() - tbh) / 2) - tby;
    my_display.setFullWindow();
    my_display.firstPage();
    do
    {
        my_display.fillScreen(GxEPD_WHITE);
        my_display.setCursor(x, y);
        my_display.print(text);
    }
    while (my_display.nextPage());
    //Serial.println("helloWorld done");
}

void gfx_line(GxEPD2_GFX &my_display, st_pwin *partial_win, int x0, int y0, int x1, int y1) {

    if (x0 > x1) {
        int x_tmp = x0;
        x0 = x1;
        x1 = x_tmp;
    }

    if (y0 > y1) {
        int y_tmp = y0;
        y0 = y1;
        y1 = y_tmp;
    }

    if (partial_win->simulate) {
        // keep the dimenstion of the updated windows
        if (x0 < partial_win->x0) partial_win->x0 = x0;
        if (y0 < partial_win->y0) partial_win->y0 = y0;

        if (x1 > partial_win->x1) partial_win->x1 = x1;
        if (y1 > partial_win->y1) partial_win->y1 = y1;

    } else {
        my_display.drawFastHLine(x0, y0, x1 - x0, GxEPD_BLACK);
    }
    //    my_display.drawLine(x0,y0,x1,y1,GxEPD_BLACK);

}


void DrawArcCircle_Q1(GxEPD2_GFX &my_display, st_pwin *pwin, int x0, int y0, double start_angle, double end_angle, int ri, int ro) {

    start_angle = start_angle * (M_PI / 180);
    end_angle = end_angle * (M_PI / 180);
    double tan_start_angel = tan(start_angle);
    double tan_end_angel = tan(end_angle);


    int ri2 = ri * ri;
    int ro2 = ro * ro;
    for (int cy = -ro; cy <= 0; cy++) {
        int cx_i;
        if (abs(ri) < abs(cy)) {
            cx_i = -1;
        } else {
            cx_i = (int) (sqrt(ri2 - cy * cy) + 0.5);
        }


        int cx_o = (int) (sqrt(ro2 - cy * cy) + 0.5);
        int cyy = cy + y0;

        int dx_1 = tan_start_angel * cy * -1;
        int dx_2 = tan_end_angel * cy * -1;


        if ((dx_1) > (cx_i)) { cx_i = dx_1; };
        if ((dx_2) < (cx_o)) { cx_o = dx_2; };

        int x0d = x0 + cx_i;
        int x1d = x0 + cx_o;


        if (cx_i <= cx_o) {
            gfx_line(my_display, pwin, x0d, cyy, x1d, cyy);
            /*       DP(cyy);
                   DP(":");
                   DP(cx_i);
                   DP("-");
                   DPL(cx_o);*/
        }
    }
}


void DrawArcCircle_Q2(GxEPD2_GFX &my_display, st_pwin *pwin, int x0, int y0, double start_angle, double end_angle, int ri, int ro) {

    start_angle = (start_angle - 90) * (M_PI / 180);
    end_angle = (end_angle - 90) * (M_PI / 180);
    double tan_start_angel = tan(start_angle);
    double tan_end_angel = tan(end_angle);

    int ri2 = ri * ri;
    int ro2 = ro * ro;
    for (int cy = 0; cy <= ro; cy++) {
        int cx_i;
        if (abs(ri) < abs(cy)) {
            cx_i = -1;
        } else {
            cx_i = (int) (sqrt(ri2 - cy * cy) + 0.5);
        }
        int cx_o = (int) (sqrt(ro2 - cy * cy) + 0.5);
        int cyy = cy + y0;


        int dx_1 = cy / tan_start_angel;
        int dx_2 = cy / tan_end_angel;

        if (dx_1 < cx_o) { cx_o = dx_1; };
        if (dx_2 > cx_i) { cx_i = dx_2; };

        if (cx_i <= cx_o) {
            gfx_line(my_display, pwin, x0 + cx_i, cyy, x0 + cx_o, cyy);
        }

    }
}

void DrawArcCircle_Q3(GxEPD2_GFX &my_display, st_pwin *pwin, int x0, int y0, double start_angle, double end_angle, int ri, int ro) {

    start_angle = (270 - start_angle) * (M_PI / 180);
    end_angle = (270 - end_angle) * (M_PI / 180);
    double tan_start_angel = tan(start_angle);
    double tan_end_angel = tan(end_angle);

    int ri2 = ri * ri;
    int ro2 = ro * ro;
    for (int cy = 0; cy <= ro; cy++) {
        int cx_i;
        if (abs(ri) < abs(cy)) {
            cx_i = -1;
        } else {
            cx_i = (int) (sqrt(ri2 - cy * cy) + 0.5);
        }
        int cx_o = (int) (sqrt(ro2 - cy * cy) + 0.5);
        int cyy = cy + y0;

        int dx_1 = cy / tan_end_angel;
        int dx_2 = cy / tan_start_angel;

        if (dx_1 < cx_o) { cx_o = dx_1; };
        if (dx_2 > cx_i) { cx_i = dx_2; };

        if (cx_i <= cx_o) {
            gfx_line(my_display, pwin, x0 - cx_i, cyy, x0 - cx_o, cyy);
        }

    }
}

void DrawArcCircle_Q4(GxEPD2_GFX &my_display, st_pwin *pwin, int x0, int y0, double start_angle, double end_angle, int ri, int ro) {

    start_angle = (360 - start_angle) * (M_PI / 180);
    end_angle = (360 - end_angle) * (M_PI / 180);
    double tan_start_angel = tan(start_angle);
    double tan_end_angel = tan(end_angle);

    int ri2 = ri * ri;
    int ro2 = ro * ro;
    for (int cy = -ro; cy <= 0; cy++) {
        int cx_i;
        if (abs(ri) < abs(cy)) {
            cx_i = -1;
        } else {
            cx_i = (int) (sqrt(ri2 - cy * cy) + 0.5);
        }
        int cx_o = (int) (sqrt(ro2 - cy * cy) + 0.5);
        int cyy = cy + y0;

        int dx_1 = tan_start_angel * cy * -1;
        int dx_2 = tan_end_angel * cy * -1;

        if (dx_1 < cx_o) { cx_o = dx_1; };
        if (dx_2 > cx_i) { cx_i = dx_2; };

        if (cx_i <= cx_o) {
            gfx_line(my_display, pwin, x0 - cx_i, cyy, x0 - cx_o, cyy);
        }
    }
}

//paint a full WatchView based on minutes, pwin contains indicator if refresh or just window calculation
void PaintFullWatchMin(GxEPD2_GFX &my_display, st_pwin *pwin, int min) {
    int rin;


    my_display.drawFastHLine(my_display.width() / 2 - (ROUT + 20), my_display.height() / 2, 40, GxEPD_BLACK);
    my_display.drawFastHLine(my_display.width() / 2 + (ROUT + 20), my_display.height() / 2, 40, GxEPD_BLACK);

    rin = (min >= 5) ? RIN : RIN_EMPTY;
    DrawArcCircle_Q1(my_display, pwin, 200, 150, 4, 26, rin, ROUT);

    rin = (min >= 10) ? RIN : RIN_EMPTY;
    DrawArcCircle_Q1(my_display, pwin, 200, 150, 34, 56, rin, ROUT);

    rin = (min >= 15) ? RIN : RIN_EMPTY;
    DrawArcCircle_Q1(my_display, pwin, 200, 150, 64, 86, rin, ROUT);

    DrawArcCircle_Q1(my_display, pwin, 200, 150, 89, 90, SEP_IN, SEP_OUT);
    DrawArcCircle_Q2(my_display, pwin, 200, 150, 90, 91, SEP_IN, SEP_OUT);

    rin = (min >= 20) ? RIN : RIN_EMPTY;
    DrawArcCircle_Q2(my_display, pwin, 200, 150, 94, 116, rin, ROUT);

    rin = (min >= 25) ? RIN : RIN_EMPTY;
    DrawArcCircle_Q2(my_display, pwin, 200, 150, 124, 146, rin, ROUT);

    rin = (min >= 30) ? RIN : RIN_EMPTY;
    DrawArcCircle_Q2(my_display, pwin, 200, 150, 154, 176, rin, ROUT);

    DrawArcCircle_Q2(my_display, pwin, 200, 150, 179, 180, SEP_IN, SEP_OUT);
    DrawArcCircle_Q3(my_display, pwin, 200, 150, 180, 181, SEP_IN, SEP_OUT);

    rin = (min >= 35) ? RIN : RIN_EMPTY;
    DrawArcCircle_Q3(my_display, pwin, 200, 150, 184, 206, rin, ROUT);

    rin = (min >= 40) ? RIN : RIN_EMPTY;
    DrawArcCircle_Q3(my_display, pwin, 200, 150, 214, 236, rin, ROUT);

    rin = (min >= 45) ? RIN : RIN_EMPTY;
    DrawArcCircle_Q3(my_display, pwin, 200, 150, 244, 266, rin, ROUT);

    DrawArcCircle_Q3(my_display, pwin, 200, 150, 269, 270, SEP_IN, SEP_OUT);
    DrawArcCircle_Q4(my_display, pwin, 200, 150, 270, 271, SEP_IN, SEP_OUT);

    rin = (min >= 50) ? RIN : RIN_EMPTY;
    DrawArcCircle_Q4(my_display, pwin, 200, 150, 274, 296, rin, ROUT);

    rin = (min >= 55) ? RIN : RIN_EMPTY;
    DrawArcCircle_Q4(my_display, pwin, 200, 150, 304, 326, rin, ROUT);

//    rin = (min > 55) ? RIN : RIN_EMPTY;
    DrawArcCircle_Q4(my_display, pwin, 200, 150, 334, 356, RIN_EMPTY, ROUT);

    DrawArcCircle_Q4(my_display, pwin, 200, 150, 359, 360, SEP_IN, SEP_OUT);
    DrawArcCircle_Q1(my_display, pwin, 200, 150, 0, 1, SEP_IN, SEP_OUT);


}

void PaintPartialWatchMin(GxEPD2_GFX &my_display, st_pwin *pwin, int min) {
    int rin = RIN;
    if (min >= 55) DrawArcCircle_Q4(my_display, pwin, 200, 150, 304, 326, rin, ROUT);
    else if (min >= 50) DrawArcCircle_Q4(my_display, pwin, 200, 150, 274, 296, rin, ROUT);
    else if (min >= 45) DrawArcCircle_Q3(my_display, pwin, 200, 150, 244, 266, rin, ROUT);
    else if (min >= 40) DrawArcCircle_Q3(my_display, pwin, 200, 150, 214, 236, rin, ROUT);
    else if (min >= 35) DrawArcCircle_Q3(my_display, pwin, 200, 150, 184, 206, rin, ROUT);
    else if (min >= 30) DrawArcCircle_Q2(my_display, pwin, 200, 150, 154, 176, rin, ROUT);
    else if (min >= 25) DrawArcCircle_Q2(my_display, pwin, 200, 150, 124, 146, rin, ROUT);
    else if (min >= 20) DrawArcCircle_Q2(my_display, pwin, 200, 150, 94, 116, rin, ROUT);
    else if (min >= 15) DrawArcCircle_Q1(my_display, pwin, 200, 150, 64, 86, rin, ROUT);
    else if (min >= 10) DrawArcCircle_Q1(my_display, pwin, 200, 150, 34, 56, rin, ROUT);
    else if (min >= 5) DrawArcCircle_Q1(my_display, pwin, 200, 150, 4, 26, rin, ROUT);

//            display.drawRect(pwin.x0, pwin.y0, pwin.x1-pwin.x0, pwin.y1-pwin.y0,GxEPD_BLACK);
}


void PaintQuickTime(GxEPD2_GFX &display, boolean b_clear) {

    //Paint Time
    int16_t tbx, tby;
    uint16_t tbw, tbh;

    int16_t sbx, sby;
    uint16_t sbw, sbh;

    display.init(0, false);

    DateTime now = rtc_watch.now();
    char str_format[]="hh:mm:ss";
    String str_time_now= now.toString(str_format);

    display.setFont(&FreeSans70pt7b);
    display.getTextBounds("88", 0, 0, &tbx, &tby, &tbw, &tbh);
    uint16_t tx = ((display.width() - tbw) / 2) - tbx;
    uint16_t ty = ((display.height() - tbh) / 2) - tby - tbh; // y is base line!
    display.setPartialWindow(tx, ty, tbw, tbh + 8);

    display.setFont(&FreeSans18pt7b);
    display.getTextBounds(str_time_now, tx, ty, &sbx, &sby, &sbw, &sbh);

    // center the bounding box by transposition of the origin:
    uint16_t x1 = ((tbw - sbw) / 2);
    uint16_t y1 = ((tbh - sbh) / 2) + sbh;

/*    DP("tbw:");
    DP(tbw);
    DP(" tbh:");
    DPL(tbh);
    DP("sbw:");
    DP(sbw);
    DP(" sbh:");
    DPL(sbh);
    DP("tx:");
    DP(tx);
    DP(" ty:");
    DPL(ty);
    DP("x1:");
    DP(x1);
    DP(" y1:");
    DPL(y1);*/


    display.firstPage();
    do {
//        display.setCursor(x1, y1);
        display.fillScreen(GxEPD_WHITE);
        display.setTextColor(GxEPD_BLACK);
        display.setCursor(tx + x1, ty + y1);
        display.print(str_time_now);
    } while (display.nextPage());

    delay(2000);
    display.firstPage();
    do {
        //Paint Time
        int16_t tbx, tby;
        uint16_t tbw, tbh;

        String str_hour=String(now.hour());
        DP("Print Hour:");DPL(str_hour);

        display.setTextColor(GxEPD_BLACK);
        display.setFont(&FreeSans70pt7b);
        display.getTextBounds(str_hour, 0, 0, &tbx, &tby, &tbw, &tbh);
        uint16_t x = ((display.width() - tbw) / 2) - tbx;
        uint16_t y = (display.height() * 2 / 4) + tbh / 2; // y is base line!
        display.setCursor(x, y);
        display.print(str_hour);
    } while (display.nextPage());
}


void PaintWatch(GxEPD2_GFX &display, boolean b_refresh_only, boolean b_show_hhmm_time) {

    DateTime now = rtc_watch.now();

    char str_format[]="hh:mm:ss - DDD, DD.MM.YYYY";
    DP("****** Entering Paint Watch Function - Timebase:");
    DPL(now.toString(str_format));

//    int min =min_sim;
//    min_sim=min_sim+5;
//    if (min_sim>60) min_sim=0;
//    DP("****** MIN SIM:");DPL(min_sim);*/

    int min = now.minute();
    if (now.second() && min<59) min=min+1; //if wakeup is at 04:59 - make sure we enable the next segment

    display.init(0, false);
    st_pwin pwin{};
    pwin.x0 = display.width() - 1;
    pwin.y0 = display.height() - 1;
    pwin.x1 = 0;
    pwin.y1 = 0;

//    display.init(115200, false);


    // just make sure the watch get refreshed
    if (min < 7 && !b_watch_refreshed) {
        DPL("***New Hour, repaint full watch");
        b_watch_refreshed = true;
        b_refresh_only = false;
    } else if (min > 10) {
        DPL("***No new Hour, normal refresh");
        b_watch_refreshed = false;
    }

    // to determine area that needs to be refreshed
    if (b_refresh_only) {
        // Run simulation to determine partial windows size and set partial windows
        pwin.simulate = true;

    } else {
        display.setTextColor(GxEPD_BLACK);
        display.setFont(&FreeSans70pt7b);
    }

    // *************** Paint the watch in EPD loop *************************************

    display.firstPage();
    do {

        if (b_refresh_only) {
            DPL("***** Partial Refresh of Display ***");

            pwin.simulate = true;
            PaintPartialWatchMin(display, &pwin, min);
            display.setPartialWindow(pwin.x0, pwin.y0, pwin.x1 - pwin.x0, pwin.y1 - pwin.y0);
            pwin.simulate = false;
            PaintPartialWatchMin(display, &pwin, min);


        } else {
            DPL("***** Full Refresh of Display ***");
            pwin.simulate = false;
            display.fillScreen(GxEPD_WHITE);
            PaintFullWatchMin(display, &pwin, min);

/*          Paint Time*/

            int16_t tbx, tby;
            uint16_t tbw, tbh;

            String str_hour=String(now.hour());
            DP("Print Hour:");DPL(str_hour);
            display.setTextColor(GxEPD_BLACK);
            display.setFont(&FreeSans70pt7b);
            display.getTextBounds(str_hour, 0, 0, &tbx, &tby, &tbw, &tbh);
            uint16_t x = ((display.width() - tbw) / 2) - tbx;
            uint16_t y = (display.height() * 2 / 4) + tbh / 2; // y is base line!
            display.setCursor(x, y);
            display.print(str_hour);

/*          Paint Alarm */

            DateTime rtc_alarm={};
            DateTime cur_time=rtc_watch.now();

            if (rtc_watch.getAlarm2(&rtc_alarm, cur_time)) {;
                char str_format1[]="hh:mm DD.MM.YYYY";
                String str_alarm = rtc_alarm.toString(str_format1);
                DP("****** Alarm from Watch is set-up:");
                DPL(str_alarm);
                
                int hours=(rtc_alarm-cur_time).hours();
                if (hours<24) {
                     char str_format2[]="hh:mm";
                    str_alarm = rtc_alarm.toString(str_format2);
                    str_alarm = String(str_alarm+" ("+String(hours)+"h)");
                    display.setTextColor(GxEPD_BLACK);
                    display.setFont(&FreeSans12pt7b);
                    PL(display, 10*PT12_HEIGHT, 0,str_alarm , false, true);
                } else { DPL("Alarm more than 24 hours away");}
                
                
            } else {
                DPL("No alarm active");
            }


        }


    } while (display.nextPage());
}

