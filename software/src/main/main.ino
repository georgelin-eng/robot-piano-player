#include <stdio.h>       // Standard IO
#include "RP2040_PWM.h"  // PWM
#include "pins.h"        // pin to variable mappings
#include "commands.h"    // command table
#include "peripherals.h" // ISRs for interacting with peripherals

#define RAD_PER_PULSE 0.0981747704247  // 2pi / 64.
#define KJT 0.00097000000000000005055  // joint-to-task space: 1:10 gearbox + 17.4mm diameter pulley 
#define KTJ 1030.9278350515462535      // task-to-joint space

#define POS_ERR_THRS   (3 * 0.001)     // 3mm precision
#define ANGLE_ERR_THRS (POS_ERR_THRS * KTJ)

#define PWM_FREQ 20000 // 20KHz

// Logging parameters
enum eLogLevel {
    LOG_NONE   = 0,
    LOG_LOW    = 100,
    LOG_MEDIUM = 200,
    LOG_HIGH   = 300,
    LOG_FULL   = 400,
    LOG_DEBUG  = 500
};

#define GLOBAL_LOG_LEVEL LOG_MEDIUM
#define TEST_MODE_ENABLED 1

char MSG_BUFFER[64]; // global message buffer for serial logging

//creates pwm instance
RP2040_PWM* PWM_Instance;

// Motor control variables
volatile long pulseCount = 0;
volatile float ANGLE;
volatile float POSITION;

// FSM 
enum eFSM_STATE {
    IDLE,
    HOME,
    RUN,
    PAUSE, 
    ERROR,
    POWERDOWN,

    // Tests for software bringup
    TEST, 
    SOFT_STOP,
    HARD_STOP,
    MOVE_CW_HOME,
    MOVE_CCW_HOME,
    CW_SOFT_RAMP,
    CCW_SOFT_RAMP,
    MOVE_CW_PID,
    MOVE_CCW_PID,
    SLOW_RAMP
};

  
enum eTestCase {
    spin_direction_home_test,
    soft_start_test,
    position_drift_test,
    motor_intertia_test
};

eFSM_STATE state = IDLE;
eTestCase  test_case = motor_intertia_test;

// ------------------------ S E T U P    C O D E    B E G I N ------------------------

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

    // PWM setup w/ 0% DC
    PWM_Instance = new RP2040_PWM(PWM_pin, PWM_FREQ, 0);

}

// ------------------------ M A I N    C O D E    B E G I N ------------------------

// TODO: Get timers working and start logging encoder reads
void loop() {

    //Printing for encoder
    static unsigned long lastLogTime = 0;
    const unsigned long logInterval  = 500; // ms

    double absolute_angle_rad;
    uin8_t pwm_dc;    

    float kp = 0.003100008993921;
    float ki = 0.0000410086921359;
    float kd = 0.001340560346924;

    // --------- FSM BEGIN ---------
    switch(state) {
        case(IDLE):
            if (TEST_MODE_ENABLED) {
                pwm_dc = 0;
                state = TEST;
            }
            break;
        case(TEST):
            switch (test_case) {
                case (spin_direction_home_test):
                    state = MOVE_CW_HOME;
                    break;
                case (soft_start_test):
                    state = CW_SOFT_RAMP;
                    break;
                case (position_drift_test):
                    state = MOVE_CW_PID;
                    break;
                case (motor_intertia_test):
                    state = SLOW_RAMP;
                    break;
            }
        case (SLOW_RAMP):
            // Increase PWM DC to 100% +1% every 10ms
            PWM_Instance->setPWM(PWM_pin, PWM_FREQ, pwm_dc);

            if (millis() - lastLogTime >= logInterval) {
                lastLogTime = millis();
                pwm_dc = pwm_dc - 1;

                sprintf(MSG_BUFFER, "pulseCount == %ld", pulseCount);
                Log("ENCODER", MSG_BUFFER, LOG_MEDIUM);

                // 20Khz - 10% DC
                PWM_Instance->setPWM(PWM_pin, PWM_FREQ, 10);

            }


        case (HARD_STOP):
            PWM_Instance->setPWM(PWM_pin, PWM_FREQ, 0);

        case (SOFT_STOP):
            // Decrease PWM DC to 0%  -1% every 10ms. 
            PWM_Instance->setPWM(PWM_pin, PWM_FREQ, pwm_dc);

        case(ERROR):
            state = ERROR;
        default:
            Log ("FSM", "Unhandled State!", LOG_NONE);
            state = ERROR;
    }

    if (millis() - lastLogTime >= logInterval) {
        lastLogTime = millis();

        noInterrupts();
        sprintf(MSG_BUFFER, "pulseCount == %ld", pulseCount);
        Log("ENCODER", MSG_BUFFER, LOG_MEDIUM);

        // 20Khz - 10% DC
        PWM_Instance->setPWM(PWM_pin, PWM_FREQ, 10);

        interrupts();
    }
  
    /*

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

    /*

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
            absolute_angle_rad = pulseCount * RAD_PER_PULSE;
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
    */
}

void Log(const char* ID, const char *MSG, enum eLogLevel level) {
    if (level > GLOBAL_LOG_LEVEL) return;

    // Equivalent to 
    // printf("[%s], %s\n", ID, MSG)
    Serial.print("[");
    Serial.print(ID);
    Serial.print("], ");
    Serial.println(MSG);
}
