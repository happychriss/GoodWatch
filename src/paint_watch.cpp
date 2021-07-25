//
// Created by development on 25.07.21.
//

#include "paint_watch.h"
#include "support.h"
//
// Created by development on 25.07.21.
//

#define M_PI 3.1415926535897932

void gfx_line(GxEPD2_GFX &my_display, int x0, int y0, int x1, int y1) {
//    my_display.drawLine(x0,y0,x1,y1,GxEPD_BLACK);
    if (x1 > x0) {
        my_display.drawFastHLine(x0, y0, x1 - x0, GxEPD_BLACK);
    } else {
        my_display.drawFastHLine(x1, y0, x0 - x1, GxEPD_BLACK);
    }


}

void DrawArcCircle_Q1(GxEPD2_GFX &my_display, int x0, int y0, double start_angle, double end_angle, int ri, int ro) {


    start_angle = start_angle * (M_PI / 180);
    end_angle = end_angle * (M_PI / 180);
    double tan_start_angel = tan(start_angle);
    double tan_end_angel = tan(end_angle);

    int ri2 = ri * ri;
    int ro2 = ro * ro;
    for (int cy = -ro; cy <= 0; cy++) {
        int cx_i;
        if (abs(ri)<abs(cy)) {
            cx_i=-1;
        } else {
            cx_i = (int) (sqrt(ri2 - cy * cy) + 0.5);
        }


        int cx_o = (int) (sqrt(ro2 - cy * cy) + 0.5);
        int cyy = cy + y0;

        int dx_1 = tan_start_angel * cy * -1;
        int dx_2 = tan_end_angel * cy * -1;


        if ((dx_1) > (cx_i)) { cx_i = dx_1; };
        if ((dx_2) < (cx_o)) { cx_o = dx_2; };


        if (cx_i <= cx_o) {
            gfx_line(my_display, x0 + cx_i, cyy, x0 + cx_o, cyy);
            DP(cyy);DP(":");DP(cx_i);DP("-");DPL(cx_o);
        }
    }
}


void DrawArcCircle_Q2(GxEPD2_GFX &my_display, int x0, int y0, double start_angle, double end_angle, int ri, int ro) {


    start_angle = (start_angle - 90) * (M_PI / 180);
    end_angle = (end_angle - 90) * (M_PI / 180);
    double tan_start_angel = tan(start_angle);
    double tan_end_angel = tan(end_angle);

    int ri2 = ri * ri;
    int ro2 = ro * ro;
    for (int cy = 0; cy <= ro; cy++) {
        int cx_i;
        if (abs(ri)<abs(cy)) {
            cx_i=-1;
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
            gfx_line(my_display, x0 + cx_i, cyy, x0 + cx_o, cyy);
        }

    }
}

void DrawArcCircle_Q3(GxEPD2_GFX &my_display, int x0, int y0, double start_angle, double end_angle, int ri, int ro) {

    start_angle = (270 - start_angle) * (M_PI / 180);
    end_angle = (270 - end_angle) * (M_PI / 180);
    double tan_start_angel = tan(start_angle);
    double tan_end_angel = tan(end_angle);

    int ri2 = ri * ri;
    int ro2 = ro * ro;
    for (int cy = 0; cy <= ro; cy++) {
        int cx_i;
        if (abs(ri)<abs(cy)) {
            cx_i=-1;
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
            gfx_line(my_display, x0 - cx_i, cyy, x0 - cx_o, cyy);
        }

    }
}

void DrawArcCircle_Q4(GxEPD2_GFX &my_display, int x0, int y0, double start_angle, double end_angle, int ri, int ro) {

    start_angle = (360 - start_angle) * (M_PI / 180);
    end_angle = (360 - end_angle) * (M_PI / 180);
    double tan_start_angel = tan(start_angle);
    double tan_end_angel = tan(end_angle);

    int ri2 = ri * ri;
    int ro2 = ro * ro;
    for (int cy = -ro; cy <= 0; cy++) {
        int cx_i;
        if (abs(ri)<abs(cy)) {
            cx_i=-1;
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
            gfx_line(my_display, x0 - cx_i, cyy, x0 - cx_o, cyy);
        }
    }
}


void PaintWatch(GxEPD2_GFX &display) {
#define RIN 100
#define ROUT 130
    do {
        display.fillScreen(GxEPD_WHITE);

        DrawArcCircle_Q1(display, 200, 150, 2, 28, RIN, ROUT);
        DrawArcCircle_Q1(display, 200, 150, 32, 58, RIN, ROUT);
        DrawArcCircle_Q1(display, 200, 150, 62, 88, RIN, ROUT);

        DrawArcCircle_Q2(display, 200, 150, 92, 118, RIN, ROUT);
        DrawArcCircle_Q2(display, 200, 150, 122, 148, RIN, ROUT);
        DrawArcCircle_Q2(display, 200, 150, 152, 178, RIN, ROUT);

        DrawArcCircle_Q3(display, 200, 150, 182, 208, RIN, ROUT);
        DrawArcCircle_Q3(display, 200, 150, 212, 238, RIN, ROUT);
        DrawArcCircle_Q3(display, 200, 150, 242, 268, RIN, ROUT);

        DrawArcCircle_Q4(display, 200, 150, 272, 298, RIN, ROUT);
        DrawArcCircle_Q4(display, 200, 150, 302, 328, RIN, ROUT);
        DrawArcCircle_Q4(display, 200, 150, 332, 358, RIN, ROUT);

    } while (display.nextPage());
}

