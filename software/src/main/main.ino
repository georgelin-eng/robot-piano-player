#define DRV8263H

#include <stdio.h>       // Standard IO
#include "RP2040_PWM.h"  // PWM
#include "pins.h"        // pin to variable mappings
#include "commands.h"    // command table
#include "peripherals.h" // ISRs for interacting with peripherals

#define RAD_PER_PULSE 0.0981747704247*4  // 2pi / 64.
#define KJT 0.00097000000000000005055  // joint-to-task space: 1:10 gearbox + 17.4mm diameter pulley 
#define KTJ 1030.9278350515462535      // task-to-joint space

#define POS_ERR_THRS   (15 * 0.001)     // 3mm precision
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

#define GLOBAL_LOG_LEVEL LOG_HIGH
#define TEST_MODE_ENABLED 1

char MSG_BUFFER[64]; // global message buffer for serial logging

//creates pwm instance
RP2040_PWM* PWM1_Instance;
RP2040_PWM* PWM2_Instance;

// Motor control variables
volatile long pulseCount = 0;
volatile long prev_pulseCount;
volatile long diff_pulseCount;
volatile long diff_pulseCount_filtered;
volatile int Beta = 0.2759;
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
eTestCase  test_case = position_drift_test;

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
    attachInterrupt(digitalPinToInterrupt(ENCA_pin), A_posedge, RISING);
    attachInterrupt(digitalPinToInterrupt(ENCA_pin), A_negedge, FALLING);
    attachInterrupt(digitalPinToInterrupt(ENCB_pin), B_posedge, RISING);
    attachInterrupt(digitalPinToInterrupt(ENCB_pin), B_negedge, FALLING);

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
    const static double control_interval = 1/80.0;

    static double measured_absolute_angle_rad;
    static double wanted_absolute_angle_rad;
    static double error;
    static double dError;
    static double error_sum;
    static double prev_error;
    static double output;
    static int first_entry ; 

    int pwm_dc;    

    double speed;

    measured_absolute_angle_rad = pulseCount * RAD_PER_PULSE;
    // TODO: Might need gain scheduling for left vs right side response
    // Linear interpolation of PID not needed. Can use basic piecewaise gain schedule

    const static struct K_PID PID_left = 
    {
        .Kp = 0.0746,
        .Ki = 0.0033,
        .Kd = 0.0044
    };

    const static struct K_PID PID_right = 
    {
        .Kp = 0.0746,
        .Ki = 0.0033,
        .Kd = 0.0044
    };

    // --------- FSM BEGIN ---------
    switch(state) {
        case(IDLE):
            pwm_dc = 0;
            first_entry = 0;
            if (TEST_MODE_ENABLED) {
                state = TEST;
            } else {
                state = IDLE;
            }

            break;
        case(TEST):
            Log("FSM", "Starting Test...", LOG_LOW);

            error_sum = 0;

            switch (test_case) {
                case (spin_direction_home_test):
                    Log("FSM", "Starting spin_direction_home_test", LOG_LOW);
                    state = MOVE_LEFT_HOME;
                    break;
                case (soft_start_test):
                    state = LEFT_SOFT_RAMP;
                    break;
                case (position_drift_test):
                    Log("FSM", "Starting position_drift_test", LOG_LOW);

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

                if (pwm_dc < 100) {
                    pwm_dc = pwm_dc + 1;
                    sprintf(MSG_BUFFER, "pwm_dc == %d", pwm_dc);
                    Log("FSM", MSG_BUFFER, LOG_MEDIUM);

                    // 20Khz - 10% DC
                    set_left_PWM(pwm_dc);

                }
                else {
                    state = HARD_STOP;
                }
            }
            break;
        
        case(MOVE_LEFT_HOME):
            //Start PWM 1, set PWM 2 to 3.3V
            set_left_PWM(20);
            Log("FSM", "Moving left", LOG_MEDIUM);

            delay(move_interval);
            state = MOVE_RIGHT_HOME;
            break;
        case(MOVE_RIGHT_HOME):
            //start PWM2, set PWM 1 to 3.3V
            set_right_PWM(20);
            Log("FSM", "Moving right", LOG_MEDIUM);

            delay(move_interval);
            state = MOVE_LEFT_HOME;
            break;

        case (HARD_STOP):
            set_left_PWM(0);
            set_right_PWM(0);

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

                if (pwm_dc > 0) {
                    pwm_dc = pwm_dc - 1;

                    sprintf(MSG_BUFFER, "pwm_dc == %d", pwm_dc);
                    Log("FSM", MSG_BUFFER, LOG_MEDIUM);

                    set_left_PWM(pwm_dc);
                    set_right_PWM(pwm_dc);
                }
            }
            break;
            
        case(MOVE_RIGHT_PID):
            wanted_absolute_angle_rad = 0.15 * KTJ;
            wanted_absolute_angle_rad = 420;

            if (first_entry == 0)  {
                error = wanted_absolute_angle_rad - measured_absolute_angle_rad;
                first_entry = 1;
            }

            if(millis() - prev_move_right_time >= control_interval*1e3){
                prev_move_right_time = millis();

                error = wanted_absolute_angle_rad - measured_absolute_angle_rad;
                diff_pulseCount = pulseCount - prev_pulseCount;
                diff_pulseCount_filtered = (1-Beta)*diff_pulseCount_filtered + Beta*diff_pulseCount;
                speed = RAD_PER_PULSE*diff_pulseCount_filtered/control_interval;
                prev_pulseCount = pulseCount;

                //Get initial error signal

                //Calculate integral and different terms (converting control interval into seconds)
                dError = (error - prev_error) / control_interval; 
                error_sum = error_sum + error*control_interval;
                if (abs(error_sum) > 174) error_sum / abs(error_sum) * 174;

                output = PID_right.Kp*error + PID_right.Ki*error_sum + PID_right.Kd*dError;
                prev_error = error;

                if (output > 1.0) output = 1.0;
                else if (output < -1.0)  output = -1.0;

                if (output > 0) set_right_PWM((int)output*100);
                else set_left_PWM((int) abs(output)*100);


                if (abs(error) < ANGLE_ERR_THRS) {
                    state = MOVE_LEFT_PID;
                    error_sum = 0;
                    prev_error = 0;
                    first_entry = 0;
                    set_left_PWM((int) abs(0*100));
                    set_right_PWM((int) (0*100));
                    delay(1000);
                }

                sprintf(MSG_BUFFER, "wanted = %0.5f, measured=%0.5f, pulseCount=%ld", wanted_absolute_angle_rad, measured_absolute_angle_rad, pulseCount);
                Log("MOVE_RIGHT_PID", MSG_BUFFER, LOG_MEDIUM);

                sprintf(MSG_BUFFER, "output = %d, error=%0.5f, error_sum=%0.5f, dError=%0.5f", (int) output*100, error, error_sum, dError);
                Log("MOVE_RIGHT_PID", MSG_BUFFER, LOG_HIGH);
            }

            break;
        
        case(MOVE_LEFT_PID):
            // wanted_absolute_angle_rad = -0.15 * KTJ;
            wanted_absolute_angle_rad = 110;
    
            if (first_entry == 0){
                error = wanted_absolute_angle_rad - measured_absolute_angle_rad;
                first_entry = 1;
            }
            if(millis() - prev_move_left_time >= control_interval*1e3){ 

                prev_move_left_time = millis();

                error = wanted_absolute_angle_rad - measured_absolute_angle_rad;
          
                diff_pulseCount = pulseCount - prev_pulseCount;
                diff_pulseCount_filtered = (1-Beta)*diff_pulseCount_filtered + Beta*diff_pulseCount;
                speed = RAD_PER_PULSE*diff_pulseCount_filtered/control_interval;
                prev_pulseCount = pulseCount;

                //Get initial error signal
                error = wanted_absolute_angle_rad - measured_absolute_angle_rad;

                //Calculate integral and different terms (converting control interval into seconds)
                dError = (error - prev_error) / (control_interval); 

                // if (speed > 1.0) {
                    error_sum = error_sum + error*control_interval;
                // }
                if (abs(error_sum) > 174) error_sum / abs(error_sum) * 174;

                output = PID_left.Kp*error + PID_left.Ki*error_sum + PID_left.Kd*dError;
                prev_error = error;

                if (output > 1.0) output = 1.0;
                else if (output < -1.0) output = -1.0;

                if(output > 0) set_right_PWM((int) (output*100));
                else set_left_PWM((int) abs(output*100));

                if (abs(error) < ANGLE_ERR_THRS) {
                    state = MOVE_RIGHT_PID;
                    // state = MOVE_LEFT_PID;
                    prev_error = 0;
                    error_sum = 0;
                    first_entry = 0;
                    set_left_PWM((int) abs(0*100));
                    set_right_PWM((int) (0*100));
                    delay(1000);
                } 

                sprintf(MSG_BUFFER, "wanted = %0.5f, measured=%0.5f, pulseCount=%ld", wanted_absolute_angle_rad, measured_absolute_angle_rad, pulseCount);
                Log("MOVE_LEFT_PID(1)", MSG_BUFFER, LOG_MEDIUM);

                sprintf(MSG_BUFFER, "output = %d, error=%0.5f, error_sum=%0.5f, dError=%0.5f", (int) output*100, error, error_sum, dError);
                Log("MOVE_LEFT_PID", MSG_BUFFER, LOG_HIGH);
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

/*
    When using DRV8263H board, DRVOFF needs to be attached to 3.3V and sleep to GND
    Treat PWM2_pin as PH pin for left and right control (1/0) and PWM1 as main (PWM)

    When using Hbridge, optoisolators are referenced to 3.3V so inverse of DC is required
    Left vs right control is achieved by difference between PWM1 and PWM2. 
    For simplicity the opposite side is completely turned off via setting PWM to 100
*/
void set_left_PWM(int pwm_dc) {
    #ifdef DRV8263H 
        PWM1_Instance->setPWM(PWM1_pin, PWM_FREQ, pwm_dc);
        PWM2_Instance->setPWM(PWM2_pin, PWM_FREQ, 0);
    #else
        PWM1_Instance->setPWM(PWM1_pin, PWM_FREQ, 100);
        PWM2_Instance->setPWM(PWM2_pin, PWM_FREQ, 100 - pwm_dc);
    #endif
}

void set_right_PWM(int pwm_dc) {
    #ifdef DRV8263H 
        PWM1_Instance->setPWM(PWM1_pin, PWM_FREQ, 0);
        PWM2_Instance->setPWM(PWM2_pin, PWM_FREQ, pwm_dc);
    #else
        PWM1_Instance->setPWM(PWM1_pin, PWM_FREQ, 100 - pwm_dc);
        PWM2_Instance->setPWM(PWM2_pin, PWM_FREQ, 100);
    #endif
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
