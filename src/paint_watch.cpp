//
// Created by development on 25.07.21.
//
#include <Arduino.h>
#include <StreamString.h>
#include "paint_watch.h"
#include "support.h"
#include "custom_fonts/FreeSansNumOnly75.h"

//
// Created by development on 25.07.21.
//9

#define M_PI 3.1415926535897932
#define RIN 120
#define RIN_EMPTY 138
#define ROUT 140

struct st_pwin {
    int x0;
    int y0;
    int x1;
    int y1;
    bool simulate;
};



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

    rin = (min > 5) ? RIN : RIN_EMPTY;
    DrawArcCircle_Q1(my_display, pwin, 200, 150, 4, 26, rin, ROUT);

    rin = (min > 10) ? RIN : RIN_EMPTY;
    DrawArcCircle_Q1(my_display, pwin, 200, 150, 34, 56, rin, ROUT);

    rin = (min > 15) ? RIN : RIN_EMPTY;
    DrawArcCircle_Q1(my_display, pwin, 200, 150, 64, 86, rin, ROUT);

    rin = (min > 20) ? RIN : RIN_EMPTY;
    DrawArcCircle_Q2(my_display, pwin, 200, 150, 94, 116, rin, ROUT);

    rin = (min > 25) ? RIN : RIN_EMPTY;
    DrawArcCircle_Q2(my_display, pwin, 200, 150, 124, 146, rin, ROUT);

    rin = (min > 30) ? RIN : RIN_EMPTY;
    DrawArcCircle_Q2(my_display, pwin, 200, 150, 154, 176, rin, ROUT);

    rin = (min > 35) ? RIN : RIN_EMPTY;
    DrawArcCircle_Q3(my_display, pwin, 200, 150, 184, 206, rin, ROUT);

    rin = (min > 40) ? RIN : RIN_EMPTY;
    DrawArcCircle_Q3(my_display, pwin, 200, 150, 214, 236, rin, ROUT);

    rin = (min > 45) ? RIN : RIN_EMPTY;
    DrawArcCircle_Q3(my_display, pwin, 200, 150, 244, 266, rin, ROUT);

    rin = (min > 50) ? RIN : RIN_EMPTY;
    DrawArcCircle_Q4(my_display, pwin, 200, 150, 274, 296, rin, ROUT);

    rin = (min > 55) ? RIN : RIN_EMPTY;
    DrawArcCircle_Q4(my_display, pwin, 200, 150, 304, 326, rin, ROUT);

//    rin = (min > 55) ? RIN : RIN_EMPTY;
    DrawArcCircle_Q4(my_display, pwin, 200, 150, 334, 356, RIN_EMPTY, ROUT);
}

void PaintPartialWatchMin(GxEPD2_GFX &my_display, st_pwin *pwin, int min) {
    int rin = RIN;
    if (min > 55)      DrawArcCircle_Q4(my_display, pwin, 200, 150, 304,326, rin, ROUT);
    else if (min > 50)      DrawArcCircle_Q4(my_display, pwin, 200, 150, 274,296, rin, ROUT);
    else if (min > 45) DrawArcCircle_Q4(my_display, pwin, 200, 150, 244, 266, rin, ROUT);
    else if (min > 40) DrawArcCircle_Q3(my_display, pwin, 200, 150, 214, 236, rin, ROUT);
    else if (min > 35) DrawArcCircle_Q3(my_display, pwin, 200, 150, 184, 206, rin, ROUT);
    else if (min > 30) DrawArcCircle_Q2(my_display, pwin, 200, 150, 154, 176, rin, ROUT);
    else if (min > 25) DrawArcCircle_Q2(my_display, pwin, 200, 150, 124, 146, rin, ROUT);
    else if (min > 20) DrawArcCircle_Q2(my_display, pwin, 200, 150, 94, 116, rin, ROUT);
    else if (min > 15) DrawArcCircle_Q1(my_display, pwin, 200, 150, 64, 86, rin, ROUT);
    else if (min > 10) DrawArcCircle_Q1(my_display, pwin, 200, 150, 34, 56, rin, ROUT);
    else if (min > 5) DrawArcCircle_Q1(my_display, pwin, 200, 150, 4, 26, rin, ROUT);

//            display.drawRect(pwin.x0, pwin.y0, pwin.x1-pwin.x0, pwin.y1-pwin.y0,GxEPD_BLACK);
}

void PaintWatch(GxEPD2_GFX &display, boolean b_refresh_only, boolean b_show_hhmm_time) {

    DPL("****** Entering Paint Watch Function");

    struct tm timeinfo{};
    GetTimeNowString(64, &timeinfo);
/*    int min =min_sim;
    min_sim=min_sim+5;
    if (min_sim>60) min_sim=0;
    DP("****** MIN SIM:");DPL(min_sim);*/

    int min = timeinfo.tm_min;

    st_pwin pwin{};
    pwin.x0 = display.width() - 1;
    pwin.y0 = display.height() - 1;
    pwin.x1 = 0;
    pwin.y1 = 0;

    int16_t tbx, tby;
    uint16_t tbw, tbh;
    StreamString valueString;
    valueString.print(timeinfo.tm_hour, 0);
    display.setFont(&FreeSans75pt7b);
    display.getTextBounds(valueString, 0, 0, &tbx, &tby, &tbw, &tbh);
    uint16_t x = ((display.width() - tbw) / 2) - tbx;
    uint16_t y = (display.height() * 2 / 4) + tbh / 2; // y is base line!


//    display.init(115200, false);
    display.init(115200, false);

    // just make sure the watch get refreshed
    if (min<7 && !b_watch_refreshed) {
        DPL("***New Hour, repaint full watch");
        b_watch_refreshed=true;
        b_refresh_only=false;
    } else if (min>10) {
        DPL("***No new Hour, normal refresh");
        b_watch_refreshed=false;
    }

    // to determine area that needs to be refreshed
    if (b_refresh_only) {
        // Run simulation to determine partial windows size and set partial windows
        pwin.simulate = true;
        PaintPartialWatchMin(display, &pwin, min);
        display.setPartialWindow(pwin.x0, pwin.y0, pwin.x1 - pwin.x0, pwin.y1 - pwin.y0);
    } else {
        display.setTextColor(GxEPD_BLACK);
        display.setFont(&FreeSans75pt7b);
    }

    // *************** Paint the watch in EPD loop *************************************

    display.firstPage();
    do {
        pwin.simulate = false;

        if (b_refresh_only) {
            DPL("***** Partial Refresh of Display ***");
            PaintPartialWatchMin(display, &pwin, min);

        } else {
            DPL("***** Full Refresh of Display ***");
            display.fillScreen(GxEPD_WHITE);
            PaintFullWatchMin(display, &pwin, min);

            //Paint Time
            display.setCursor(x, y);
            display.print(valueString);

        }


    } while (display.nextPage());
}

