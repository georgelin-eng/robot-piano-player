#include <stdio.h>             // Standard IO
#include "pid-autotune.h"      // Auto-tune library
#include "RP2040_PWM.h"        // PWM
#include "pins.h"              // pin to variable mappings
#include "peripherals.h"       // ISRs for interacting with peripherals
#include <LCD1602.h>           // LCD library

#include <string>

// ----------- DEFINITIONS --------------

#define RAD_PER_PULSE 0.0981747704247*2// 2pi / 64.
#define KJT 0.00097000000000000005055  // joint-to-task space: 1:10 gearbox + 17.4mm diameter pulley 
#define KTJ 1030.9278350515462535      // task-to-joint space

#define PWM_FREQ 20000 // 20KHz
#define PID_CONTROL_FREQ 1000.0
#define PID_CONTROL_INTERVAL (1 / PID_CONTROL_FREQ)


#define GLOBAL_LOG_LEVEL LOG_HIGH

static char MSG_BUFFER[64]; // global message buffer for serial logging
static char LCD_BUFFER[16]; // LCD buffer



bool tuningFinished = false;

// ------------- OBJECT INSTANCES -------------

PID pid = PID();
pid_tuner tuner = pid_tuner(pid);

//creates pwm instance
RP2040_PWM* PWM1_Instance;
RP2040_PWM* PWM2_Instance;

LCD lcd(LCD_RS, LCD_Not_Use_Port_RW, LCD_E, LCD_D7, LCD_D6, LCD_D5, LCD_D4);

// --------------GLOBAL VARIABLES---------

// variables to be declared as volatile since they're modified in ISRs
volatile long pulseCount = 0; 

// -------------- ENUMERATIONS --------


// ------------------------ S E T U P    C O D E    B E G I N ------------------------

void setup() {
    Serial.begin(115200);

    pinMode(ENCA_pin, INPUT); // ENCA
    pinMode(ENCB_pin, INPUT); // ENCB
    pinMode(A3, OUTPUT); // PWM_o

    // Count positive and negative edges encoder to achieve max 64CPR resolution
    attachInterrupt(digitalPinToInterrupt(ENCA_pin), A_posedge, RISING);
    attachInterrupt(digitalPinToInterrupt(ENCA_pin), A_negedge, FALLING);
    attachInterrupt(digitalPinToInterrupt(ENCB_pin), B_posedge, RISING);
    attachInterrupt(digitalPinToInterrupt(ENCB_pin), B_negedge, FALLING);

    // PWM setup w/ 0% DC
    PWM1_Instance = new RP2040_PWM(PWM1_pin, PWM_FREQ, 0);
    PWM2_Instance = new RP2040_PWM(PWM2_pin, PWM_FREQ, 0);

    tuner.setOutputRange(-100, 100);
    tuner.setTargetValue(20); // mm
    tuner.setCycles(100);
    tuner.setTimeout(30000); // 30 seconds
    tuner.setTuningMode(pid_tuner::NO_OVERSHOOT);

    tuner.start();

    pulseCount = 0;

        // LCD setup
    lcd.init();
    lcd.displaySwitch(true, true, false);
    lcd.inputSet(true, false);
    lcd.clear();


}


// ------------------------ M A I N    C O D E    B E G I N ------------------------

void loop() {
    static unsigned long prev_pid_time = 0;
    double input = pulseCount * RAD_PER_PULSE * KJT * 1000.0; 

    if (!tuningFinished) {
        if (millis() - prev_pid_time >= 1) { // 1ms = 1000Hz
            prev_pid_time = millis();

            if (!tuner.isDone()) {
                double output = tuner.update(input);
                set_PWM(-output);
                
                sprintf(LCD_BUFFER, "input = %1.1lf", input);
                LCD_Log(LCD_BUFFER, 2);
            } else {
                
                tuningFinished = true;

                // Get results
                double* constants = tuner.getConstants(); // [Kp, Ki, Kd]

                set_PWM(0);
                sprintf(LCD_BUFFER, "p: %0.4lf", constants[0]);
                LCD_Log(LCD_BUFFER, 1);
                sprintf(LCD_BUFFER, "i: %0.4lf", constants[1]);
                LCD_Log(LCD_BUFFER, 2);

                delay(5000);
                

                sprintf(LCD_BUFFER, "d:%0.4lf", constants[2]);
                LCD_Log(LCD_BUFFER, 2);
            
            }
        }
    }
}

    

// ------------------------ F U N C T I O N S    B E G I N ------------------------

void set_PWM(float output) {
    // sprintf(MSG_BUFFER, "output= %0.2f", output);

    if (output > 0) set_right_PWM(output);
    else            set_left_PWM(abs(output));
}

void set_left_PWM(int pwm_dc) {
    PWM1_Instance->setPWM(PWM1_pin, PWM_FREQ, 100 - pwm_dc);
    PWM2_Instance->setPWM(PWM2_pin, PWM_FREQ, 100);

}

void set_right_PWM(int pwm_dc) {
    PWM1_Instance->setPWM(PWM1_pin, PWM_FREQ, 100);
    PWM2_Instance->setPWM(PWM2_pin, PWM_FREQ, 100 - pwm_dc);

}

double real_abs (double x){
    if (x > 0) {
        return x;
    } else {
        return -x;
    }
}

void LCD_Log(char *MSG, int line_num) {
    if (line_num == 1) {
        lcd.setDataAddr(LCD_Line1Start);
    }
    else if (line_num == 2) {
        lcd.setDataAddr(LCD_Line2Start);
    }
    lcd.writeData(MSG);
}
