#include <stdio.h>

enum eLogSubSystem {
    FSM,
    MOTOR_CONTROL,
    HOMING,
    ENCODER,
    PWM
};

enum eLogLevel {
    NONE   = 0,
    LOW    = 100,
    MEDIUM = 200,
    HIGH   = 300,
    FULL   = 400,
    DEBUG  = 500
};

#define GLOBAL_LOG_LEVEL MEDIUM

void setup() {
  pinMode(A0, INPUT_PULLUP);
  pinMode(A1, INPUT_PULLUP);
  pinMode(A2, INPUT_PULLUP);
  pinMode(A3, OUTPUT);
  pinMode(4, INPUT_PULLUP);
  pinMode(5, INPUT_PULLUP);
  pinMode(6, OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
}


// TODO: Make option to print only certain subsystems
void Log(enum eLogSubSystem sys, enum eLogLevel level, char *msg) {
    const char* logSubSystemNames[] = {
        "FSM",
        "MOTOR_CONTROL",
        "HOMING",
        "ENCODER",
        "PWM"
    };

    if (level > GLOBAL_LOG_LEVEL) return;

    Serial.print("[%s] %s\n", logSubSystemNames[sys], msg);
}
