#ifndef COMMANDS_H
#define COMMANDS_H

#define MOVE 0
#define PLAY 1

#include <stdint.h> // included for uint8_t definition

const int INITIAL_MOTOR_POSITION_MM = 256;

struct command {
    uint8_t action;
    uint32_t solenoid_or_position;
    float start_time;
    float end_time;
};

struct command schedule[] = {
    {MOVE, 50,   0.000f,  2.000f},
    {MOVE, 100,  2.000f,  4.000f},
    {MOVE, 10,   4.000f,  6.000f},
    {MOVE, 200,  6.000f,  8.000f},
    {MOVE, 220,  8.000f, 10.000f},
    {MOVE, 240, 10.000f, 12.000f},
    {MOVE, 280, 12.000f, 14.000f},
    {MOVE, 200, 14.000f, 16.000f},
};

const int SCHEDULE_LENGTH = 165;



#endif