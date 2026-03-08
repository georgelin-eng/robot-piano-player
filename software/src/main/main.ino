#include <stdio.h>             // Standard IO
#include "RP2040_PWM.h"        // PWM
#include "pins.h"              // pin to variable mappings
#include "commands.h"          // command table
#include "peripherals.h"       // ISRs for interacting with peripherals
#include "PID.h"               // PID functions
#include "logging.h"           // logging functions
#include <Adafruit_MCP23X17.h> // I2C expander library

#include <LCD1602.h>           // LCD library
#include "I2CScanner.h"        // I2C device scanning

#include <string>

// ----------- DEFINITIONS --------------

#define RAD_PER_PULSE 0.0981747704247  // 2pi / 64.
#define KJT 0.00097000000000000005055  // joint-to-task space: 1:10 gearbox + 17.4mm diameter pulley 
#define KTJ 1030.9278350515462535      // task-to-joint space

#define POS_ERR_THRS   (3 * 0.001)     // 3mm precision
#define ANGLE_ERR_THRS (POS_ERR_THRS * KTJ)

#define PWM_FREQ 20000 // 20KHz
#define PID_CONTROL_FREQ 80.0
#define PID_CONTROL_INTERVAL (1 / PID_CONTROL_FREQ)

// PID parameters
#define PID_KP 0.0746
#define PID_KI 0.0033
#define PID_KD 0.0044
#define PID_KAW 0.01 // Anti-integral windup gain (WIP)
#define PID_BETA 0.2759
#define PID_LIM_MIN -1.0
#define PID_LIM_MAX 1.0

#define FINGERS_IN_EXISTENCE 9

#define GLOBAL_LOG_LEVEL LOG_LOW

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


// -------------- ENUMERATIONS --------

enum eFSM_STATE {
    IDLE,
    HOME,
    RUN_INIT,
    RUN,
    PAUSE, 
    ERROR,
    POWERDOWN
};

eFSM_STATE state;

// ------------------------ S E T U P    C O D E    B E G I N ------------------------

void setup() {
    pinMode(A0, INPUT); // current_sense_i
    pinMode(24, INPUT); // ENCA_i
    pinMode(25, INPUT); // ENCB_ic:\Users\flipa\OneDrive - UBC\University\Course Materials\3rd year\ELEC 391\robot-piano-player\circuits\pcb_test\pcb_test.ino
    pinMode(A3, OUTPUT); // PWM_o
    pinMode(4, INPUT); // prox_sens_i
    pinMode(5, OUTPUT); // fault_detected_n
    pinMode(6, OUTPUT); // PWM_dir_o
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
    PID.control_interval = PID_CONTROL_INTERVAL;

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
      Serial.print("Error on main board\r\n");
      while (1);
    }
        Serial.print("Found moving board\r\n");
}


// ------------------------ M A I N    C O D E    B E G I N ------------------------

// TODO: Get timers working and start logging encoder reads
void loop() {

    static double pid_output;
    static double measured_rad;
    static double wanted_rad;
    static unsigned long prev_pid_time = 0;

    static double song_start_time = 0; // time when song starts, used to track elapsed time for command scheduling
    static double song_elapsed_time = 0; // time elapsed since start of song
    static double action_start_time = 0; // start time of each action command
    static double action_end_time   = 0; // target end time of action

    static int action_type;  // TODO: Make this an enum
    static int command_idx = 0;
    static uint16_t current_solenoid;
    static uint16_t wanted_position;


    // TODO: Might need gain scheduling for left vs right side response
    // Linear interpolation of PID not needed. Can use basic piecewaise gain schedule

    // --------- FSM BEGIN ---------
    switch(state) {
        case(IDLE):
            PIDController_Init(&PID);

            sprintf(LCD_BUFFER, "IDLE");
            LCD_Log(LCD_BUFFER, 1);
            delay(500);

            state = HOME;

            break;
        case(HOME):
            set_left_PWM(12); // TODO: Should ramp up to slow speed

            // Use PID controller to get speed measurements (derivative of position)
            measured_rad = pulseCount * RAD_PER_PULSE;
            PIDController_Measure(&PID, measured_rad);
            
            sprintf(LCD_BUFFER, "HOMING");
            LCD_Log(LCD_BUFFER, 1);
            sprintf(LCD_BUFFER, "p1=%0d, w=%0.2f", digitalRead(PROX_SENSE1), PID.d_measured);
            LCD_Log(LCD_BUFFER, 2);
            
            if (digitalRead(PROX_SENSE1) == 0) {
                state = RUN_INIT;
            }

            break;

        case(RUN_INIT):
            PIDController_Init(&PID);

            song_start_time = millis()/1000.0; // convert to seconds
            command_idx = 0;
            state = RUN;        
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
            
            song_elapsed_time = millis()/1000.0 - song_start_time;

            action_type       = schedule[command_idx].action;
            action_start_time = schedule[command_idx].start_time;
            action_end_time   = schedule[command_idx].end_time;

            // PID loop
            if (action_type == RIGHT_MOVE){
                sprintf(LCD_BUFFER, "%0d: R_MOVE", command_idx);
                LCD_Log(LCD_BUFFER, 1);

                if(micros() - prev_pid_time  >= PID_CONTROL_INTERVAL*1e6){
                    prev_pid_time = micros();

                    wanted_position = schedule[command_idx].solenoid_or_position;
                    wanted_rad = wanted_position * KTJ;

                    measured_rad = pulseCount * RAD_PER_PULSE;

                    pid_output = PIDController_Update(&PID, wanted_rad, measured_rad);
                    set_PWM(pid_output);

                    sprintf(LCD_BUFFER, "%yd=0.2f: ya=%0.2f", wanted_rad, measured_rad);
                    LCD_Log(LCD_BUFFER, 2);

                    if (PID.error < ANGLE_ERR_THRS && song_elapsed_time > action_end_time) {
                        command_idx++;
                    }
                }
            }
            else if (action_type == RIGHT_PLAY){
                sprintf(LCD_BUFFER, "%0d: R_PLAY", command_idx);
                LCD_Log(LCD_BUFFER, 1);

                // DECODE
                uint16_t mask = schedule[command_idx].solenoid_or_position;

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
    int pwm_dc = (int) (output * 100);

    if (pwm_dc > 0) set_right_PWM(pwm_dc);
    else            set_left_PWM(abs(pwm_dc));
}

void set_left_PWM(int pwm_dc) {
    sprintf(MSG_BUFFER, "Left PWM = %d", pwm_dc);
    Log("LEFT VAL", MSG_BUFFER, LOG_HIGH);
    PWM1_Instance->setPWM(PWM1_pin, PWM_FREQ, 100);
    PWM2_Instance->setPWM(PWM2_pin, PWM_FREQ, 100 - pwm_dc);
}

void set_right_PWM(int pwm_dc) {
    sprintf(MSG_BUFFER, "Right PWM = %d", pwm_dc);
    Log("RIGHT VAL", MSG_BUFFER, LOG_HIGH);
    PWM1_Instance->setPWM(PWM1_pin, PWM_FREQ, 100 - pwm_dc);
    PWM2_Instance->setPWM(PWM2_pin, PWM_FREQ, 100);
}

/*
    We use a single switch statement to turn solenoids on and off
    Since the mappings of finger number to address aren't linear
    we use a case statement as a dictionary with the mappings defined in pins.h
*/
void set_note_state (int ith_finger, bool state){ 
    // Decode solenoid to i2c command
    switch (ith_finger)
    {
        case 0: 
            mcp_move.digitalWrite(SOLENOID_R_0, state);
            break;
        case 1:
            mcp_main.digitalWrite(SOLENOID_R_1, state);
            break;
        case 2:
            mcp_main.digitalWrite(SOLENOID_R_2, state);
            break;
        case 3:
            mcp_main.digitalWrite(SOLENOID_R_3, state);
            break;
        case 4:
            mcp_main.digitalWrite(SOLENOID_R_4, state);
            break;
        case 5:
            mcp_main.digitalWrite(SOLENOID_R_5, state);
            break;
        case 6:
            mcp_main.digitalWrite(SOLENOID_R_6, state);
            break;
        case 7:
            mcp_main.digitalWrite(SOLENOID_R_7, state);
            break;
        case 8:
            mcp_main.digitalWrite(SOLENOID_R_8, state);
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
