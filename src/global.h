//
// Created by development on 29.06.21.
//

#ifndef GOODWATCH_PINOUT_H
#define GOODWATCH_PINOUT_H

#define INF_ERROR (-1)

// Distance Measurement APDS9960 via *I2C* and GPIO 21=SDA and 22=SCL
#define APDS9960_INT   34  // Needs to be an interrupt pin

// Display Light
#define DISPLAY_CONTROL 27 //dims the light
#define DISPLAY_POWER 13// enables the  MT3608

//SD Card (Pins are not used, set as default) using SPI together with EPD
#define SD_CS          4
#define SPI_MOSI      23    // SD Card
#define SPI_MISO      19
#define SPI_SCK       18

//I2S Interface for Loudspeaker
#define I2S_NUM_1_BCLK 12
#define I2S_NUM_1_LRC 14
#define I2S_NUM_1_DOUT 26

//I2S Interface for Microphone
#define I2S_NUM_2_BCLK 25
#define I2S_NUM_2_LRC 33
#define I2S_NUM_2_DIN 32

// EPD Pins - GxEPD2_display_selection_new_style.h using SPI
// EPD Pins****
// CS=5, DC=0, RST=2,BUSY=15;
// SPI: SCK=18, SDI=MISO=23
// BUSY -> 15, RST -> 2, DC -> 0, CS -> 5, CLK -> SCK(18), DIN -> MOSI(23), GND -> GND, 3.3V -> 3.3V

#define EPD_CS 5 // Chip Select -
#define EPD_DC 0
#define EPD_RST 2
#define EPD_BUSY 15


#endif //GOODWATCH_PINOUT_H
