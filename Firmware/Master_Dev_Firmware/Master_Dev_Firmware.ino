// Inluded for interrupt handling
#include <avr/io.h>
#include <avr/interrupt.h>

// Included for LCD
#include <Wire.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"

// Variables for slave (UI) events
byte slaveEventPins = 2;                                  // Represents Pins 2-6, 3-6 called by loops
volatile bool slaveInterrupt = false;                     // Is set to true by ISR
volatile unsigned long lastInterruptTime = micros();      // Is set to the current time when the ISR executes.  Is used to, "debounce" the ISR. 
byte currentSlaveEvent = 0;                               // Stores events from the slave.  A value of 0 means there was no event.
const char *slaveEvents[] = {
  "None",
  "S1",
  "S2",
  "S3",
  "S4",
  "S5",
  "S6",
  "S7",
  "S8",
  "S9",
  "S10",
  "S11",
  "S12",
  "S13",
  "S14",
  "S15",
  "S16",
  "enCW",
  "enCCW",
  "enBtn"
};

// Variables for LCD
#define I2C_ADDRESS 0x3C
SSD1306AsciiWire oled;

const uint8_t littleBuddyHeight = 10;
const char *littleBuddy[] = {
  ".-\"^\"-._.-\"^\"-._.-\"^\"-._.-\"^\"-._",
  "_.-\"^\"-._.-\"^\"-._.-\"^\"-._.-\"^\"-.",
  "._.-\"^\"-._.-\"^\"-._.-\"^\"-._.-\"^\"-",
  "-._.-\"^\"-._.-\"^\"-._.-\"^\"-._.-\"^\"-",
  "\"-._.-\"^\"-._.-\"^\"-._.-\"^\"-._.-\"^",
  "^\"-._.-\"^\"-._.-\"^\"-._.-\"^\"-._.-\"",
  "\"^\"-._.-\"^\"-._.-\"^\"-._.-\"^\"-._.-",
  "-\"^\"-._.-\"^\"-._.-\"^\"-._.-\"^\"-._.-\\",
  "-\"^\"-._.-\"^\"-._.-\"^\"-._.-\"^\"-._.-\\",
  "\"^\"-._.-\"^\"-._.-\"^\"-._.-\"^\"-._.-",
  "^\"-._.-\"^\"-._.-\"^\"-._.-\"^\"-._.-\"",
  "\"-._.-\"^\"-._.-\"^\"-._.-\"^\"-._.-\"^",
  "-._.-\"^\"-._.-\"^\"-._.-\"^\"-._.-\"^\"-",
  "._.-\"^\"-._.-\"^\"-._.-\"^\"-._.-\"^\"-",
  "_.-\"^\"-._.-\"^\"-._.-\"^\"-._.-\"^\"-.",
};

void setup()  {
  Serial.begin(256000);
  // Configure pins 2-6 as inputs with interrupts for slave (UI) event transmission
  for (byte i = 0; i < 5; i++) {
    pinMode(i + slaveEventPins, INPUT);
    attachInterrupt(digitalPinToInterrupt(i + slaveEventPins), interruptRoutine, RISING);
  }

  // For LCD
  Wire.begin();
  Wire.setClock(400000L);
  oled.begin(&Adafruit128x64, I2C_ADDRESS);
  oled.setFont(System5x7);
  oled.clear();
  oled.set2X();
  oled.print("Dev\nReady!");
  delay(2000);
  oled.clear();
  oled.set1X();
  oled.setScrollMode(SCROLL_MODE_APP);
  oled.setRow(oled.displayRows() - oled.fontRows());
}

unsigned long lastEventTime = millis();
void loop() {

  currentSlaveEvent = checkUserEvents();

  if (currentSlaveEvent) {
    oled.clear();
    oled.set2X();
    oled.print(slaveEvents[currentSlaveEvent]);
    oled.set1X();
    lastEventTime = millis();
  }

  if ((millis() - lastEventTime) >= 2000) {
    scrollLittleBuddy();
  }
}

// This is the Interrupt Serive Routine (ISR) for user events that are transmitted from the slave device
void interruptRoutine () {

  if ((micros() - lastInterruptTime) >= 2000) {
    slaveInterrupt = true;
  }

  lastInterruptTime = micros();
}

// Reads the slave pins and returns the event ID
byte getSlaveEvent (void) {
  byte eventNum = 0;
  
  for (byte i = 0; i < 5; i++) {
    bitWrite(eventNum, i, digitalRead(i + slaveEventPins));
  }

  return eventNum;
}

// Checks the ISR flag and gets the event ID if it is true.  Returns the event ID.
byte checkUserEvents (void) {
  byte event = 0;

  cli();  // Disable global interrupts to protect ISR variables during this operation

  // Read slave event N microseconds after interrupt to give all slave pins time to set
  if (slaveInterrupt && ((micros() - lastInterruptTime) >= 50)) {
    event = getSlaveEvent();
    slaveInterrupt = false;

  }
  sei();  // enable interrupts globally

  return event;
}

uint32_t scrollTime;
uint16_t line;
uint8_t row = 0;
const uint8_t scrollDwellTime = 15;
void scrollLittleBuddy() {
  if (oled.scrollIsSynced()) {
    // Scroll memory window one row (8 scan lines) with a newline ('\n').
    oled.print("\n");
    oled.print(littleBuddy[row]);
    row++;
  } else {
    uint32_t now = millis();
    if ((now - scrollTime) >= scrollDwellTime) {
      // Scroll display window one scan line. 
      oled.scrollDisplay(1);
      scrollTime = now;
    }
  }
  if (row > (littleBuddyHeight-1)) {
    row = 0;
  }
}