#include <stdio.h>
#include "RP2040_PWM.h"

#define RAD_PER_PULSE 0.0981747704247; // 2pi / 64.
#define ENCA_pin A1
#define ENCB_pin A2
#define PWM_pin  A3

// enum eLogSubSystem {
//     FSM,
//     MOTOR_CONTROL,
//     HOMING,
//     ENCODER,
//     PWM
// };

// enum eLogLevel {
//     NONE   = 0,
//     LOW    = 100,
//     MEDIUM = 200,
//     HIGH   = 300,
//     FULL   = 400,
//     DEBUG  = 500
// };

// #define GLOBAL_LOG_LEVEL MEDIUM

//creates pwm instance
RP2040_PWM* PWM_Instance;

volatile long pulseCount = 0;
float GLOBAL_POSITION_RAD;
float GLOBAL_POSITION_M;

void setup() {
    pinMode(A0, INPUT); // current_sense_i
    pinMode(A1, INPUT); // ENCA_i
    pinMode(A2, INPUT); // ENCB_i
    pinMode(A3, OUTPUT); // PWM_o
    pinMode(4, INPUT); // prox_sens_i
    pinMode(5, INPUT); // fault_detected_n
    pinMode(6, OUTPUT); // PWM_dir_o

    // Count positive and negative edges encoder to achieve max 64CPR resolution
    attachInterrupt(digitalPinToInterrupt(A1), A_posedge, RISING);
    attachInterrupt(digitalPinToInterrupt(A1), A_negedge, FALLING);
    attachInterrupt(digitalPinToInterrupt(A2), B_posedge, RISING);
    attachInterrupt(digitalPinToInterrupt(A2), B_negedge, FALLING);

    // PWM setup. 20KHz
    PWM_Instance = new RP2040_PWM(PWM_pin, 20000, 0);

}

/*
CCW: A leading B
     ┌───┐     ┌───┐
A ───┘   └─────┘   └───
       ┌───┐     ┌───┐
B   ───┘   └─────┘   └───

CW: B leading A
       ┌───┐     ┌───┐
A   ───┘   └─────┘   └───
     ┌───┐     ┌───┐
B ───┘   └─────┘   └───

For a quadratrue encoder on a pos/neg edge depending on the value of the opposite phase,
we can know if the motor is rotating CW or CCW

Convert pulse count to global position (rad) inside the PID which fires at 80Hz
*/

void A_posedge() {
    if (digitalRead(ENCB_pin) == 1) { // CW
        pulseCount++;
    } // CCW
    else {
        pulseCount--;
    }
}

void A_negedge() {
    if (digitalRead(ENCB_pin) == 0) { // CW
        pulseCount++;
    } // CCW
    else {
        pulseCount--;
    }
}

void B_posedge() {
    if (digitalRead(ENCA_pin) == 0) { // CW
        pulseCount++;
    } // CCW
    else {
        pulseCount--;
    }
}

void B_negedge() {
    if (digitalRead(ENCA_pin) == 1) { // CW
        pulseCount++;
    } // CCW
    else {
        pulseCount--;
    }
}

// TODO: Get timers working and start logging encoder reads
void loop() {
    static unsigned long lastLogTime = 0;
    const unsigned long logInterval  = 500; // ms

    if (millis() - lastLogTime >= logInterval) {
        lastLogTime = millis();

        noInterrupts();
        Serial.print("[ENCODER] pulseCount == ");
        Serial.print(pulseCount);
        Serial.print("\n");

        // 20Khz - 10% DC
        PWM_Instance->setPWM(PWM_pin, 20000, 10);

        interrupts();
    }
}
 
// TODO: Make option to print only certain subsystems
// void Log(enum eLogSubSystem sys, enum eLogLevel level, char *msg) {
//     const char* logSubSystemNames[] = {
//         "FSM",
//         "MOTOR_CONTROL",
//         "HOMING",
//         "ENCODER",
//         "PWM"
//     };

//     if (level > GLOBAL_LOG_LEVEL) return;

//     Serial.print("[%s] %s\n", logSubSystemNames[sys], msg);
// }
