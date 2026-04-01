#include <stdio.h>             // Standard IO
#include "RP2040_PWM.h"        // PWM
#include "../main/pins.h"              // pin to variable mappings
#include "../main/commands.h"          // command table
#include "../main/peripherals.h"       // ISRs for interacting with peripherals
#include "../main/PID.h"               // PID functions
#include "../main/logging.h"           // logging functions
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
#define PID_S_KP (0.0567 * K0 ) * 0.25// (0.0567 * K0 * 0.174) * 1.49999// * 0.138
#define PID_S_KI (0.00067091 * K0 ) * 500//(0.00067091 * K0 * 260) *0.00000021 // * 1.45
#define PID_S_KD (0.0011 * K0 )*0.01*0.18 //0.00000000087//(0.0011 * K0 * 0.00135)* 0//* 0.012

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

#define PID_STICTION_MIDDLE 0.059 * 1.03
#define PID_STICTION_SIDES 0.04

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

    static double pwm;

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
        case(RUN): {

            static unsigned long last_increment_time = 0;
            static double test_pwm = 0;
            static double start_test_rad = 0;
            static int sub_state = 0; 

            measured_rad = pulseCount * RAD_PER_PULSE;
            if (measured_rad * KJT * 1000 >= 240) {
                state = DONE;
                break;
            }

            switch (sub_state) {
                case 0: 
                    set_right_PWM(0);
                    set_left_PWM(0);
                    test_pwm = 0;
                    start_test_rad = measured_rad;
                    last_increment_time = millis();
                    sub_state = 1;
                    break;

                case 1: 
                    if (millis() - last_increment_time > 15) { 
                        test_pwm += 0.25;

                        if (test_pwm > 100) test_pwm = 100; 

                        set_left_PWM(test_pwm); 
                        last_increment_time = millis();
                    }


                    if (real_abs(measured_rad - start_test_rad) > 0.05) { // corresponds to 0.05mm, too small - gl
                        

                        set_left_PWM(0); 
                        
                        sprintf(MSG_BUFFER, "Pos: %0.2f rad, Stic PWM: %0.2lf", measured_rad, test_pwm);
                        Log("STICTION_MAP", MSG_BUFFER, LOG_NONE);

                        last_increment_time = millis();
                        sub_state = 2;
                    }
                    break;

                case 2: 
                    if (millis() - last_increment_time > 250) { 
                        sub_state = 0; // Restart the cycle at the new position
                    }
                    break;
            }
            break;

        }
        case(DONE):
            set_left_PWM(0);
            set_right_PWM(0);

            for (int i = 0; i < FINGERS_IN_EXISTENCE; i ++){
                set_note_state(i, LOW);
            }

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

    if (output > 0) set_right_PWM(output*100);
    else            set_left_PWM(real_abs(output*100));
}

void set_left_PWM(double pwm_dc) {
    PWM1_Instance->setPWM(PWM1_pin, PWM_FREQ, 100 - pwm_dc);
    PWM2_Instance->setPWM(PWM2_pin, PWM_FREQ, 100);

}

void set_right_PWM(double pwm_dc) {
    PWM1_Instance->setPWM(PWM1_pin, PWM_FREQ, 100);
    PWM2_Instance->setPWM(PWM2_pin, PWM_FREQ, 100 - pwm_dc);

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