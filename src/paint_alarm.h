//
// Created by development on 13.08.21.
//

#ifndef GOODWATCH_PAINT_ALARM_H
#define GOODWATCH_PAINT_ALARM_H
#include <Arduino.h>
#include <StreamString.h>
#include <ctime>
#include "paint_watch.h"
#include "support.h"


#define PT18_HEIGHT 39
#define PT18_WIDTH 32

#define PT12_HEIGHT 25
#define PT12_WIDTH 20

void ProgramAlarm(GxEPD2_GFX &d );
void ActivateAlarm(GxEPD2_GFX &d);

#endif //GOODWATCH_PAINT_ALARM_H
