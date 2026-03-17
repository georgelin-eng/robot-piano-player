#ifndef LED_COMMANDS_H
#define LED_COMMANDS_H

#include <stdint.h> // included for uint8_t definition

struct command {
    uint8_t action;
    uint32_t solenoid_or_position;
    float start_time;
    float end_time;
};

#define RIGHT_MOVE 0
#define RIGHT_PLAY 1
#define LEFT_PLAY  2

#define SCHEDULE_LENGTH 12

struct command schedule[] = {
//   - [ ]  Datasheet & Application
// - [ ]  Give a lesson FoD tool
// - [ ]  SOP Preparation  // 0
    {RIGHT_PLAY, (1 << 12) + 1, 2.000f,   3.0000f}, // 1
    {RIGHT_PLAY, (2 << 12) + 0, 3.000f,   4.0000f}, // 2
    {RIGHT_PLAY, (3 << 12) + 1, 4.000f,   5.0000f}, // 3
    {RIGHT_PLAY, (0 << 12) + 0, 5.000f,   6.0000f}, // 4

    {RIGHT_PLAY, (1 << 12) + 1, 6.000f,   7.0000f},  // 5
    {RIGHT_PLAY, (2 << 12) + 0, 7.000f,   8.0000f},  // 6
    {RIGHT_PLAY, (3 << 12) + 1, 8.000f,   9.0000f},  // 7
    {RIGHT_PLAY, (0 << 12) + 0, 9.000f,   10.0000f}, // 8

    {RIGHT_PLAY, (1 << 12) + 1, 10.000f,   11.0000f}, // 9
    {RIGHT_PLAY, (2 << 12) + 0, 11.000f,   12.0000f}, // 10
    {RIGHT_PLAY, (3 << 12) + 1, 12.000f,   13.0000f}, // 11
    {RIGHT_PLAY, (0 << 12) + 0, 13.000f,   14.0000f}, // 12
};

#endif