//
// Created by development on 18.10.21.
//

#ifndef GOODWATCH_GLOBAL_DISPLAY_H
#define GOODWATCH_GLOBAL_DISPLAY_H
#define ALARM_NUMBERS_DISPLAY 5
#define TIME_COLUMN 7 // which column the time to display
#define WAKE_HOURS_COLUMN (TIME_COLUMN + 5)
#define CONFIRM_LINE 11 // which line to show confirmation message
#define MESSAGE_LINE 1 // which line to show welcome message, instructions
#define MESSAGE_ANSWER_COLUMN  15
#define ERROR_CHAR 'x'
#define INPUT_PROMPT '?'
#define EMPTY_PROMPT '-'

#define TIME_START "?-:--"
#define TIME_EMPTY "--:--"

enum enum_alarm_type {
    single, repeating, bike
};

struct str_watch_config {
    int days[7];
    enum_alarm_type type;
    int dl; //line on the display

};

// days start from Sunday: Sun,M,T,T,F,Sat



#endif //GOODWATCH_GLOBAL_DISPLAY_H
