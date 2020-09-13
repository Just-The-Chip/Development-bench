#include "Arduino.h"
#include "slaveHardware.h"

// Variables for the rotary encoder:
const int sampleDwelluS ((0.001 / samplesPerMs) * 1000000);             // Wait time between samples in microseconds to free up processor for other things.
bool samplesB[sampleBufferSize];                                        // Buffer that stores [sampleBufferSize] number of samples from encoder channel B.
bool samplesA[sampleBufferSize];                                        // Buffer that stores [sampleBufferSize] number of samples from encoder channel A.
byte bufferIndex = 0;                                                   // Keeps track of where in samplesB/A to write the next sample.  Ensures FIFO.
unsigned long lastSampleTime = micros();                                // Last time encoder channels were sampled in microseconds.  Used for sample timing.
unsigned long stateTime = micros();                                     // Contains the last time the state was changed in microseconds.
byte encoderState = 0;                                                  // States: Idle, One channel low, Both channels low, One channel back high, Both channels back high (0, 1, 2, 3, 4 respectively)
unsigned long encoderTriggerTime = micros();                            // Time that the callback was triggered in microseconds.

// Variables specifically for the encoder button:
bool buttonLastState = false;                                           // Contains the previous state of the encoder button.  Used to execute code on release of the button.
const byte buttonDebounce = 300;                                        // Debounce time for the encoder button in milliseconds.
unsigned long buttonLastPressed = millis();                             // Last time the encoder button was pressed in milliseconds.

//  These variables are TRUE when their respetive events are detected:
bool EnAtrig = false;
bool EnBtrig = false;
bool btnTrig = false;

Encoder::Encoder(byte EnApin, byte EnBpin, byte btnPin) {
    pinMode(EnApin, INPUT);
    pinMode(EnBpin, INPUT);
    pinMode(btnPin, INPUT);
    _EnApin = EnApin;
    _EnBpin = EnBpin;
    _btnPin = btnPin;

    initializeBuffer(samplesB);
    initializeBuffer(samplesA);
}

userEvent Encoder::checkEncoder(void) {
    EnAtrig = !digitalRead(_EnApin);  // Is true when Encoder channel A is active (pin is active low)
    EnBtrig = !digitalRead(_EnBpin);  // Is true when Encoder channel B is active (pin is active low)
    btnTrig = !digitalRead(_btnPin);  // Is true when Encoder button is active (pin is active low)

    userEvent result = None;

    if (EnAtrig || EnBtrig) {         // if either EnA or EnB is active, record time and handle accordingly
        encoderTriggerTime = micros();
        result = handleEncoder();
    }
    if (btnTrig || buttonLastState) {
        result = handleEncoderPress();
    }

    return result;
}

// Initializes all elements of buffer to true
void Encoder::initializeBuffer(bool buffer[]) {
    for (byte i = 0; i < sampleBufferSize; i++) {
        buffer[i] = true;
    }
}

// Is used specifically to sample the encoder button.
byte Encoder::sampleChannel(byte channel, byte sampleQuantity) {
    float sample = 0;                               // Stores IO samples from encoder button.

    for(byte i = 0; i < sampleQuantity; i++) {      // Get samples
        sample = sample + !digitalRead(channel);     // add samples together to get average value
    }
    return (byte) round(sample / sampleQuantity);
}

// Executes code when the encoder button is pressed.
userEvent Encoder::handleEncoderPress() {
    bool state = sampleChannel(_btnPin, 10);                                                 // sample 10 times
    Serial.print(state);
    Serial.println(" ");
    userEvent result = None;

    if (buttonLastState && !state && ((millis() - buttonLastPressed) >= buttonDebounce)) {   // if button was just released and debounce time has elapsed, execute
        result = enBtn;
        buttonLastPressed = millis();
        Serial.println("btnExecuted");
    }
    
    buttonLastState = state;
    return result;
}

// Adds elements to the sample buffers and advances the buffer index.  The index is used to ensure FIFO for elements added to the buffer.
void Encoder::elementsToBuffer (bool EnB, bool EnA) {
    samplesB[bufferIndex] = EnB;
    samplesA[bufferIndex] = EnA;
    bufferIndex++;
    if (bufferIndex == sampleBufferSize) {
        bufferIndex = 0;
    }
}

// Returns true if all elements in samples are true.
bool Encoder::allTrue (bool samples[]) {
    bool result = true;
    for (byte i = 0; i < sampleBufferSize; i++) {
        result = samples[i];
        if (result == false) {
            break;
        }
    }
    return result;
}

// Returns true if any elements in samples are true.
bool Encoder::anyTrue (bool samples[]) {
    bool result = false;
    for (byte i = 0; i < sampleBufferSize; i++) {
            result = samples[i];
            if (result == true) {
                break;
            }
        }
    return result;
}

// Updates state machine state and updates time of the change.
void Encoder::stateChange(byte stateNumber) {               
    encoderState = stateNumber;
    stateTime = micros();
}

// Handles the rotary input of the encoder.
userEvent Encoder::handleEncoder(void) { 

    // Encoder state variables:
    bool stateB = true;                     // State of encoder channel B (filtered). Logic is made to match that of the encoder.
    bool stateA = true;                     // State of encoder channel A (filtered). Logic is made to match that of the encoder.
    bool bFirst = false;                    // Indicates if encoder channel B went low first.

    if ((micros() - stateTime) <= 1500) {   // Adds debounce to encoder rotation detection.
        return None;
    }

    // This is the state machine for trtacking encoder state changes.
    while (true) {
        //////////////////////////////////////////////////////// SAMPLE CHANNELS ////////////////////////////////////////////////////////
        // This first block alwayse executes.
        // It takes samples and updates the encoder channels filtered values every time it executes.
        // Advances state machine when either encoder channels buffer reads all false.
        if ((micros() - lastSampleTime) >= sampleDwelluS) {   // This 'if' samples both encoder channels 'samplesPerMs' times a millisecond.
            lastSampleTime = micros();
            elementsToBuffer(digitalRead(_EnBpin), digitalRead(_EnApin));
            if (allTrue(samplesB)) {                        // Updates 'stateB' continously
                stateB = true;
            }
            if (allTrue(samplesA)) {                        // Updates 'stateA' continously
                stateA = true;
            }
            if (!anyTrue(samplesB)) {                       // Updates 'stateB' continously, tracks channels firstness, and advances state
                stateB = false;
                if (encoderState == 0 && stateA) {
                    bFirst = true;
                    stateChange(1);
                }
            }
            if (!anyTrue(samplesA)) {                       // Updates 'stateA' continously, tracks channels firstness, and advances state
                stateA = false;
                if (encoderState == 0 && stateB) {
                    bFirst = false;
                    stateChange(1);
                }
            }
        }
        //////////////////////////////////////////////////////// State 0 - Idle: ////////////////////////////////////////////////////////
        // Exits callback if [time] elapses.
        if (encoderState == 0) {
            if ((micros() - encoderTriggerTime) >= 3000){
                stateChange(0);
                return None;
            }
        }
        //////////////////////////////////////////////////////// State 1 - One channel low: ////////////////////////////////////////////////////////
        // Resets state machine if timeout elapses. Goes back to previous state if the encoder channel that went low first goes high again.
        // Advances state machine if both encoder channels filtered logic are low at the same time.
        if (encoderState == 1) {
            if ((micros() - stateTime) >= 200000) {      // Timeout Reset (seconds)
                stateChange(0);
                return None;
            }
            else if (bFirst && stateB) {                // Unexpected channel B Reset
                stateChange(0);
                return None;
            }
            else if (!bFirst && stateA) {               // Unexpected channel A Reset
                stateChange(0);
                return None;
            }
            else if (!stateB && !stateA) {              // Advance state machine 
                stateChange(2);
            }
        }
        //////////////////////////////////////////////////////// State 2 - Both channels low: ////////////////////////////////////////////////////////
        // Resets state machine if timeout elapses. Goes back to previous state if wrong encoder channel goes high first.
        // Advances state machine if the correct channel goes high.
        if (encoderState == 2) {
            if ((micros() - stateTime) >= 110000) {     // Timeout Reset (seconds)
                stateChange(0);
                return None;
            }
            else if (stateA && stateB) {                // Unexpected both channels go back high. Not sure why this happens, but skipping to state 4 improves recognition.
                stateChange(4);
            }
            else if (bFirst && stateA) {                // Unexpected channel Reset A
                stateChange(1);
            }
            else if (!bFirst && stateB) {               // Unexpected channel Reset B
                stateChange(1);
            }
            else if (bFirst && stateB) {                // Advance state, expected channel B
                stateChange(3);
            }
            else if (!bFirst && stateA) {               // Advance state, expected channel A
                stateChange(3);
            }
        }
        //////////////////////////////////////////////////////// State 3 - One channel back high: ////////////////////////////////////////////////////////
        // Resets state machine if timeout elapses. Goes back to previous state if encoder channel that just went high goes low again.
        // Advances state machine if a single sample reads high in the expected encoder channels buffer.
        if (encoderState == 3) {
            if ((micros() - stateTime) >= 40000) {      // Timeout Reset (seconds)
                stateChange(0);
                return None;
            }
            else if (bFirst && !stateB) {               // Unexpected channel Reset B
                stateChange(2);
            }
            else if (!bFirst && !stateA) {              // Unexpected channel ResetA
                stateChange(2);
            }
            else if (bFirst && anyTrue(samplesA)) {     // Advance state, expected channel A
                stateChange(4);
            }
            else if (!bFirst && anyTrue(samplesB)) {    // Advance state, expected channel B
                stateChange(4);
            }
        }
        //////////////////////////////////////////////////////// State 4 - Both channels back high ////////////////////////////////////////////////////////
        // Resets state machine upon completion of this block.
        // Updates rotation visual.
        if (encoderState == 4) {
            stateChange(0);
            if (bFirst) {                               //Clockwise
                return enCW;
            }
            else {                                      //Counter clockwise
                return enCCW;
            }
        }
    }
}