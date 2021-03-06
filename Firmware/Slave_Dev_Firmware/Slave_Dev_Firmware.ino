
// Library for attached hardware
#include "slaveHardware.h"

// Definitions for Keypad
const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
char hexaKeys[ROWS][COLS] = {
  {1,2,3,4},    //Cannot use ASCII 0 because it is NULL character
  {5,6,7,8},
  {9,10,11,12},
  {13,14,15,16}
};
byte rowPins[ROWS] = {2, 3, 4, 5}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {9, 8, 7, 6}; //connect to the column pinouts of the keypad
//initialize an instance of class NewKeypad
Keypad myKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);
unsigned long lastKeyEventTime = millis();  //  Is set to current time when keyPadEvent is detected.  Used for debouncing.

// Definitions for Encoder
byte pinA = 10;
byte pinB = 11;
byte btnPin = 12;
Encoder myEncoder = Encoder(pinA, pinB, btnPin);

uint8_t eventPins[] = {A0, A1, A2, A3, A4};  // Pins used to signal events to master

//Definitions for LED
byte LEDpin = 13;
unsigned long lastEventTime = millis();

void setup() {
	Serial.begin(115200);

  for(byte i = 0; i < 5; i++) {
    pinMode(eventPins[i], OUTPUT);
    digitalWrite(eventPins[i], LOW);
  }

  pinMode(LEDpin, OUTPUT);
  digitalWrite(LEDpin, LOW);
}


void loop() {
  byte keyPadEvent = myKeypad.getKey();
  userEvent encoderEvent = myEncoder.checkEncoder();
  unsigned long now = millis();

  if (keyPadEvent && ((now - lastKeyEventTime) >= 100)) {
    setLED();
    sendEvent(keyPadEvent);
    lastKeyEventTime = now;
  }
  if (encoderEvent) {
    setLED();
    sendEvent(encoderEvent);
  }
  if ((now - lastEventTime) >= 25) {
    digitalWrite(LEDpin, LOW);
  }
}

void sendEvent(userEvent event) {

  for(byte i = 0; i < 5; i++) {
    digitalWrite(eventPins[i], bitRead(event, i));
  }
  delay(1);
  for(byte i = 0; i < 5; i++) {

    digitalWrite(eventPins[i], LOW);
  }

}

// Turns on the LED and records the current time
void setLED (void) {
  digitalWrite(LEDpin, HIGH);
  lastEventTime = millis();
}
