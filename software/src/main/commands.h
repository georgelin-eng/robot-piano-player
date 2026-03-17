#ifndef COMMANDS_H
#define COMMANDS_H

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

#define SCHEDULE_LENGTH = 53

struct command schedule[] = {
    {RIGHT_MOVE, 0, 0.500f, 0.500f},
    {RIGHT_PLAY, 1, 0.500f, 0.696f},
    {RIGHT_PLAY, 1, 0.630f, 0.696f},
    {RIGHT_MOVE, 40, 0.694f, 0.761f},
    {RIGHT_PLAY, 16, 0.761f, 0.826f},
    {RIGHT_MOVE, 0, 0.825f, 0.891f},
    {RIGHT_PLAY, 16, 0.891f, 0.957f},
    {RIGHT_PLAY, 1, 1.022f, 1.217f},
    {RIGHT_MOVE, 40, 1.022f, 1.088f},
    {RIGHT_PLAY, 16, 1.087f, 1.120f},
    {RIGHT_MOVE, 20, 1.119f, 1.152f},
    {RIGHT_PLAY, 4, 1.152f, 1.185f},
    {RIGHT_MOVE, 0, 1.249f, 1.283f},
    {RIGHT_PLAY, 16, 1.283f, 1.413f},
    {RIGHT_PLAY, 2, 1.543f, 1.739f},
    {RIGHT_PLAY, 2, 1.674f, 1.739f},
    {RIGHT_MOVE, 60, 1.704f, 1.804f},
    {RIGHT_PLAY, 8, 1.804f, 1.870f},
    {RIGHT_MOVE, 20, 1.868f, 1.935f},
    {RIGHT_PLAY, 16, 1.935f, 2.000f},
    {RIGHT_MOVE, 0, 2.032f, 2.065f},
    {RIGHT_PLAY, 2, 2.065f, 2.261f},
    {RIGHT_MOVE, 60, 2.065f, 2.165f},
    {RIGHT_PLAY, 8, 2.130f, 2.163f},
    {RIGHT_MOVE, 40, 2.162f, 2.196f},
    {RIGHT_PLAY, 4, 2.196f, 2.228f},
    {RIGHT_MOVE, 20, 2.293f, 2.326f},
    {RIGHT_PLAY, 16, 2.326f, 2.457f},
    {RIGHT_MOVE, 0, 2.554f, 2.587f},
    {RIGHT_PLAY, 1, 2.587f, 2.783f},
    {RIGHT_PLAY, 1, 2.717f, 2.783f},
    {RIGHT_MOVE, 40, 2.781f, 2.848f},
    {RIGHT_PLAY, 16, 2.848f, 2.913f},
    {RIGHT_MOVE, 0, 2.912f, 2.978f},
    {RIGHT_PLAY, 16, 2.978f, 3.043f},
    {RIGHT_PLAY, 1, 3.109f, 3.304f},
    {RIGHT_MOVE, 40, 3.109f, 3.175f},
    {RIGHT_PLAY, 16, 3.174f, 3.207f},
    {RIGHT_PLAY, 1, 3.239f, 3.272f},
    {RIGHT_PLAY, 1, 3.370f, 3.484f},
    {RIGHT_PLAY, 1, 3.630f, 3.696f},

    {RIGHT_PLAY, 4, 3.696f, 3.728f},
    {RIGHT_PLAY, 8, 3.761f, 3.793f},
    {RIGHT_PLAY, 8, 3.891f, 3.957f},
    {RIGHT_PLAY, 16, 3.957f, 3.989f},
    {RIGHT_MOVE, 60, 3.988f, 4.022f},
    {RIGHT_PLAY, 8, 4.022f, 4.054f},
    {RIGHT_PLAY, 8, 4.152f, 4.217f},
    {RIGHT_PLAY, 16, 4.217f, 4.250f},
    {RIGHT_MOVE, 80, 4.249f, 4.283f},
    {RIGHT_PLAY, 16, 4.283f, 4.315f},
    {RIGHT_MOVE, 0, 4.283f, 4.416f},
    {RIGHT_PLAY, 1, 4.413f, 4.478f},
};

#endif