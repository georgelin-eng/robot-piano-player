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


#define SOL_TEST_MODE 0 // tests solenoid function - S1 controls sol 1, S2 controls sol 13 (on moving board)
#define PROX_TEST_MODE 1 // tests proximity sensor
#define MOTOR_TEST_MODE 2 // tests motor - left and right every MOTOR_DELAY ms
#define CURRENT_SENSE_MODE 3 // test motor and print value of the current sense on the screen
#define I2C_SCAN_MODE 4 // scans for I2C devices - should get one at 0x20 and one at 0x21 with both boards

#define MOVE_BOARD 1 // 1 if connected
#define PWM_FREQ 20000 // 20KHz
#define MOTOR_DELAY 2000 // 1s
#define RUNNING_AVG_SIZE 10 // Number of samples for running average

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

// Running average variables
int adc_buffer1[RUNNING_AVG_SIZE] = {0};
int adc_buffer2[RUNNING_AVG_SIZE] = {0};
int buffer_index = 0;
float sense_volt1 = 0.0;
float sense_volt2 = 0.0;
float sense_current1 = 0.0;
float sense_current2 = 0.0;

char output[16];

int mode = SOL_TEST_MODE; // set mode here
int move_found = 1;

// Non-blocking timer variables
unsigned long motor_last_change = 0;
unsigned long current_time = 0;
int motor_state = 0; // 0=left, 1 = left-fast, 2=stop, 3=right, 4=right-fast, 5=stop

char* get_mode_name(int m) {
  switch(m) {
    case 0: return "Mode: Sol Test";
    case 1: return "Mode: Prox Test";
    case 2: return "Mode: Motor Test";
    case 3: return "Mode: Curr Sense";
    case 4: return "Mode: I2C Scan";
    default: return "Mode: Unknown";
  }
}

void setup() {
  //delay(5000);
  Serial.begin(9600);

  lcd.init();
  lcd.displaySwitch(true, true, false);
  lcd.inputSet(true, false);
  lcd.clear();

  pinMode(BUTTON_3_PIN, INPUT);

  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);

  pixels.begin();
  pixels.setPixelColor(0, pixels.Color(0, 150, 0));
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
    if (MOVE_BOARD) {
      if (!mcp_move.begin_I2C(33)) {
        Serial.print("Error on moving board\r\n");
        move_found = 0;
      }
      Serial.print("Found moving board\r\n");
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

  delay(3000);
  lcd.clear();
  lcd.writeData(get_mode_name(mode));
  Serial.print("Starting loop\r\n");
}

void loop() {
  button_3_val = digitalRead(BUTTON_3_PIN);
  if (button_3_val == LOW) {
    mode = (mode + 1) % 3; // cycle through modes
    lcd.clear();
    lcd.writeData(get_mode_name(mode));

    prev_button_1 = -1; // reset previous values to force update on screen
    prev_button_2 = -1;
    prev_prox_val = -1;
    prev_sval1 = -1;
    prev_sval2 = -1;
    
    set_left_PWM(0); // stop motor when changing modes
    set_right_PWM(0);
    
    // Reset timers
    motor_last_change = millis();
    motor_state = 0;
    
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
      if (prox_val) sprintf(output, "No metal        ");
      else sprintf(output, "Metal detected  ");

      lcd.setDataAddr(LCD_Line2Start);
      lcd.writeData(output);

      prev_prox_val = prox_val;
    }
  }

  if (mode == MOTOR_TEST_MODE) {
    current_time = millis();
    
    // Change motor state every MOTOR_DELAY ms
    if (current_time - motor_last_change >= MOTOR_DELAY) {
      motor_state = (motor_state + 1) % 6;
      motor_last_change = current_time;
    }
    
    // Set motor PWM 
    if (motor_state == 0) {
      set_left_PWM(20);
      sprintf(output, "move left       ");
    } else if (motor_state == 1) {
      set_left_PWM(40);
      sprintf(output, "move left fast  ");
    } else if (motor_state == 2 || motor_state == 5) {
      set_left_PWM(0);
      set_right_PWM(0);
      sprintf(output, "stopped         ");
    } else if (motor_state == 3) {
      set_right_PWM(20);
      sprintf(output, "move right       ");
    } else if (motor_state == 4) {
      set_right_PWM(40);
      sprintf(output, "move right fast ");
    }
    
    lcd.setDataAddr(LCD_Line2Start);
    lcd.writeData(output);
  }

  if (mode == CURRENT_SENSE_MODE) {
    current_time = millis();
    
    // Change motor state every MOTOR_DELAY ms
    if (current_time - motor_last_change >= MOTOR_DELAY) {
      motor_state = (motor_state + 1) % 4;
      motor_last_change = current_time;
    }
    
    // Set motor PWM 
    if (motor_state == 0) {
      set_left_PWM(20);
    } else if (motor_state == 1 || motor_state == 3) {
      set_left_PWM(0);
      set_right_PWM(0);
    } else {
      set_right_PWM(20);
    }
    
    sense_val1 = analogRead(CURRENT_S1);
    sense_val2 = analogRead(CURRENT_S2);

    // // Update running average 
    // adc_buffer1[buffer_index] = sense_val1;
    // adc_buffer2[buffer_index] = sense_val2;
    // buffer_index = (buffer_index + 1) % RUNNING_AVG_SIZE;
    
    // // Calculate running averages
    // int sum1 = 0, sum2 = 0;
    // for (int i = 0; i < RUNNING_AVG_SIZE; i++) {
    //   sum1 += adc_buffer1[i];
    //   sum2 += adc_buffer2[i];
    // }
    // sense_volt1 = (sum1 / (float)RUNNING_AVG_SIZE / 4095.0) * 3.3;
    // sense_volt2 = (sum2 / (float)RUNNING_AVG_SIZE / 4095.0) * 3.3;

    sense_volt1 = sense_val1 / 4095.0 * 3.3;
    sense_volt2 = sense_val2 / 4095.0 * 3.3;

    if (sense_val1 != prev_sval1 || sense_val2 != prev_sval2) {
      // Convert voltage to current using sense resistor and amplifying circuit
      // Current = V / (Rsense × gain) = V / (0.01 × (3900/51)) = V × 51 / 39
      sense_current1 = sense_volt1 * 51.0 / 39.0;
      sense_current2 = sense_volt2 * 51.0 / 39.0;
      

      sprintf(output, "V1:%.2f V2:%.2f ", sense_volt1, sense_volt2);
      lcd.setDataAddr(LCD_Line1Start);
      lcd.writeData(output);
      sprintf(output, "I1:%.2f I2:%.2f ", sense_current1, sense_current2);
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
