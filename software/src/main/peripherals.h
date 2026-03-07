#ifndef PERIPHERALS_H
#define PERIPHERALS_H   
/*
CCW: A leading B
     ┌───┐     ┌───┐
A ───┘   └─────┘   └───
       ┌───┐     ┌───┐
B   ───┘   └─────┘   └───

CW: B leading A
       ┌───┐     ┌───┐
A   ───┘   └─────┘   └───
     ┌───┐     ┌───┐
B ───┘   └─────┘   └───

For a quadratrue encoder on a pos/neg edge depending on the value of the opposite phase,
we can know if the motor is rotating CW or CCW

Convert pulse count to global position (rad) inside the PID which fires at 80Hz
*/
extern volatile long pulseCount;

void A_posedge() {
    if (digitalRead(ENCB_pin) == 1) { // CW
        pulseCount++;
    } // CCW
    else {
        pulseCount--;
    }
}

void A_negedge() {
    if (digitalRead(ENCB_pin) == 0) { // CW
        pulseCount++;
    } // CCW
    else {
        pulseCount--;
    }
}

void B_posedge() {
    if (digitalRead(ENCA_pin) == 0) { // CW
        pulseCount++;
    } // CCW
    else {
        pulseCount--;
    }
}

void B_negedge() {
    if (digitalRead(ENCA_pin) == 1) { // CW
        pulseCount++;
    } // CCW
    else {
        pulseCount--;
    }
}

#endif