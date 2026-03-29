#ifndef COMMANDS_H
#define COMMANDS_H

#define MOVE 0
#define PLAY 1

#include <stdint.h> // included for uint8_t definition

const int INITIAL_MOTOR_POSITION_MM = 0;

struct command {
    uint8_t action;
    uint32_t solenoid_or_position;
    float start_time;
    float end_time;
};

#define START_POS 0
#define MOVE_SIZE 20


struct command schedule[] = {
    {MOVE, START_POS,             0.000f,  1.000f},  // 1
    {MOVE, START_POS+MOVE_SIZE,   2.000f,  4.000f},  // 2
    {MOVE, START_POS,             4.000f,  6.000f},  // 3
    {MOVE, START_POS+MOVE_SIZE,   6.000f,  6.000f},  // 4

    {MOVE, START_POS,             8.000f,  9.000f},  // 5
    {MOVE, START_POS+MOVE_SIZE,   10.000f, 10.000f}, // 6
    {MOVE, START_POS,             12.000f, 11.000f}, // 7
    {MOVE, START_POS+MOVE_SIZE,   14.000f, 12.000f}, // 8
};

const int SCHEDULE_LENGTH = 8;


#endif