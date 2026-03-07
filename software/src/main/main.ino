// #define DRV8263H

#include <stdio.h>       // Standard IO
#include "RP2040_PWM.h"  // PWM
#include "pins.h"        // pin to variable mappings
#include "commands.h"    // command table
#include "peripherals.h" // ISRs for interacting with peripherals
#include "PID.h"         // PID functions

#include <LCD1602.h>
#include <string>
// #include "I2CScanner.h"

#define RAD_PER_PULSE 0.0981747704247*4  // 2pi / 64.
#define KJT 0.00097000000000000005055  // joint-to-task space: 1:10 gearbox + 17.4mm diameter pulley 
#define KTJ 1030.9278350515462535      // task-to-joint space

#define POS_ERR_THRS   (15 * 0.001)     // 3mm precision
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

// Logging parameters
enum eLogLevel {
    LOG_NONE   = 0,
    LOG_LOW    = 100,
    LOG_MEDIUM = 200,
    LOG_HIGH   = 300,
    LOG_FULL   = 400,
    LOG_DEBUG  = 500
};

#define GLOBAL_LOG_LEVEL LOG_LOW

char MSG_BUFFER[64]; // global message buffer for serial logging
char LCD_BUFFER[16]; // LCD buffer

// LCD instance
LCD lcd(LCD_RS, LCD_Not_Use_Port_RW, LCD_E, LCD_D7, LCD_D6, LCD_D5, LCD_D4);

//creates pwm instance
RP2040_PWM* PWM1_Instance;
RP2040_PWM* PWM2_Instance;

// PID instance
PIDController PID;

// Motor control variables
volatile long pulseCount = 0;
volatile long diff_pulseCount;
volatile long prev_pulseCount;
volatile float ANGLE;
volatile float POSITION;

// FSM 
enum eFSM_STATE {
    IDLE,
    HOME,
    RUN,
    PAUSE, 
    ERROR,
    POWERDOWN
};

  
eFSM_STATE state = IDLE;

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

    static double measured_absolute_angle_rad;
    static double wanted_absolute_angle_rad;
    static double error;
    static double output;
    static int first_entry ; 

    int pwm_dc;    

    double speed;

    

    measured_absolute_angle_rad = pulseCount * RAD_PER_PULSE;
    // TODO: Might need gain scheduling for left vs right side response
    // Linear interpolation of PID not needed. Can use basic piecewaise gain schedule

    // --------- FSM BEGIN ---------
    switch(state) {
        case(IDLE):
            PIDController_Init(&PID);
            pwm_dc = 0;

            sprintf(LCD_BUFFER, "IDLE");
            LCD_Log(LCD_BUFFER, 1);
            delay(500);

            state = HOME;

            break;
        case(HOME):
            set_left_PWM(12); // TODO: Should ramp up to slow speed

            sprintf(LCD_BUFFER, "HOMING");
            LCD_Log(LCD_BUFFER, 1);
            sprintf(LCD_BUFFER, "p1=%0d, p2=%0d", digitalRead(PROX_SENSE1), digitalRead(PROX_SENSE2));
            LCD_Log(LCD_BUFFER, 2);
            
            if (digitalRead(PROX_SENSE1) == 0) {
                state = ERROR;
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