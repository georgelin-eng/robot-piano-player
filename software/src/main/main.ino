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
RP2040_PWM* PWM1_Instance;
RP2040_PWM* PWM2_Instance;

// Motor control variables
volatile long pulseCount = 0;
volatile long prev_pulseCount;
volatile long diff_pulseCount;
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
    MOVE_LEFT_HOME,
    MOVE_RIGHT_HOME,
    LEFT_SOFT_RAMP,
    RIGHT_SOFT_RAMP,
    MOVE_LEFT_PID,
    MOVE_RIGHT_PID,
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

struct K_PID {
    float Kp;
    float Ki;
    float Kd;
};

// ------------------------ S E T U P    C O D E    B E G I N ------------------------

void setup() {
    pinMode(A0, INPUT); // current_sense_i
    pinMode(A1, INPUT); // ENCA_i
    pinMode(A2, INPUT); // ENCB_i
    pinMode(A3, OUTPUT); // PWM_o
    pinMode(4, INPUT); // prox_sens_i
    pinMode(5, OUTPUT); // fault_detected_n
    pinMode(6, OUTPUT); // PWM_dir_o

    // Count positive and negative edges encoder to achieve max 64CPR resolution
    attachInterrupt(digitalPinToInterrupt(A1), A_posedge, RISING);
    attachInterrupt(digitalPinToInterrupt(A1), A_negedge, FALLING);
    attachInterrupt(digitalPinToInterrupt(A2), B_posedge, RISING);
    attachInterrupt(digitalPinToInterrupt(A2), B_negedge, FALLING);

    // PWM setup w/ 0% DC
    PWM1_Instance = new RP2040_PWM(PWM1_pin, PWM_FREQ, 0);
    PWM2_Instance = new RP2040_PWM(PWM2_pin, PWM_FREQ, 0);

    state = IDLE;
}

// ------------------------ M A I N    C O D E    B E G I N ------------------------

// TODO: Get timers working and start logging encoder reads
void loop() {

    //Printing for encoder
    static unsigned long prev_log_time = 0;
    const static unsigned long log_interval  = 500; // ms

    static unsigned long prev_ramp_time = 0;
    const static unsigned long ramp_interval = 10; // ms

    static unsigned long prev_move_time = 0;
    const static unsigned long move_interval = 500; // ms
    
    static unsigned long prev_move_right_time = 0;
    static unsigned long prev_move_left_time = 0;
    const static unsigned long control_interval = 1/(80*1e-3);

    double measured_absolute_angle_rad;
    double wanted_absolute_angle_rad;
    double error;
    double dError;
    double error_sum;
    double prev_error;
    double output;

    uint8_t pwm1_dc;    
    uint8_t pwm2_dc;    

    double speed;

    measured_absolute_angle_rad = pulseCount * RAD_PER_PULSE;
    // TODO: Might need gain scheduling for left vs right side response
    // Linear interpolation of PID not needed. Can use basic piecewaise gain schedule

    const static struct K_PID PID_left = 
    {
        .Kp = 0.0054,
        .Ki = 0.00015645,
        .Kd = 0.00098194
    };

    const static struct K_PID PID_right = 
    {
        .Kp = 0.0054,
        .Ki = 0.00015645,
        .Kd = 0.00098194
    };

    // --------- FSM BEGIN ---------
    switch(state) {
        case(IDLE):
            pwm1_dc = 0;
            pwm2_dc = 0;

            if (TEST_MODE_ENABLED) {
                state = TEST;
            } else {
                state = IDLE;
            }

            break;
        case(TEST):
            Log("FSM", "Starting Test...", LOG_LOW);

            switch (test_case) {
                case (spin_direction_home_test):
                    Log("FSM", "Starting spin_direction_home_test", LOG_LOW);
                    state = MOVE_LEFT_HOME;
                    break;
                case (soft_start_test):
                    state = LEFT_SOFT_RAMP;
                    break;
                case (position_drift_test):
                    state = MOVE_LEFT_PID;
                    break;
                case (motor_intertia_test):
                    Log("FSM", "Starting motor_intertia_test", LOG_LOW);

                    // state = SLOW_RAMP;
                    state = HARD_STOP;
                    break;
                default:
                    state = ERROR;
            }
        case (SLOW_RAMP):
            // Increase PWM DC to 100% +1% every 10ms
            if (millis() - prev_ramp_time >= ramp_interval) {
                prev_ramp_time = millis();

                if (pwm1_dc < 100) {
                    pwm1_dc = pwm1_dc + 1;
                    sprintf(MSG_BUFFER, "pwm1_dc == %d", pwm1_dc);
                    Log("FSM", MSG_BUFFER, LOG_MEDIUM);

                    // 20Khz - 10% DC
                    PWM1_Instance->setPWM(PWM1_pin, PWM_FREQ, 100 - pwm1_dc);
                    PWM2_Instance->setPWM(PWM2_pin, PWM_FREQ, 100);

                }
                else {
                    state = HARD_STOP;
                }
            }
            break;
        
        case(MOVE_LEFT_HOME):
            //Start PWM 1, set PWM 2 to 3.3V
            PWM1_Instance->setPWM(PWM1_pin, PWM_FREQ, 100);
            PWM2_Instance->setPWM(PWM2_pin, PWM_FREQ, 80);
            Log("FSM", "Moving left", LOG_MEDIUM);

            delay(move_interval);
            state = MOVE_RIGHT_HOME;
            break;
        case(MOVE_RIGHT_HOME):
            //start PWM2, set PWM 1 to 3.3V
            PWM1_Instance->setPWM(PWM1_pin, PWM_FREQ, 80);
            PWM2_Instance->setPWM(PWM2_pin, PWM_FREQ, 100);
            Log("FSM", "Moving right", LOG_MEDIUM);

            delay(move_interval);
            state = MOVE_LEFT_HOME;
            break;

        case (HARD_STOP):
            pwm1_dc = 0;
            pwm2_dc = 0;
            PWM1_Instance->setPWM(PWM1_pin, PWM_FREQ, 100-pwm1_dc);
            PWM2_Instance->setPWM(PWM2_pin, PWM_FREQ, 100-pwm2_dc);

            if (millis() - prev_log_time >= 10) {
                prev_log_time = millis();
                diff_pulseCount = pulseCount - prev_pulseCount;

                speed = RAD_PER_PULSE*diff_pulseCount/10.0 * 1000;

                sprintf(MSG_BUFFER, "speed(rad/s) == %0.5f", speed);
                //sprintf(MSG_BUFFER, "pulseCount == %ld", pulseCount);
                Log("FSM", MSG_BUFFER, LOG_MEDIUM);

                prev_pulseCount = pulseCount;

                // if (speed < 0.05) {
                    // state = SLOW_RAMP;
                // }
            }

            // display 
            break;

        case (SOFT_STOP):
            // Decrease PWM DC to 0%  -1% every 10ms. 
            if (millis() - prev_ramp_time >= ramp_interval) {
                prev_ramp_time = millis();

                if (pwm1_dc > 0 && pwm2_dc > 0) {
                    pwm1_dc = pwm1_dc - 1;
                    pwm2_dc = pwm2_dc - 1;

                    sprintf(MSG_BUFFER, "pwm1_dc == %d, pwm2_dc == %d", pwm1_dc, pwm2_dc);
                    Log("FSM", MSG_BUFFER, LOG_MEDIUM);

                    PWM1_Instance->setPWM(PWM1_pin, PWM_FREQ, 100 - pwm1_dc);
                    PWM2_Instance->setPWM(PWM2_pin, PWM_FREQ, 100 - pwm2_dc);
                }
            }
            break;
            
        case(MOVE_RIGHT_PID):
            if (abs(error) < 0.01) state = MOVE_LEFT_PID;

            if(millis() - prev_move_right_time >= control_interval){
                prev_move_right_time = millis();

                //Get initial error signal
                error = wanted_absolute_angle_rad - measured_absolute_angle_rad;

                //Calculate integral and different terms (converting control interval into seconds)
                dError = (error - prev_error) / (control_interval*1e-3); 
                error_sum+= error_sum*control_interval*1e-6;
                output = PID_right.Kp*error + PID_right.Ki*error_sum + PID_right.Kd*dError;
                prev_error = error;

            }

            break;
        
        case(MOVE_LEFT_PID):
            if (abs(error) < 0.01) state = SLOW_RAMP;

            if(millis() - prev_move_right_time >= control_interval){ 
                prev_move_right_time = millis();

                //Get initial error signal
                error = wanted_absolute_angle_rad - measured_absolute_angle_rad;

                //Calculate integral and different terms (converting control interval into seconds)
                dError = (error - prev_error) / (control_interval*1e-6); 
                error_sum+= error_sum*control_interval*1e-6;
                output = PID_left.Kp*error + PID_left.Ki*error_sum + PID_left.Kd*dError;
                prev_error = error;
            }
            
            break;
        case(ERROR):
            state = ERROR;
            break;
        default:
            Log ("FSM", "Unhandled State!", LOG_NONE);
            state = ERROR;
    }

    // if (millis() - prev_log_time >= log_interval) {
    //     prev_log_time = millis();

    //     noInterrupts();
    //     sprintf(MSG_BUFFER, "pulseCount == %ld", pulseCount);
    //     Log("ENCODER", MSG_BUFFER, LOG_MEDIUM);

    //     interrupts();
    // }
  
    /*

    <ACTION, SOLENOID_OR_POSITION, START_TIME, END_TIME >
        
    ACTION (uint8_t)
    RIGHT_MOVE
    RIGHT_PLAY
    LEFT_PLAY
        
    SOLENOID_OR_POSITION (uint16_t)
    SOLENOID 
    If ACTION is a PLAY command then decode the individual bits to specify if a solenoid is on or off. 
    Using uint16_t allows this to work for up to 16 solenoids per hand

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
