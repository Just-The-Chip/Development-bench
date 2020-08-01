
// Libraryies for LCD
#include <ArducamSSD1306.h>    // Modification of Adafruit_SSD1306 for ESP8266 compatibility
#include <Adafruit_GFX.h>   // Needs a little change in original Adafruit library (See README.txt file)
#include <Wire.h>           // For I2C comm, but needed for not getting compile error
// Library for Keypad
#include "Keypad.h"
// Library for encoder and future hardware
#include "Hardware.h"

// Definitions for LCD
/*
HardWare I2C pins for LCD on Arduino Uno
A4   SDA
A5   SCL
*/
#define OLED_RESET  16  // Pin 15 -RESET digital signal
ArducamSSD1306 display(OLED_RESET); // FOR I2C

// Definitions for Keypad
const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
char hexaKeys[ROWS][COLS] = {
  {1,2,3,4},    //Cannot use ASCII 0 because it is NULL character
  {5,6,7,8},
  {9,10,11,12},
  {13,14,15,16}
};
char *myKeys[] = {"S1","S2","S3","S4","S5","S6","S7","S8","S9","S10","S11","S12","S13","S14","S15","S16"}; //names of buttons on my keypad
byte rowPins[ROWS] = {2, 3, 4, 5}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {9, 8, 7, 6}; //connect to the column pinouts of the keypad
//initialize an instance of class NewKeypad
Keypad myKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

// Definitions for Encoder
byte pinA = 10;
byte pinB = 11;
byte btnPin = 12;
Encoder myEncoder = Encoder(pinA, pinB, btnPin, &displayEncoderCW, &displayEncoderCCW, &displayEncoderButton);

void setup() {
	Serial.begin(115200);  //This baud rate required for the LCD
  // SSD1306 Init
  initializeDisplay();
}


void loop() {
  byte checkKeypad = myKeypad.getKey();
  myEncoder.checkEncoder();

  if (checkKeypad) {
    displayKeypad(checkKeypad);
  }
}

void initializeDisplay(void) {
  display.begin();  // Switch OLED
  // Initialize the display with some text
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(20,0);
  display.println("Bitch");
  display.print("Lasagnia");
  display.display();
  display.setTextSize(1);
}

void displayKeypad(byte key) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0,0);
  display.print("Button =");
  display.setTextSize(3);
  display.setCursor(40,30);
  display.println(myKeys[key - 1]);
  display.display();
}

void displayEncoderCW(void) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0,0);
  display.print("EncoderCW");
  display.setTextSize(3);
  display.setCursor(40,30);
  display.println(">");
  display.display();
}

void displayEncoderCCW(void) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0,0);
  display.print("EncoderCCW");
  display.setTextSize(3);
  display.setCursor(40,30);
  display.println("<");
  display.display();
}

void displayEncoderButton(void) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0,0);
  display.print("Button");
  display.setTextSize(3);
  display.setCursor(20,30);
  display.println("Press");
  display.display();
}
