#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdint.h> // included for uint8_t definition

struct command {
    uint8_t action;
    uint16_t solenoid_or_position;
    float start_time;
    float end_time;
};

extern const struct command command_table[];
extern const uint32_t command_table_len;


#endif