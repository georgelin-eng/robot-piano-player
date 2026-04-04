#ifndef LED_COMMANDS_H
#define LED_COMMANDS_H

#define MOVE 0
#define SOLENOID_ON 1
#define SOLENOID_OFF 2


#include <stdint.h> // included for uint8_t definition
const float INITIAL_MOTOR_POSITION_MM = 0.0f;


struct command {
    float time;
    uint8_t action;
    float position_mm;
    uint32_t solenoid_mask;
};

#define MOVE 0
#define PLAY  2

#define SCHEDULE_LENGTH 2

struct command schedule[] = {

     {0.0f, SOLENOID_ON, 0.0f, 1<<18},
     {2.0f, SOLENOID_OFF, 0.0f, 1<<18},

};


#endif