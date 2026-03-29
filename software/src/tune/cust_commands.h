#ifndef COMMANDS_H
#define COMMANDS_H

#define MOVE 0
#define PLAY 1

#include <stdint.h> // included for uint8_t definition

const int INITIAL_MOTOR_POSITION_MM = 200;

struct command {
    uint8_t action;
    uint32_t solenoid_or_position;
    float start_time;
    float end_time;
};

#define START_POS 100
#define MOVE_SIZE 20


struct command schedule[] = {
    {MOVE, START_POS,             0.000f,  1.000f}, // 1
    {MOVE, START_POS+MOVE_SIZE,   1.000f,  2.000f}, // 2
    {MOVE, START_POS,             2.000f,  3.000f}, // 3
    {MOVE, START_POS+MOVE_SIZE,   3.000f,  4.000f}, // 4

    {MOVE, START_POS,             4.000f,  5.000f}, // 5
    {MOVE, START_POS+MOVE_SIZE,   5.000f,  6.000f}, // 6
    {MOVE, START_POS,             6.000f,  7.000f}, // 7
    {MOVE, START_POS+MOVE_SIZE,   7.000f,  8.000f}, // 8


};

const int SCHEDULE_LENGTH = 8;


#endif