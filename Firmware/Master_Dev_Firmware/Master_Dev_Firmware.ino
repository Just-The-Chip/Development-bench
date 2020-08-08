#include <avr/io.h>
#include <avr/interrupt.h>

byte slaveEventPins = 2;                                  // Represents Pins 2-6, 3-6 called by loops
volatile bool slaveInterrupt = false;                     // Is set to true by ISR
volatile unsigned long lastInterruptTime = micros();      // Is set to the current time when the ISR executes.  Is used to, "debounce" the ISR. 
byte currentSlaveEvent = 0;                               // Stores events from the slave.  A value of 0 means there was no event.

// Pins 2-6 used to read events from slave

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

void setup()  {
  Serial.begin(256000);
  // Configure pins 2-6 as inputs with interrupts and debounce timers
  for (byte i = 0; i < 5; i++) {
    pinMode(i + slaveEventPins, INPUT);
    attachInterrupt(digitalPinToInterrupt(i + slaveEventPins), interruptRoutine, RISING);
  }
}

void loop() {

  currentSlaveEvent = checkUserEvents();

  if (currentSlaveEvent) {
    Serial.println(slaveEvents[currentSlaveEvent]);
  }

}

// This is the Interrupt Serive Routine (ISR) for user events that are transmitted from the slave device
void interruptRoutine () {

  if (micros() >= lastInterruptTime + 2000) {
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
  if (slaveInterrupt && (micros() > (lastInterruptTime + 50))) {
    event = getSlaveEvent();
    slaveInterrupt = false;

  }
  sei();  // enable interrupts globally

  return event;
}