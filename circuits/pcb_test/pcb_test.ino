#include <Adafruit_MCP23X17.h>
#include <Adafruit_NeoPixel.h>
#include <stdio.h>
#include <LCD1602.h>
#include <string>
#include "I2CScanner.h"
#include "RP2040_PWM.h"  // PWM


#define OUT_PIN 0     // MCP23XXX pin 
#define LED 13
#define NEOPIX 16
#define BUTTON_1_PIN 18 // Pin S1 (SCK)
#define BUTTON_2_PIN 19 // Pin S2 (MOSI)
#define BUTTON_3_PIN 20 // Pin S3 (MISO)
#define PROX_PIN 28
#define MOTOR_A 7  // PWM pin
#define MOTOR_B 8  // PWM pin
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

#define PWM_FREQ 20000 // 20KHz
#define MOTOR_DELAY 1000 // 1s

Adafruit_MCP23X17 mcp_main;
Adafruit_MCP23X17 mcp_move;
I2CScanner scanner;
LCD lcd(LCD_RS, LCD_Not_Use_Port_RW, LCD_E, LCD_D7, LCD_D6, LCD_D5, LCD_D4);
RP2040_PWM* PWM1_Instance;
RP2040_PWM* PWM2_Instance;
Adafruit_NeoPixel pixels(1, NEOPIX, NEO_GRB + NEO_KHZ800);

int button_1_val = -1; 
int button_2_val = -1; 
int button_3_val = -1;
int prev_button_1 = -1;
int prev_button_2 = -1;
int prox_val;
int prev_prox_val = -1;
int sense_val1 = -1;
int sense_val2 = -1;
int prev_sval1 = -1;
int prev_sval2 = -1;

char output[16];

<<<<<<< Updated upstream
int mode = SOL_TEST_MODE; // set mode here
int move_found = 1;

// Non-blocking timer variables
unsigned long motor_last_change = 0;
bool motor_going_left = true;
unsigned long current_time = 0;

char* get_mode_name(int m) {
  switch(m) {
    case 0: return "Mode: I2C Scan";
    case 1: return "Mode: Sol Test";
    case 2: return "Mode: Prox Test";
    case 3: return "Mode: Motor Test";
    case 4: return "Mode: Curr Sense";
    default: return "Mode: Unknown";
  }
}
=======
int mode = MOTOR_TEST_MODE; // set mode here
#define MOVE_BOARD 1 // 1 if connected
>>>>>>> Stashed changes

void setup() {
  delay(5000);
  Serial.begin(9600);

  lcd.init();
  lcd.displaySwitch(true, true, false);
  lcd.inputSet(true, false);
  lcd.clear();

  pinMode(BUTTON_3_PIN, INPUT);

  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);

  pixels.begin();
  pixels.setPixelColor(0, pixels.Color(0, 0, 0));
  pixels.show();   

  Serial.print("\r\nELEC 391 G1 Demo\r\n");

  sprintf(output, "ELEC 391 G1 Demo");
  lcd.writeData(output);

  if (mode == I2C_SCAN_MODE) {
    Serial.print("\r\nScanning!\r\n");
	  scanner.Init();
  
    scanner.Scan();
  }

  else {
    if (!mcp_main.begin_I2C(32)) {
      Serial.print("Error on main board\r\n");
      while (1);
    }
    Serial.print("Found main board\r\n");

    if (!mcp_move.begin_I2C(33)) {
      Serial.print("Error on moving board\r\n");
      move_found = 0;
    }

    // configure pins
    mcp_main.pinMode(OUT_PIN, OUTPUT);
    if (move_found) mcp_move.pinMode(OUT_PIN, OUTPUT);
    pinMode(BUTTON_1_PIN, INPUT);
    pinMode(BUTTON_2_PIN, INPUT);
    pinMode(PROX_PIN, INPUT);
    pinMode(MOTOR_A, OUTPUT);
    pinMode(MOTOR_B, OUTPUT);

    Serial.print("Configured pins\r\n");

    PWM1_Instance = new RP2040_PWM(MOTOR_A, PWM_FREQ, 0);
    Serial.print("Configured PWM1\r\n");

    PWM2_Instance = new RP2040_PWM(MOTOR_B, PWM_FREQ, 0);
    Serial.print("Configured PWM2\r\n");
  }

  delay(2000);
  lcd.clear();
  lcd.writeData(get_mode_name(mode));
  Serial.print("Starting loop\r\n");
}

void loop() {
  button_3_val = digitalRead(BUTTON_3_PIN);
  if (button_3_val == LOW) {
    mode = (mode + 1) % 5; // cycle through modes
    lcd.clear();
    lcd.writeData(get_mode_name(mode));

    prev_button_1 = -1; // reset previous values to force update on screen
    prev_button_2 = -1;
    prev_prox_val = -1;
    prev_sval1 = -1;
    prev_sval2 = -1;

    set_left_PWM(0); // stop motor when changing modes
    set_right_PWM(0);
    
    delay(500); // debounce delay
  }

  if (mode == SOL_TEST_MODE) {
    button_1_val = digitalRead(BUTTON_1_PIN);
    mcp_main.digitalWrite(OUT_PIN, !button_1_val);
    if (move_found) {
      button_2_val = digitalRead(BUTTON_2_PIN);
      mcp_move.digitalWrite(OUT_PIN, !button_2_val);
    }

    if (button_1_val != prev_button_1 || button_2_val != prev_button_2) {
      if (move_found) sprintf(output, "Sol 1:%d Sol 13:%d", !button_1_val, !button_2_val);
      else sprintf(output, "Sol 1:%d         ", !button_1_val);
      
      lcd.setDataAddr(LCD_Line2Start);
      lcd.writeData(output);

      prev_button_1 = button_1_val;
      prev_button_2 = button_2_val;
    }
  }
  if (mode == PROX_TEST_MODE) {
    prox_val = digitalRead(PROX_PIN);

    if (prox_val != prev_prox_val) {
      if (prox_val) sprintf(output, "No prox");
      else sprintf(output, "Prox!!!");

      lcd.setDataAddr(LCD_Line2Start);
      lcd.writeData(output);

      prev_prox_val = prox_val;
    }
  }

  if (mode == MOTOR_TEST_MODE) {
    current_time = millis();
    
    // Change motor direction every MOTOR_DELAY ms
    if (current_time - motor_last_change >= MOTOR_DELAY) {
      motor_going_left = !motor_going_left;
      motor_last_change = current_time;
    }
    
    if (motor_going_left) {
      set_left_PWM(20);
      sprintf(output, "moving left ");
    } else {
      set_right_PWM(20);
      sprintf(output, "moving right");
    }
    
    lcd.setDataAddr(LCD_Line2Start);
    lcd.writeData(output);
  }

  if (mode == CURRENT_SENSE_MODE) {
    // Run motor continuously while sensing
    set_left_PWM(20);
    
    sense_val1 = analogRead(CURRENT_S1);
    sense_val2 = analogRead(CURRENT_S2);

    if (sense_val1 != prev_sval1 || sense_val2 != prev_sval2) {
      // Convert ADC values to voltage (12-bit ADC, 3.3V reference)
      float volt1 = (sense_val1 / 4095.0) * 3.3;
      float volt2 = (sense_val2 / 4095.0) * 3.3;
      
      sprintf(output, "S1:%.2fV S2:%.2fV", volt1, volt2);
      lcd.setDataAddr(LCD_Line2Start);
      lcd.writeData(output);

      prev_sval1 = sense_val1;
      prev_sval2 = sense_val2;
    }
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
