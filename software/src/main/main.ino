#include <stdio.h>             // Standard IO
#include "RP2040_PWM.h"        // PWM
#include "pins.h"              // pin to variable mappings
// #include "commands.h"          // command table
#include "stiction_map.h"          // command table
//#include "cust_commands.h"        // command table
#include "led_commands.h" 
#include "peripherals.h"       // ISRs for interacting with peripherals
#include "PID.h"               // PID functions
#include "logging.h"           // logging functions
#include <Adafruit_MCP23X17.h> // I2C expander library

#include <LCD1602.h>           // LCD library
#include "I2CScanner.h"        // I2C device scanning

#include <string>

// ----------- DEFINITIONS --------------

#define RAD_PER_PULSE 0.0981747704247*2// 2pi / 64.
#define KJT 0.00097000000000000005055  // joint-to-task space: 1:10 gearbox + 17.4mm diameter pulley 
#define KTJ 1030.9278350515462535      // task-to-joint space

#define POS_ERR_THRS (7/1000.0) //   (10 /1000.0)
#define ANGLE_ERR_THRS (POS_ERR_THRS * KTJ)

#define PWM_FREQ 20000 // 20KHz
#define PID_CONTROL_FREQ 1000.0
#define PID_CONTROL_INTERVAL (1 / PID_CONTROL_FREQ)
// #define PID_CONTROL_INTERVAL 0.0125
#define TARGET_HOME_SPEED 3.0 // cm per second

// PID parameters
#define K0 0.6// 0.6 works good

// Small movement PID values
#define PID_S_KP (0.0567 * K0 ) * 0.04// (0.0567 * K0 * 0.174) * 1.49999// * 0.138
#define PID_S_KI (0.00067091 * K0 ) * 1.45//(0.00067091 * K0 * 260) *0.00000021 // * 1.45
#define PID_S_KD (0.0011 * K0 )*0.01*0.25 //0.00000000087//(0.0011 * K0 * 0.00135)* 0//* 0.012

// large movement PID values
#define PID_L_KP (0.0567 * K0) * 0.123 // (0.0567 * K0 * 0.3) * 1.6 // * 0.138
#define PID_L_KI (0.00067091 * K0 ) * 450// (0.00067091 * K0 * 200) *0.000005  // * 1.45
#define PID_L_KD (0.0011 * K0)*0.01*0.32 //0.00000000087//(0.0011 * K0 * 0.00135)* 0//* 0.012 

#define PID_MIN_MOVE 40 // mm
#define PID_MAX_MOVE 80 // mm

#define PID_STICTION 0.01 // feedforward control for stiction (WIP)

#define PID_KAW 0.01 // Anti-integral windup gain (WIP)
// #define PID_BETA 0.2759 // mdp * 10 @ 80Hz
// #define PID_BETA 0.4211 // mdp * 5 @ 80Hz
#define PID_BETA 0.7800 // mdp * 10 @ 1kHz
#define PID_LIM_MIN_INT -0.5
#define PID_LIM_MAX_INT 0.5
#define PID_LIM_MIN -1.0
#define PID_LIM_MAX 1.0

#define PID_STICTION_MIDDLE  0.11 //0.059 * 1.03
#define PID_STICTION_SIDES 0.11 //0.04

// PID integral separation
#define PID_ERR_THRS_3 (40/1000.0 * KTJ)
#define PID_ERR_THRS_2 (20/1000.0 * KTJ) 
#define PID_ERR_THRS_1 (10/1000.0 * KTJ)

#define PID_SWITCH_COEFF3 0.1     // large error
#define PID_SWITCH_COEFF2 0.4   
#define PID_SWITCH_COEFF1 0.8
#define PID_SWITCH_COEFF0 1     // small error

// PID error debounce parameters
#define PID_ERROR_SETTLE_MS 50

#define FINGERS_IN_EXISTENCE 20

#define GLOBAL_LOG_LEVEL LOG_HIGH

static char MSG_BUFFER[64]; // global message buffer for serial logging
static char LCD_BUFFER[16]; // LCD buffer

// ------------- OBJECT INSTANCES -------------

// PID instance
PIDController PID;
K_GAIN K_large;
K_GAIN K_small;
IntegralCoeff IntCoeff;

//creates pwm instance
RP2040_PWM* PWM1_Instance;
RP2040_PWM* PWM2_Instance;

// Solenoid Driver board
Adafruit_MCP23X17 mcp_main;
Adafruit_MCP23X17 mcp_move;
I2CScanner scanner;

// LCD instance
LCD lcd(LCD_RS, LCD_Not_Use_Port_RW, LCD_E, LCD_D7, LCD_D6, LCD_D5, LCD_D4);

// --------------GLOBAL VARIABLES---------

// variables to be declared as volatile since they're modified in ISRs
volatile long pulseCount = 0; 
int move_found;

// -------------- ENUMERATIONS --------

enum eFSM_STATE {
    IDLE,
    HOME,
    RUN_INIT,
    RUN,
    PAUSE, 
    ERROR,
    DONE,
    POWERDOWN
};

eFSM_STATE state;

// ------------------------ S E T U P    C O D E    B E G I N ------------------------

void setup() {
    pinMode(A0, INPUT); // current_sense_i
    pinMode(ENCA_pin, INPUT); // ENCA
    pinMode(ENCB_pin, INPUT); // ENCB
    pinMode(A3, OUTPUT); // PWM_o
    pinMode(PROX_SENSE1, INPUT); // Proximity sensor1
    pinMode(PROX_SENSE2, INPUT); // Proximity sensor2

    // Count positive and negative edges encoder to achieve max 64CPR resolution

    // BUG: An interrupt source is handeled by one ISR. Calling again to ENCA_pin
    // overrides the previous attachInterrupt. We get 0.19mm resolution, not 0.1mm,
    // interrupts on CHANGE requires some more complicated bitmasking. This is why RAD_PER_PULSE
    // must be multiplied by 2 since we have 32 CPR instead of 64 as we miss rising edges. 
    // trade off at this point in development isn't worth it as it works for our purposes. 
    attachInterrupt(digitalPinToInterrupt(ENCA_pin), A_posedge, RISING);
    attachInterrupt(digitalPinToInterrupt(ENCA_pin), A_negedge, FALLING);
    attachInterrupt(digitalPinToInterrupt(ENCB_pin), B_posedge, RISING);
    attachInterrupt(digitalPinToInterrupt(ENCB_pin), B_negedge, FALLING);

    // PWM setup w/ 0% DC
    PWM1_Instance = new RP2040_PWM(PWM1_pin, PWM_FREQ, 0);
    PWM2_Instance = new RP2040_PWM(PWM2_pin, PWM_FREQ, 0);

    // PID gain schedule structs
    K_large.Kp = PID_L_KP;
    K_large.Ki = PID_L_KI;
    K_large.Kd = PID_L_KD;

    K_small.Kp = PID_S_KP;
    K_small.Ki = PID_S_KI;
    K_small.Kd = PID_S_KD;

    // PID setup
    PID.Kp = PID_L_KP;
    PID.Ki = PID_L_KI;
    PID.Kd = PID_L_KD;
    PID.min_move = PID_MIN_MOVE;
    PID.max_move = PID_MAX_MOVE;
    PID.Kaw = PID_KAW;
    PID.beta = PID_BETA;
    PID.limMin = PID_LIM_MIN;
    PID.limMax = PID_LIM_MAX;
    PID.limMaxInt = PID_LIM_MIN_INT;
    PID.limMinInt = PID_LIM_MAX_INT;
    PID.control_interval = PID_CONTROL_INTERVAL;
    PID.stiction = PID_STICTION;

    IntCoeff.beta0 = PID_SWITCH_COEFF0;
    IntCoeff.beta1 = PID_SWITCH_COEFF1;
    IntCoeff.beta2 = PID_SWITCH_COEFF2;
    IntCoeff.beta3 = PID_SWITCH_COEFF3;

    IntCoeff.err_thrs_1 = PID_ERR_THRS_1;
    IntCoeff.err_thrs_2 = PID_ERR_THRS_2;
    IntCoeff.err_thrs_3 = PID_ERR_THRS_3;

    state = IDLE;

    // LCD setup
    lcd.init();
    lcd.displaySwitch(true, true, false);
    lcd.inputSet(true, false);
    lcd.clear();

    Serial.print("\r\nScanning!\r\n");
    scanner.Init();
  
    scanner.Scan();
    if (!mcp_main.begin_I2C(32)) {
        sprintf(LCD_BUFFER, "Error on main board");
        LCD_Log(LCD_BUFFER, 1);

        Serial.print("Error on main board\r\n");

      while (1);
    }

    if (!mcp_move.begin_I2C(33)) {
        // Serial.print("Error on moving board\r\n");
        sprintf(LCD_BUFFER, "Error on moving board");
        LCD_Log(LCD_BUFFER, 2);
        move_found = 0;
    }
    
    move_found = 1;
    Serial.print("Found moving board\r\n");

    // I2C pinmode setups

    mcp_main.pinMode(SOLENOID_L_1, OUTPUT);
    mcp_main.pinMode(SOLENOID_L_2, OUTPUT);
    mcp_main.pinMode(SOLENOID_L_3, OUTPUT);
    mcp_main.pinMode(SOLENOID_L_4, OUTPUT);
    mcp_main.pinMode(SOLENOID_L_5, OUTPUT);
    mcp_main.pinMode(SOLENOID_L_6, OUTPUT);
    mcp_main.pinMode(SOLENOID_L_7, OUTPUT);
    mcp_main.pinMode(SOLENOID_L_8, OUTPUT);
    mcp_main.pinMode(SOLENOID_L_9, OUTPUT);
    mcp_main.pinMode(SOLENOID_L_10, OUTPUT);
    mcp_main.pinMode(SOLENOID_L_11, OUTPUT);
    mcp_main.pinMode(SOLENOID_L_12, OUTPUT);
    
    if (move_found) {
        mcp_move.pinMode(SOLENOID_R_0, OUTPUT);
        mcp_move.pinMode(SOLENOID_R_1, OUTPUT);
        mcp_move.pinMode(SOLENOID_R_2, OUTPUT);
        mcp_move.pinMode(SOLENOID_R_3, OUTPUT);
        mcp_move.pinMode(SOLENOID_R_4, OUTPUT);
        mcp_move.pinMode(SOLENOID_R_5, OUTPUT);
        mcp_move.pinMode(SOLENOID_R_6, OUTPUT);
        mcp_move.pinMode(SOLENOID_R_7, OUTPUT);
    }
}


// ------------------------ M A I N    C O D E    B E G I N ------------------------

// TODO: Get timers working and start logging encoder reads
void loop() {

    static double pid_output;
    static double measured_rad;
    static double prev_measured_rad;
    static double wanted_rad;
    static unsigned long prev_pid_time = 0;
    static double stiction_coeff;

    static double   pid_within_error_time;
    static int      pid_error_settle_first_time_entry = 1;
    static int      PID_move_size_mm;
    static int      PID_prev_setpoint_mm;

    // --- Dither Statics ---
    static double dither_val = 0.0;
    static double dither_dir = 1.0;

    static double song_start_time = 0; // time when song starts, used to track elapsed time for command scheduling
    static double song_start = 0;
    static double offset = 0;
    static double song_elapsed_time; // time elapsed since start of song
    static double action_start_time = 0; // start time of each action command
    static double action_end_time   = 0; // target end time of action
    static double PID_settle_time;
    static double worst_PID_settle_time = 0;
    static double song_play_time = 0;

    static int action_type;  // TODO: Make this an enum
    static int command_idx = 0;
    static uint16_t wanted_position;

    static double speed_cmps;
    static int pwm_dc = 0;

    // TODO: Might need gain scheduling for left vs right side response
    // Linear interpolation of PID not needed. Can use basic piecewaise gain schedule

    // --------- FSM BEGIN ---------
    switch(state) {
        case(IDLE):
            PIDController_Init(&PID);

            sprintf(LCD_BUFFER, "IDLE");
            LCD_Log(LCD_BUFFER, 1);
            delay(500);

            pwm_dc = 0;
            state = HOME;

            break;
        case(HOME):
            measured_rad = pulseCount * RAD_PER_PULSE;
            PIDController_Measure(&PID, measured_rad);

            speed_cmps = PID.d_measured * KJT * 100.0;

            // ramp up speed slowly. Just use if statement based control instead of tuning a PID control loop on speed
            if (real_abs(speed_cmps) < TARGET_HOME_SPEED) {
                pwm_dc = pwm_dc + 1;

                if (pwm_dc >= 20) {
                    pwm_dc = 20;
                }
            } else if (real_abs(speed_cmps) > TARGET_HOME_SPEED) {
                pwm_dc = pwm_dc - 1;

                if (pwm_dc < 0) {
                    pwm_dc = 0;
                }
            }

             set_right_PWM(pwm_dc);

            delay(10);

            // Use PID controller to get speed measurements (derivative of position)
            
            // sprintf(LCD_BUFFER, "HOME, dc=%0d", pwm_dc);
            sprintf(LCD_BUFFER, "HOME, dc=30");
            LCD_Log(LCD_BUFFER, 1);
            // sprintf(LCD_BUFFER, "rad=%0.2f, v=%0.2f", measured_rad, speed_cmps);
            // LCD_Log(LCD_BUFFER, 2);
            
            if (digitalRead(PROX_SENSE1) == 0) {
                state = RUN_INIT;
                pulseCount = 0; // Once we are finished homing set this position as 0
                 PIDController_Init(&PID);
            }
            break;

        case(RUN_INIT):
           

            command_idx = 0;    

            sprintf(LCD_BUFFER, "RUN_INIT");
            LCD_Log(LCD_BUFFER, 1);

            sprintf(LCD_BUFFER, "start=%0.1lf", song_start_time);
            LCD_Log(LCD_BUFFER, 2);


            wanted_rad = INITIAL_MOTOR_POSITION_MM * KTJ/1000.0;
            PIDController_GainSchedule(&PID, &K_large, &K_small, INITIAL_MOTOR_POSITION_MM);
            if(millis() - prev_pid_time  >= PID_CONTROL_INTERVAL*1e3){
                prev_pid_time = millis();
                measured_rad = pulseCount * RAD_PER_PULSE;

                pid_output = PIDController_Update(&PID, wanted_rad, measured_rad);
                
                set_PWM(-pid_output);

                if (real_abs(PID.error) < ANGLE_ERR_THRS) {
                    PIDController_Init(&PID);
                    set_PWM(0);
                    state = RUN;    
                }

                sprintf(LCD_BUFFER, "yd=%0.1f,ya=%0.1f", wanted_rad*KJT*1000, measured_rad*KJT*1000);
                LCD_Log(LCD_BUFFER, 1);
            }

            lcd.setDataAddr(LCD_Line1Start);
            lcd.clear();    
            lcd.setDataAddr(LCD_Line2Start);
            lcd.clear();    

            break;

        case(RUN):

            if (song_start != 1){
                song_start = 1;
                song_start_time = millis()/1000.0; // convert to seconds
                 PIDController_Init(&PID);

            }
            // Command parsing and the PID control loop happen at the same interval. 
            // at 80Hz this becomes a 12.5ms delay in command parsing. 
            
            // NOTE: Code is written for just one hand (RH)
            // play notes when we've hit start time of command
            // increment command index when we've reached the end time of the current command

            /*
                1. read command type (MOVE / PLAY)
                    1.1
                    Movement commands:

                    start time - 
                    end time -
                    error < X -> Parse next command

                    1.2
                    Play Command
                    start time -
                    Song Run time > start time -> Play
                    Decode using or command, actuate using for loop (i2c)
                    end time -> Parse next command
                    

            */
            
            song_elapsed_time = millis()/1000.0 - song_start_time - offset;

            action_type       = schedule[command_idx].action;
            action_start_time = schedule[command_idx].start_time;
            action_end_time   = schedule[command_idx].end_time;

            if (command_idx == SCHEDULE_LENGTH - 1) {
                lcd.setDataAddr(LCD_Line1Start);
                lcd.clear();    
                lcd.setDataAddr(LCD_Line2Start);
                lcd.clear();    
                state = DONE;

                song_play_time = millis()/1000.0 - song_start_time;
            }

            // PID loop
            if (action_type == MOVE){
                // sprintf(LCD_BUFFER, "%0d: R_MOVE", command_idx);
                // LCD_Log(LCD_BUFFER, 1);


                /*

                */
                // previous vs last target position determines the size of the movement
                // hence which set of PID values to use
                if (command_idx == 0) {
                    PID_move_size_mm = schedule[command_idx].solenoid_or_position - INITIAL_MOTOR_POSITION_MM;
                } else {
                    PID_move_size_mm = schedule[command_idx].solenoid_or_position - PID_prev_setpoint_mm;
                }

                 PIDController_GainSchedule(&PID, &K_large, &K_small, PID_move_size_mm);

                if(millis() - prev_pid_time  >= PID_CONTROL_INTERVAL*1e3){
                    prev_pid_time = millis();


                  //  PIDController_GainSchedule(&PID, &K_large, &K_small, PID.error*KJT*1000.0);
                    // --- GENERATE TRIANGULAR DITHER ---
                    /*
                        To decrease the effect of static friction, we add a
                        triangular dither to our output
                    */

                    double dither_amp = 0.10;  // Peak amplitude to break stiction
                    double dither_step = 0.02; // How fast the triangle wave moves per 1ms tick

                    dither_val += (dither_step * dither_dir);
                    if (dither_val >= dither_amp) {
                        dither_val = dither_amp;
                        dither_dir = -1.0; // Turn around and go down
                    } else if (dither_val <= -dither_amp) {
                        dither_val = -dither_amp;
                        dither_dir = 1.0;  // Turn around and go up
                    }

                    /*
                    PID error being too low can result in being stuck in this loop for a long time
                    This can cause subsequant play commands to get skipped. We fix this
                    be keeping song_elapsed_time the same value by subtracting an offset
                    
                    since millis() continuously grows, we need to use it in the calculation
                    of the offset as well. Offset should grow based on the sum of the total
                    time delta from when the PID loop allows things to end and action end time
                    */
                    if (song_elapsed_time >= action_end_time) {
                        offset = (millis()/1000.0 - song_start_time) - action_end_time;
                    }

                    wanted_rad = schedule[command_idx].solenoid_or_position * KTJ/1000.0;
                    prev_measured_rad = measured_rad;
                    measured_rad = pulseCount * RAD_PER_PULSE;

                    // calculate the error first for use in the loop
                    pid_output = PIDController_Update(&PID, wanted_rad, measured_rad);
                    // PIDController_IntegralUpdate(&PID, &IntCoeff);

                    if (real_abs(PID.error) < ANGLE_ERR_THRS && song_elapsed_time >= action_end_time) {
                        if (pid_error_settle_first_time_entry) {
                            pid_within_error_time = millis();
                            pid_error_settle_first_time_entry = 0;
                        }

                        /*
                            the first time the error is within threshold, we wait
                            for next instance after PID_ERROR_SETTLE_MS that 
                            the PID error is within threshold. There is always
                            the chance for an invalid exit since final settle
                            cannot be estimated while the PID is running

                            Works for the no OS case
                            Assumes that if there is OS case that oscilations
                            are within the error treshold
                        */

                        if (millis() - pid_within_error_time >= PID_ERROR_SETTLE_MS) {
                            PID_prev_setpoint_mm = schedule[command_idx].solenoid_or_position;

                            PID_settle_time = millis()/1000.0 - action_end_time - offset;

                            if(PID_settle_time > worst_PID_settle_time) {
                                worst_PID_settle_time = PID_settle_time;
                            }

                            command_idx++;
                            PIDController_Init(&PID);
                            set_PWM(0);

                            pid_error_settle_first_time_entry = 1;
                        }
                    }
                    else if (real_abs(PID.error) >= ANGLE_ERR_THRS) {
                        
                        if ((measured_rad*KJT*1000 >= 185 || measured_rad*KJT*1000 <= 65) && prev_measured_rad == measured_rad){
                            stiction_coeff = PID_STICTION_SIDES;
                        }
                        else if (prev_measured_rad == measured_rad){
                            stiction_coeff = PID_STICTION_MIDDLE;
                        }

                        else stiction_coeff = 0;

                        if (PID.error > 0) set_PWM(-(pid_output + stiction_coeff));
                        else set_PWM(-(pid_output - stiction_coeff));
                        
                        pid_error_settle_first_time_entry = 1;
                        pid_within_error_time = millis();
                        
                        sprintf(LCD_BUFFER, "yd=%0.1f,ya=%0.1f", wanted_rad*KJT*1000, measured_rad*KJT*1000);
                        LCD_Log(LCD_BUFFER, 1);
    
                        if (abs(PID_move_size_mm) < PID_MIN_MOVE) {
                            sprintf(LCD_BUFFER, "id%0d: S, %0d", command_idx, PID_move_size_mm);
                            LCD_Log(LCD_BUFFER, 2);
                        }
                        else if (abs(PID_move_size_mm) < PID_MAX_MOVE) {
                            sprintf(LCD_BUFFER, "id%0d: M, %0d", command_idx, PID_move_size_mm);
                            LCD_Log(LCD_BUFFER, 2);
                        } else {
                            sprintf(LCD_BUFFER, "id%0d: L, %0d", command_idx, PID_move_size_mm);
                            LCD_Log(LCD_BUFFER, 2);
                        }
                    }
                }
            }
            else if (action_type == PLAY & (song_elapsed_time > action_start_time)){
                // DECODE
                uint32_t mask = schedule[command_idx].solenoid_or_position;

                sprintf(LCD_BUFFER, "%0d: R_P %0d", command_idx, mask);
                LCD_Log(LCD_BUFFER, 1);

                // for (int i = 0; i < FINGERS_IN_EXISTENCE; i ++){
                //     if (mask & (1 << i)){
                //         set_note_state(i, HIGH);
                //     }
                //     else{
                //         set_note_state(i, LOW);
                //     }
                // }

                mcp_main.digitalWrite(0, HIGH);

                //3 CHECK TIME
                if (song_elapsed_time >= action_end_time){
                    for (int i = 0; i < FINGERS_IN_EXISTENCE; i ++){
                        set_note_state(i, LOW);
                    }

                    command_idx++;
                }
            }
        
            break;
        case(DONE):
            set_left_PWM(0);
            set_right_PWM(0);

            measured_rad = pulseCount * RAD_PER_PULSE;

            sprintf(LCD_BUFFER, "wst: %0.1f", worst_PID_settle_time);
            LCD_Log(LCD_BUFFER, 1);

            sprintf(LCD_BUFFER, "total: %0.1f", song_play_time);
            LCD_Log(LCD_BUFFER, 2);


            for (int i = 0; i < FINGERS_IN_EXISTENCE; i ++){
                set_note_state(i, LOW);
            }

            break;
        case(ERROR):
            set_left_PWM(0);
            set_right_PWM(0);
            state = ERROR;
            Log ("FSM", "ERROR!", LOG_NONE);

            sprintf(LCD_BUFFER, "ERROR!");
            LCD_Log(LCD_BUFFER, 1);

            sprintf(LCD_BUFFER, "p1=%0d, p2=%0d", digitalRead(PROX_SENSE1), digitalRead(PROX_SENSE2));
            LCD_Log(LCD_BUFFER, 2);
            break;
        default:
            sprintf(LCD_BUFFER, "Unhandeled!");
            LCD_Log(LCD_BUFFER, 1);
    }
}

// ------------------------ F U N C T I O N S    B E G I N ------------------------

void set_PWM(float output) {
    sprintf(MSG_BUFFER, "output= %0.2f", output);
    Log("PWM", MSG_BUFFER, LOG_DEBUG);

    int pwm_dc = (int) (output * 100);

    if (pwm_dc > 0) set_right_PWM(pwm_dc);
    else            set_left_PWM(abs(pwm_dc));
}

void set_left_PWM(int pwm_dc) {
    sprintf(MSG_BUFFER, "Left PWM = %d", pwm_dc);
    Log("LEFT VAL", MSG_BUFFER, LOG_DEBUG);
    PWM1_Instance->setPWM(PWM1_pin, PWM_FREQ, 100 - pwm_dc);
    PWM2_Instance->setPWM(PWM2_pin, PWM_FREQ, 100);

}

void set_right_PWM(int pwm_dc) {
    sprintf(MSG_BUFFER, "Right PWM = %d", pwm_dc);
    Log("RIGHT VAL", MSG_BUFFER, LOG_DEBUG);
    PWM1_Instance->setPWM(PWM1_pin, PWM_FREQ, 100);
    PWM2_Instance->setPWM(PWM2_pin, PWM_FREQ, 100 - pwm_dc);

}

double set_PID_(double measured_rad, double prev_measured_rad) {
    double stiction_coeff;

    // our LUT is ordered therefore search can be O(logN) instead of an if else chain which is O(N)
    if (measured_rad == prev_measured_rad) {
        // early exit on large value
        if (measured_rad > 250*KTJ/1000.0) {
            return lut[LUT_SIZE-1].stiction_pwm;
        }

        for (int i = 0; i < LUT_SIZE; i++) {
            // assumes ordering is small to large
            if (measured_rad < lut[i].measured_rad) {
                return lut[i].stiction_pwm;
            }

        }
        return 0.0;
    }
    else {
        return 0.0;
    }
}


void set_note_state (int ith_finger, bool state){ 
    // return;

    // Decode solenoid to i2c command

    // ----------- LEFT HAND FINGERS -----------

    /*
        We separate left and right by ith finger for each side with an 
        offset on right hand parsing. To make sure that each side
        doesn't get reset when parsing the other, we use an if statement
        to separate this logic so the default doesn't get hit. 
    */

    if (ith_finger <= 11) {
        switch (ith_finger)
        {
            case 0: 
                mcp_main.digitalWrite(SOLENOID_L_1, state);
                break;
            case 1:
                mcp_main.digitalWrite(SOLENOID_L_2, state);
                break;
            case 2:
                mcp_main.digitalWrite(SOLENOID_L_3, state);
                break;
            case 3:
                mcp_main.digitalWrite(SOLENOID_L_4, state);
                break;
            case 4:
                mcp_main.digitalWrite(SOLENOID_L_5, state);
                break;
            case 5:
                mcp_main.digitalWrite(SOLENOID_L_6, state);
                break;
            case 6:
                mcp_main.digitalWrite(SOLENOID_L_7, state);
                break;
            case 7:
                mcp_main.digitalWrite(SOLENOID_L_8, state);
                break;
            case 8:
                mcp_main.digitalWrite(SOLENOID_L_9, state);
                break;
            case 9:
                mcp_main.digitalWrite(SOLENOID_L_10, state);
                break;
            case 10:
                mcp_main.digitalWrite(SOLENOID_L_11, state);
                break;
            case 11:
                mcp_main.digitalWrite(SOLENOID_L_12, state);
                break;
            default:
                mcp_main.digitalWrite(SOLENOID_L_1,  HIGH);
                mcp_main.digitalWrite(SOLENOID_L_2,  HIGH);
                mcp_main.digitalWrite(SOLENOID_L_3,  LOW);
                mcp_main.digitalWrite(SOLENOID_L_4,  LOW);
                mcp_main.digitalWrite(SOLENOID_L_5,  LOW);
                mcp_main.digitalWrite(SOLENOID_L_6,  LOW);
                mcp_main.digitalWrite(SOLENOID_L_7,  LOW);
                mcp_main.digitalWrite(SOLENOID_L_8,  LOW);
                mcp_main.digitalWrite(SOLENOID_L_9,  LOW);
                mcp_main.digitalWrite(SOLENOID_L_10, LOW);
                mcp_main.digitalWrite(SOLENOID_L_11, LOW);
                mcp_main.digitalWrite(SOLENOID_L_12, LOW);
        }
    }
    else {
    // ----------- RIGHT HAND FINGERS -----------
        switch (ith_finger - 12)
        {
            case 0: 
                mcp_move.digitalWrite(SOLENOID_R_0, state);
                break;
            case 1:
                mcp_move.digitalWrite(SOLENOID_R_1, state);
                break;
            case 2:
                mcp_move.digitalWrite(SOLENOID_R_2, state);
                break;
            case 3:
                mcp_move.digitalWrite(SOLENOID_R_3, state);
                break;
            case 4:
                mcp_move.digitalWrite(SOLENOID_R_4, state);
                break;
            case 5:
                mcp_move.digitalWrite(SOLENOID_R_5, state);
                break;
            case 6:
                mcp_move.digitalWrite(SOLENOID_R_6, state);
                break;
            case 7:
                mcp_move.digitalWrite(SOLENOID_R_7, state);
                break;
            case 8:
                mcp_move.digitalWrite(SOLENOID_R_8, state);
                break;
            default:
                mcp_move.digitalWrite(SOLENOID_R_0, LOW);
                mcp_move.digitalWrite(SOLENOID_R_1, LOW);
                mcp_move.digitalWrite(SOLENOID_R_2, LOW);
                mcp_move.digitalWrite(SOLENOID_R_3, LOW);
                mcp_move.digitalWrite(SOLENOID_R_4, LOW);
                mcp_move.digitalWrite(SOLENOID_R_5, LOW);
                mcp_move.digitalWrite(SOLENOID_R_6, LOW);
                mcp_move.digitalWrite(SOLENOID_R_7, LOW);
                mcp_move.digitalWrite(SOLENOID_R_8, LOW);
        }
    }
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

void LCD_Log(char *MSG, int line_num) {
    if (line_num == 1) {
        lcd.setDataAddr(LCD_Line1Start);
    }
    else if (line_num == 2) {
        lcd.setDataAddr(LCD_Line2Start);
    }
    lcd.writeData(MSG);
}

double real_abs (double x){
    if (x > 0) {
        return x;
    } else {
        return -x;
    }
}
