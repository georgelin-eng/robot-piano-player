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
    //Printing for encoder
    static unsigned long lastLogTime = 0;
    const unsigned long logInterval  = 500; // ms

    double absolute_angle_rad;

    int kp = 1;
    int ki = 1;
    int kd = 1;

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
    /*
    struct command {
        uint_8 action;
        uint_16 solenoid_or_position;
        float start_time;
        float end_time;
    };

    <ACTION, SOLENOID_OR_POSITION, START_TIME, END_TIME >
        
    ACTION (uint_8)
    RIGHT_MOVE
    RIGHT_PLAY
    LEFT_PLAY
        
    SOLENOID_OR_POSITION (uint_16)
    SOLENOID 
    If ACTION is a PLAY command then decode the individual bits to specify if a solenoid is on or off. 
    Using uint_16 allows this to work for up to 16 solenoids per hand

    POSITION
    If ACTION is a MOVE command then change the setpoint. 
    Either parsed as an absolute position in cm or as a MIDI note pitch that gets mapped to a table of floats if key widths don’t fit well into integers
    start_time (float)
    Pathfinding should convert time to fixed-point value based on MCU frequency
    The PID control loop will prevent next command execution until position settles to <5mm of final. After this, PWM is set to 0. 
    Relative time that this command will take. New commands will only execute after this timer has run out 

    end_time (float)
    */

    double wanted_absolute_angle_rad;
    double error;
    double dError;
    double error_sum;
    double prev_error;
    int start_millis = millis();

    unsigned long currentMicros;
    unsigned long previousMicros = 0;

    unsigned long freq = 80; //FREQUENCY IN HZ
    unsigned long control_interval = 1/freq; //DELAY INTERVAL REQUIRED FOR THIS FREQUENCY
    for (int i = 0; i <= str.length(command); i++){
        //RESET FINGERS AT START OF NEXT ACTION OR END OF PREVIOUS???
        //Do WE WANT A SETTLE TIME FOR RETRACTION FO SOLENOID TO STOP MOVEMENT DURING OSCILLATIONgi


        //Start parsing actions
        if (command[i].action == RIGHT_MOVE){
            wanted_absolute_angle_rad = action[i].position;
            //Res 64 -> 360 deg 
            // PulseCount * 64/360 = angle change
            absolute_angle_rad = pulseCount * 64/2pi;
            error = wanted_absolute_angle_rad - absolute_angle_rad;
        }

        else if(command[i].action == RIGHT_PLAY){
            //PARSE RH PLAYING COMMAND BY OUTPUTTING CORRECT ADDRESSES


            // NO NEED FOR PID CONTROL
            error = 0;
        }
        else if(command[i].action == LEFT_PLAY){                     
           //PARSE LH PLAYING COMMAND BY OUTPUTTING CORRECT ADDRESSES
            // NO NEED FOR PID CONTROL
            error = 0;
        }


        //Envelope with time constraint of action lenght? 
        //millis/1000 -> convert both to secodns.
        while (abs(error) >= ???????) {
            //Res 64 -> 360 deg 
            // PulseCount * 64/360 = angle change
            absolute_angle_rad = pulseCount * 64/2pi;
            currentMicros = micros();
            if(currentMicros - previousMicros >= control_interval){
                previousMicros = currentMicros;
                
                //Get initial error signal
                error = wanted_absolute_angle_rad - absolute_angle_rad;

                //Calculate integral and different terms (converting control interval into seconds)
                dError = (error - prev_error) / (control_interval*10**(-6)); 
                error_sum+= error_sum*control_interval*10**(-6);
                output = kp*error + ki*error_sum + kd*dError;
                prev_error = error;
            }
        }

        while (millis()/1000.0 < command[i].end_time + (float)start_millis){
            //Wait till we are ready for the next instruction.
        }
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
