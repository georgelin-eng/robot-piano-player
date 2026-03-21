#include <stdio.h>             // Standard IO
#include "RP2040_PWM.h"        // PWM
#include "pins.h"              // pin to variable mappings
#include "commands.h"          // command table
// #include "cust_commands.h"        // command table
// #include "led_commands.h" 
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

#define POS_ERR_THRS   (10 /1000.0)
#define ANGLE_ERR_THRS (POS_ERR_THRS * KTJ)

#define PWM_FREQ 20000 // 20KHz
#define PID_CONTROL_FREQ 80.0
// #define PID_CONTROL_INTERVAL (1 / PID_CONTROL_FREQ)
#define PID_CONTROL_INTERVAL 0.0125
#define TARGET_HOME_SPEED 3.0 // cm per second

// PID parameters
#define K0 0.6

#define PID_KP (0.0567 * K0) * 0.3
#define PID_KI (0.00067091 * K0)
#define PID_KD (0.0011 * K0) 
#define PID_KAW 0.01 // Anti-integral windup gain (WIP)
#define PID_BETA 0.2759
#define PID_LIM_MIN_INT -0.2
#define PID_LIM_MAX_INT 0.2
#define PID_LIM_MIN -1.0
#define PID_LIM_MAX 1.0
#define PID_STICTION 0.05 // feedforward control for stiction (WIP)

#define FINGERS_IN_EXISTENCE 20

#define GLOBAL_LOG_LEVEL LOG_HIGH

static char MSG_BUFFER[64]; // global message buffer for serial logging
static char LCD_BUFFER[16]; // LCD buffer

// ------------- OBJECT INSTANCES -------------

// PID instance
PIDController PID;

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
    attachInterrupt(digitalPinToInterrupt(ENCA_pin), A_posedge, RISING);
    attachInterrupt(digitalPinToInterrupt(ENCA_pin), A_negedge, FALLING);
    attachInterrupt(digitalPinToInterrupt(ENCB_pin), B_posedge, RISING);
    attachInterrupt(digitalPinToInterrupt(ENCB_pin), B_negedge, FALLING);

    // PWM setup w/ 0% DC
    PWM1_Instance = new RP2040_PWM(PWM1_pin, PWM_FREQ, 0);
    PWM2_Instance = new RP2040_PWM(PWM2_pin, PWM_FREQ, 0);

    // PID setup
    PID.Kp = PID_KP;
    PID.Ki = PID_KI;
    PID.Kd = PID_KD;
    PID.Kaw = PID_KAW;
    PID.beta = PID_BETA;
    PID.limMin = PID_LIM_MIN;
    PID.limMax = PID_LIM_MAX;
    PID.limMaxInt = PID_LIM_MIN_INT;
    PID.limMinInt = PID_LIM_MAX_INT;
    PID.control_interval = PID_CONTROL_INTERVAL;
    PID.stiction = PID_STICTION;

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
    static double wanted_rad;
    static unsigned long prev_pid_time = 0;

    static double song_start_time = 0; // time when song starts, used to track elapsed time for command scheduling
    static double offset = 0;
    static double song_elapsed_time; // time elapsed since start of song
    static double action_start_time = 0; // start time of each action command
    static double action_end_time   = 0; // target end time of action

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
            measured_rad = -pulseCount * RAD_PER_PULSE;
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

            // set_left_PWM(12); 
            // set_left_PWM(pwm_dc);
            set_right_PWM(pwm_dc);

            delay(10);

            // Use PID controller to get speed measurements (derivative of position)
            
            sprintf(LCD_BUFFER, "HOME, dc=%0d", pwm_dc);
            LCD_Log(LCD_BUFFER, 1);
            sprintf(LCD_BUFFER, "rad=%0.2f, v=%0.2f", measured_rad, speed_cmps);
            LCD_Log(LCD_BUFFER, 2);
            
            if (digitalRead(PROX_SENSE1) == 0) {
                state = RUN_INIT;
                pulseCount = 0; // Once we are finished homing set this position as 0
            }

            break;

        case(RUN_INIT):
            PIDController_Init(&PID);

            song_start_time = millis()/1000.0; // convert to seconds
            command_idx = 0;
            state = RUN;        

            sprintf(LCD_BUFFER, "RUN_INIT");
            LCD_Log(LCD_BUFFER, 1);

            sprintf(LCD_BUFFER, "start=%0.1lf", song_start_time);
            LCD_Log(LCD_BUFFER, 2);

            if(millis() - prev_pid_time  >= PID_CONTROL_INTERVAL*1e3){
                prev_pid_time = millis();
                wanted_rad = INITIAL_MOTOR_POSITION_MM * KTJ/1000.0;
                measured_rad = -pulseCount * RAD_PER_PULSE;

                pid_output = PIDController_Update(&PID, wanted_rad, measured_rad);
                set_PWM(pid_output);

                if (real_abs(PID.error) < ANGLE_ERR_THRS) {
                    PIDController_Init(&PID);
                    set_PWM(0);
                }

                sprintf(LCD_BUFFER, "yd=%0.1f,ya=%0.1f", wanted_rad*KJT*1000, measured_rad*KJT*1000);
                LCD_Log(LCD_BUFFER, 1);
            }

            delay(500);
            lcd.setDataAddr(LCD_Line1Start);
            lcd.clear();    
            lcd.setDataAddr(LCD_Line2Start);
            lcd.clear();    

            break;

        case(RUN):
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
            }

            // PID loop
            if (action_type == MOVE){
                // sprintf(LCD_BUFFER, "%0d: R_MOVE", command_idx);
                // LCD_Log(LCD_BUFFER, 1);

                if(millis() - prev_pid_time  >= PID_CONTROL_INTERVAL*1e3){
                    prev_pid_time = millis();

                    wanted_rad = schedule[command_idx].solenoid_or_position * KTJ/1000.0;

                    measured_rad = -pulseCount * RAD_PER_PULSE;

                    pid_output = PIDController_Update(&PID, wanted_rad, measured_rad);
                    set_PWM(pid_output);

                    sprintf(LCD_BUFFER, "yd=%0.1f,ya=%0.1f", wanted_rad*KJT*1000, measured_rad*KJT*1000);
                    LCD_Log(LCD_BUFFER, 1);

                    sprintf(LCD_BUFFER, "t0=%0.1f,t1=%0.1f,ofs=%0.1f", song_elapsed_time, action_end_time, offset);
                    LCD_Log(LCD_BUFFER, 2);

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

                    if (real_abs(PID.error) < ANGLE_ERR_THRS && song_elapsed_time >= action_end_time) {
                        
                        command_idx++;
                        PIDController_Init(&PID);
                        set_PWM(0);
                    }
                }
            }
            else if (action_type == PLAY & (song_elapsed_time > action_start_time)){
                // DECODE
                uint32_t mask = schedule[command_idx].solenoid_or_position;

                sprintf(LCD_BUFFER, "%0d: R_P %0d", command_idx, mask);
                LCD_Log(LCD_BUFFER, 1);

                sprintf(LCD_BUFFER, "t=%0.1lf, e=%0.1lf", song_elapsed_time, action_end_time);
                LCD_Log(LCD_BUFFER, 2);
                
                for (int i = 0; i < FINGERS_IN_EXISTENCE; i ++){
                    if (mask & (1 << i)){
                        set_note_state(i, HIGH);
                    }
                    else{
                        set_note_state(i, LOW);
                    }
                }
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

            sprintf(LCD_BUFFER, "DONE             ");
            LCD_Log(LCD_BUFFER, 1);

            sprintf(LCD_BUFFER, "ya=%0.1f       ", measured_rad);
            LCD_Log(LCD_BUFFER, 2);


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
    Log("PWM", MSG_BUFFER, LOG_HIGH);

    int pwm_dc = (int) (output * 100);

    if (pwm_dc > 0) set_right_PWM(pwm_dc);
    else            set_left_PWM(abs(pwm_dc));
}

void set_left_PWM(int pwm_dc) {
    sprintf(MSG_BUFFER, "Left PWM = %d", pwm_dc);
    Log("LEFT VAL", MSG_BUFFER, LOG_HIGH);
    PWM1_Instance->setPWM(PWM1_pin, PWM_FREQ, 100 - pwm_dc);
    PWM2_Instance->setPWM(PWM2_pin, PWM_FREQ, 100);

}

void set_right_PWM(int pwm_dc) {
    sprintf(MSG_BUFFER, "Right PWM = %d", pwm_dc);
    Log("RIGHT VAL", MSG_BUFFER, LOG_HIGH);
    PWM1_Instance->setPWM(PWM1_pin, PWM_FREQ, 100);
    PWM2_Instance->setPWM(PWM2_pin, PWM_FREQ, 100 - pwm_dc);

}

/*
    We use a single switch statement to turn solenoids on and off
    Since the mappings of finger number to address aren't linear
    we use a case statement as a dictionary with the mappings defined in pins.h

*/
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
                mcp_main.digitalWrite(SOLENOID_L_1,  LOW);
                mcp_main.digitalWrite(SOLENOID_L_2,  LOW);
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
