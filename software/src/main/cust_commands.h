#ifndef CUST_COMMANDS_H
#define CUST_COMMANDS_H

#include <stdint.h> // included for uint8_t definition

struct command {
    uint8_t action;
    uint16_t solenoid_or_position;
    float start_time;
    float end_time;
};

#define RIGHT_MOVE 0
#define RIGHT_PLAY 1
#define LEFT_PLAY  2

#define SCHEDULE_LENGTH 15

struct command schedule[] = {
    {RIGHT_PLAY, 1,  0.500f,   1.000f},  // 0
    {RIGHT_MOVE, 20, 2.000f,   3.0000f}, // 1
    {RIGHT_MOVE, 50, 3.000f,   4.0000f}, // 2
    {RIGHT_MOVE, 80, 4.000f,   5.0000f}, // 3
    {RIGHT_MOVE, 10, 5.000f,   6.0000f}, // 4
    {RIGHT_MOVE, 50, 6.000f,   7.0000f}, // 5
    {RIGHT_MOVE, 80, 7.000f,   8.0000f}, // 6
    {RIGHT_MOVE, 20, 8.000f,   9.0000f}, // 7
    {RIGHT_MOVE, 10, 9.000f,  10.0000f}, // 8
    {RIGHT_MOVE, 60, 10.000f, 11.0000f}, // 9
    {RIGHT_MOVE, 100,11.000f, 12.0000f}, // 10
    {RIGHT_MOVE, 70, 12.000f, 13.0000f}, // 11
    {RIGHT_MOVE, 80, 13.000f, 14.0000f}, // 12
    {RIGHT_MOVE, 100,14.000f, 15.0000f}, // 13
    {RIGHT_MOVE,  20,15.000f, 16.0000f}, // 14
};

#endif