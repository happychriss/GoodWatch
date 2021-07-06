//
// Created by development on 29.06.21.
//

#ifndef GOODWATCH_APDS_SUPPORT_HPP
#define GOODWATCH_APDS_SUPPORT_HPP
#include "freertos/FreeRTOS.h"
#include <Wire.h>
#include <SparkFun_APDS9960.h>
#include <driver/rtc_io.h>

// Constants
#define PROX_INT_HIGH   50 // Proximity level for interrupt
#define PROX_INT_LOW    0  // No far interrupt

extern SparkFun_APDS9960 apds;

void apds_init_proximity() {
    apds.init();
    apds.resetGestureParameters(); // changed sparkfun, set as non private, to be fixed todo
    apds.wireWriteDataByte(APDS9960_WTIME, 0xa6);
    apds.wireWriteDataByte(APDS9960_PPULSE, DEFAULT_PROX_PPULSE);
    apds.setLEDBoost(LED_BOOST_100);
    apds.setProximityIntEnable(true);
    apds.setProximityIntLowThreshold(0); //no far interrupt
    apds.setProximityIntHighThreshold(50);
    apds.enablePower();
    apds.setMode(WAIT, 1);
    apds.setMode(PROXIMITY, 1);
    apds.clearProximityInt();
}


#endif //GOODWATCH_APDS_SUPPORT_HPP
