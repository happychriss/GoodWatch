//
// Created by development on 21.11.20.
//


#define LED_BUILTIN 13
#define I2S_MICRO I2S_NUM_1

#ifndef MS_ESP32_SUPPORT_H
#define MS_ESP32_SUPPORT_H
#define MYDEBUG
#ifdef MYDEBUG
#define MYDEBUG_CORE
#define DP(x)     Serial.print (x)
#define DPD(x)     Serial.print (x, DEC)
#define DPL(x)  Serial.println (x)
#else
#define DP(x)
#define DPD(x)
#define DPL(x)
#endif

int SerialKeyWait();
/**
 * @brief      Printf function uses vsnprintf and output using Arduino Serial
 *
 * @param[in]  format     Variable argument list
 */

 esp_sleep_wakeup_cause_t print_wakeup_reason();
void GetTimeNowString(int size, struct tm *timeinfo);


#endif //MS_ESP32_SUPPORT_H
