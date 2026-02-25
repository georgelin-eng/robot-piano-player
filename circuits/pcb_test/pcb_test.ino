#include <Adafruit_MCP23X17.h>
#include <stdio.h>
#include <LCD1602.h>
#include <string>
#include "I2CScanner.h"
#include "RP2040_PWM.h"  // PWM

#define OUT_PIN 0     // MCP23XXX pin 
#define BUTTON_1_PIN 14 // Pin S1 (SCK)
#define BUTTON_2_PIN 15 // Pin S2 (MOSI)
#define PROX_PIN 28
#define MOTOR_A 5  // PWM pin
#define MOTOR_B 6  // PWM pin
#define LCD_RS 0 // TX
#define LCD_E 1  // RX
#define LCD_D4 12
#define LCD_D5 11
#define LCD_D6 10
#define LCD_D7 9
#define CURRENT_S1 26   // A0 current sense 1
#define CURRENT_S2 27   //  A1 current sense 2


#define I2C_SCAN_MODE 0 // scans for I2C devices - should get one at 0x20 and one at 0x21 with both boards
#define SOL_TEST_MODE 1 // tests solenoid function - S1 controls sol 1, S2 controls sol 13 (on moving board)
#define PROX_TEST_MODE 2 // tests proximity sensor
#define MOTOR_TEST_MODE 3 // tests motor - left and right every MOTOR_DELAY ms
#define CURRENT_SENSE_MODE 4 // test motor and print value of the current sense on the screen

#define MOVE_BOARD 1 // 1 if connected
#define PWM_FREQ 20000 // 20KHz
#define MOTOR_DELAY 1000 // 1s

Adafruit_MCP23X17 mcp_main;
Adafruit_MCP23X17 mcp_move;
I2CScanner scanner;
LCD lcd(LCD_RS, LCD_Not_Use_Port_RW, LCD_E, LCD_D7, LCD_D6, LCD_D5, LCD_D4);
RP2040_PWM* PWM1_Instance;
RP2040_PWM* PWM2_Instance;

int button_1_val = -1; 
int button_2_val = -1; 
int prev_button_1 = -1;
int prev_button_2 = -1;
int prox_val;
int prev_prox_val = -1;
int sense_val1 = -1;
int sense_val2 = -1;
int prev_sval1 = -1;
int prev_sval2 = -1;

char output[16];

int mode = CURRENT_SENSE_MODE; // set mode here

void setup() {
  delay(5000);
  Serial.begin(9600);

  lcd.init();
  lcd.displaySwitch(true, true, false);
  lcd.inputSet(true, false);
  lcd.clear();

  sprintf(output, "ELEC 391 G1 Demo");
  lcd.writeData(output);

  if (mode == I2C_SCAN_MODE) {
    Serial.print("\r\nScanning!\r\n");
	  scanner.Init();
  
    scanner.Scan();
  }

  else if (mode == SOL_TEST_MODE) {
    Serial.print("\r\nMCP23017 Blink Test\r\n");

    if (!mcp_main.begin_I2C(32)) {
      Serial.print("Error on main board\r\n");
      while (1);
    }
    Serial.print("Found main board\r\n");

    // configure pins
    mcp_main.pinMode(OUT_PIN, OUTPUT);
    pinMode(BUTTON_1_PIN, INPUT);

    if (MOVE_BOARD) {
      if (!mcp_move.begin_I2C(33)) {
        Serial.print("Error on moving board\r\n");
        while (1);
      }
      Serial.print("Found moving board\r\n");

      // configure pins
      mcp_move.pinMode(OUT_PIN, OUTPUT);
      pinMode(BUTTON_2_PIN, INPUT);
    }

    Serial.print("Starting test \r\n");
  }

  else if (mode == PROX_TEST_MODE) {
    Serial.print("\r\nProximity Test\r\n");

    pinMode(PROX_PIN, INPUT);
  }

  else if (mode == MOTOR_TEST_MODE) {
    Serial.print("\r\nMotor Test\r\n");

    pinMode(MOTOR_A, OUTPUT);
    pinMode(MOTOR_B, OUTPUT);

    PWM1_Instance = new RP2040_PWM(MOTOR_A, PWM_FREQ, 0);
    PWM2_Instance = new RP2040_PWM(MOTOR_B, PWM_FREQ, 0);
  }

  else if (mode == CURRENT_SENSE_MODE) {
    Serial.print("\r\nMotor Test\r\n");

    pinMode(MOTOR_A, OUTPUT);
    pinMode(MOTOR_B, OUTPUT);

    PWM1_Instance = new RP2040_PWM(MOTOR_A, PWM_FREQ, 0);
    PWM2_Instance = new RP2040_PWM(MOTOR_B, PWM_FREQ, 0);
  }


}

void loop() {
  if (mode == SOL_TEST_MODE) {
    button_1_val = digitalRead(BUTTON_1_PIN);
    mcp_main.digitalWrite(OUT_PIN, !button_1_val);
    if (MOVE_BOARD) {
      button_2_val = digitalRead(BUTTON_2_PIN);
      mcp_move.digitalWrite(OUT_PIN, !button_2_val);
    }

    if (button_1_val != prev_button_1 || button_2_val != prev_button_2) {
      if (MOVE_BOARD) sprintf(output, "Sol 1:%d Sol 13:%d", !button_1_val, !button_2_val);
      else sprintf(output, "Sol 1:%d", !button_1_val);
      
      lcd.setDataAddr(LCD_Line2Start);
      lcd.writeData(output);

      prev_button_1 = button_1_val;
      prev_button_2 = button_2_val;
    }
  }
  if (mode == PROX_TEST_MODE) {
    prox_val = digitalRead(PROX_PIN);

    if (prox_val != prev_prox_val) {
      //sprintf(output, "Prox val: %d", !prox_val);
      if (prox_val) sprintf(output, "No prox");
      else sprintf(output, "Prox!!!");

      lcd.setDataAddr(LCD_Line2Start);
      lcd.writeData(output);

      prev_prox_val = prox_val;
    }
  }

  if (mode == MOTOR_TEST_MODE) {
    set_left_PWM(20);
    Serial.print("Moving Left\r\n");
    
    sprintf(output, "moving left");
    lcd.setDataAddr(LCD_Line2Start);
    lcd.writeData(output);

    delay(MOTOR_DELAY);

    set_right_PWM(20);
    Serial.print("Moving Right\r\n");

    sprintf(output, "moving right");
    lcd.setDataAddr(LCD_Line2Start);
    lcd.writeData(output);

    delay(MOTOR_DELAY);
  }

  if (mode == CURRENT_SENSE_MODE) {

    sense_val1 = digitalRead(CURRENT_S1);
    sense_val2 = digitalRead(CURRENT_S2);

    if (sense_val1 != prev_sval1 || sense_val2 != prev_sval2) {

      sprintf(output, "CS 1:%d CS 2:%d", sense_val1, sense_val2);
      
      lcd.setDataAddr(LCD_Line2Start);
      lcd.writeData(output);

      prev_sval1 = button_1_val;
      prev_sval2 = button_2_val;
    }

    set_left_PWM(20);
    Serial.print("Moving Left\r\n");

    if (sense_val1 != prev_sval1 || sense_val2 != prev_sval2) {

      sprintf(output, "CS 1:%d CS 2:%d", sense_val1, sense_val2);
      
      lcd.setDataAddr(LCD_Line2Start);
      lcd.writeData(output);

      prev_sval1 = button_1_val;
      prev_sval2 = button_2_val;
    }
    
    sprintf(output, "moving left");
    lcd.setDataAddr(LCD_Line2Start);
    lcd.writeData(output);

    if (sense_val1 != prev_sval1 || sense_val2 != prev_sval2) {

      sprintf(output, "CS 1:%d CS 2:%d", sense_val1, sense_val2);
      
      lcd.setDataAddr(LCD_Line2Start);
      lcd.writeData(output);

      prev_sval1 = button_1_val;
      prev_sval2 = button_2_val;
    }

    delay(MOTOR_DELAY);

    if (sense_val1 != prev_sval1 || sense_val2 != prev_sval2) {

      sprintf(output, "CS 1:%d CS 2:%d", sense_val1, sense_val2);
      
      lcd.setDataAddr(LCD_Line2Start);
      lcd.writeData(output);

      prev_sval1 = button_1_val;
      prev_sval2 = button_2_val;

    }

    set_right_PWM(20);
    Serial.print("Moving Right\r\n");

    sprintf(output, "moving right");
    lcd.setDataAddr(LCD_Line2Start);
    lcd.writeData(output);

    delay(MOTOR_DELAY);
  }
}


void set_left_PWM(int pwm_dc) {
    PWM1_Instance->setPWM(MOTOR_A, PWM_FREQ, 100);
    PWM2_Instance->setPWM(MOTOR_B, PWM_FREQ, 100 - pwm_dc);
}


void set_right_PWM(int pwm_dc) {
    PWM1_Instance->setPWM(MOTOR_A, PWM_FREQ, 100 - pwm_dc);
    PWM2_Instance->setPWM(MOTOR_B, PWM_FREQ, 100);
}
