#include <stdio.h>             // Standard IO

// ------------------------ S E T U P    C O D E    B E G I N ------------------------

#define PIN_TO_USE 6

void setup() {
    pinMode(PIN_TO_USE, OUTPUT); 
}

// ------------------------ M A I N    C O D E    B E G I N ------------------------
void loop() {
    digitalWrite(PIN_TO_USE, HIGH);
    delay(100);

    digitalWrite(PIN_TO_USE, LOW);
    delay(100);
}
