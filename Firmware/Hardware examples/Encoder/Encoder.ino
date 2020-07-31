// Variables for the rotary encoder:
#define  samplesPerMs 40                                                // How many encoder samples to capture per millisecond.
#define  sampleTime 0.00025                                             // Samples time in seconds.
#define  sampleDwelluS ((0.001 / samplesPerMs) * 1000000)               // Wait time between samples in microseconds to free up processor for other things.
const int sampleBufferSize = (samplesPerMs * (sampleTime * 1000));      // Size of the sample buffer for each encoder channel.
bool samplesB[sampleBufferSize];                                        // Buffer that stores [sampleBufferSize] number of samples from encoder channel B.
bool samplesA[sampleBufferSize];                                        // Buffer that stores [sampleBufferSize] number of samples from encoder channel A.
byte bufferIndex = 0;                                                   // Keeps track of where in samplesB/A to write the next sample.  Ensures FIFO.
unsigned long lastSampleTime = micros();                                // Last time encoder channels were sampled in microseconds.  Used for sample timing.
unsigned long stateTime = micros();                                     // Contains the last time the state was changed in microseconds.
byte encoderState = 0;                                                  // States: Idle, One channel low, Both channels low, One channel back high, Both channels back high (0, 1, 2, 3, 4 respectively)
unsigned long encoderTriggerTime = micros();                            // Time that the callback was triggered in microseconds.

// Variables specifically for the encoder button:
bool buttonLastState = false;                                           // Contains the previous state of the encoder button.  Used to execute code on release of the button.
const byte buttonDebounce = 200;                                        // Debounce time for the encoder button in milliseconds.
unsigned long buttonLastPressed = millis();                             // Last time the encoder button was pressed in milliseconds.

// Encoder pin assignments:
byte EnApin = 10;                                                       // Arduino pin on encoder channel A.
byte EnBpin = 11;                                                       // Arduino pin on encoder channel B.
byte btnPin = 12;                                                       // Arduino pin on encoder button.

// Debug things:
#define DEBUG  // comment thi line out to disable debugging (printing) throughout the program.

#ifdef DEBUG
    #define DEBUG_PRINT(x)  Serial.print(x)
    #define DEBUG_PRINTLN(x)  Serial.println(x)
#else
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINTLN(x)
#endif

#ifdef DEBUG
    char counterString [101] = {'\0'};     // Serves as a visual representation of rotary input. +1 for EOL character '\0'
    byte counterStringIndex = 0;           // Tracks the current index to write to counterString

    // This allows for a single call to print a bunch of things of interest
    void printVariables() {
        DEBUG_PRINT("bufferIndex = ");
        DEBUG_PRINTLN(bufferIndex);
        DEBUG_PRINT("encoderState = ");
        DEBUG_PRINTLN(encoderState);

        DEBUG_PRINT("samplesA = ");
        for (byte i = 0; i < sampleBufferSize; i++) {
            DEBUG_PRINT(samplesA[i]);
            DEBUG_PRINT(" ");
        }
        DEBUG_PRINTLN("");

        DEBUG_PRINT("samplesB = ");
        for (byte i = 0; i < sampleBufferSize; i++) {
            DEBUG_PRINT(samplesB[i]);
            DEBUG_PRINT(" ");
        }
        DEBUG_PRINTLN("");
    }
#else
    void printVariables() {
        return;
    }
#endif

void setup() {
    pinMode(EnApin, INPUT);
    pinMode(EnBpin, INPUT);
    pinMode(btnPin, INPUT);

    initializeBuffer(samplesB);
    initializeBuffer(samplesA);

    #ifdef DEBUG
        Serial.begin(250000);
    #endif
}

// Variables used exclusively in the loop():
bool EnAtrig = true;    // EnA is acrive low
bool EnBtrig = true;    // EnB is acrive low
bool btnTrig = true;    // btn is acrive low

void loop() {
    EnAtrig = digitalRead(EnApin);
    EnBtrig = digitalRead(EnBpin);
    btnTrig = digitalRead(btnPin);

    if (!EnAtrig || !EnBtrig) {         // if either EnA or EnB is low, record time and handle accordingly
        encoderTriggerTime = micros();
        handleEncoder();
    }
    if (!btnTrig || !buttonLastState) { // if buttonLastState is low, keep checking button until it goes hi again (is no longer being pressed)
        handleEncoderPress();
    }
}

// Initializes all elements of buffer to true
void initializeBuffer(bool buffer[]) {
    for (byte i = 0; i < sampleBufferSize; i++) {
        buffer[i] = true;
    }
}

// Is used specifically to sample the encoder button.
byte sampleChannel(byte channel, byte sampleQuantity) {
    float sample = 0;                               // Stores IO samples from encoder button.

    for(byte i = 0; i < sampleQuantity; i++) {      // Get samples
        sample = sample + digitalRead(channel);     // add samples together to get average value
    }
    return round(sample / sampleQuantity);
}

// Executes code when the encoder button is pressed.
void handleEncoderPress() {
    bool state = sampleChannel(btnPin, 10);                                                 // sample 10 times

    if (!buttonLastState && state && (millis() > (buttonLastPressed + buttonDebounce))) {   // Button is active low, execute when button goes back to high (no longer pressed)
        buttonLastPressed = millis();
        DEBUG_PRINTLN("---------ENCODER BUTTON PRESSED---------");
    }
    else {
        // DEBUG_PRINTLN("----------BUTTON PRESS IGNORED!-------------");
    }
    buttonLastState = state;
}

// Adds elements to the sample buffers and advances the buffer index.  The index is used to ensure FIFO for elements added to the buffer.
void elementsToBuffer (bool EnB, bool EnA) {
    samplesB[bufferIndex] = EnB;
    samplesA[bufferIndex] = EnA;
    bufferIndex++;
    if (bufferIndex == sampleBufferSize) {
        bufferIndex = 0;
    }
}

// Returns true if all elements in samples are true.
bool allTrue (bool samples[]) {
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
bool anyTrue (bool samples[]) {
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
void stateChange(byte stateNumber) {               
    encoderState = stateNumber;
    stateTime = micros();
}

// Handles the rotary input of the encoder.
void handleEncoder() { 

    // Encoder state variables:
    bool stateB = true;                     // State of encoder channel B (filtered). Logic is made to match that of the encoder.
    bool stateA = true;                     // State of encoder channel A (filtered). Logic is made to match that of the encoder.
    bool bFirst = false;                    // Indicates if encoder channel B went low first.

    if (micros() <= (stateTime + 1500)) {   // Adds debounce to encoder rotation detection.
        return;
    }

    // This is the state machine for trtacking encoder state changes.
    while (true) {
        //////////////////////////////////////////////////////// SAMPLE CHANNELS ////////////////////////////////////////////////////////
        // This first block alwayse executes.
        // It takes samples and updates the encoder channels filtered values every time it executes.
        // Advances state machine when either encoder channels buffer reads all false.
        if (micros() >= lastSampleTime + sampleDwelluS) {   // This 'if' samples both encoder channels 'samplesPerMs' times a millisecond.
            lastSampleTime = micros();
            elementsToBuffer(digitalRead(EnBpin), digitalRead(EnApin));
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
            if (micros() >= (encoderTriggerTime + 3000)){
                stateChange(0);
                break;
            }
        }
        //////////////////////////////////////////////////////// State 1 - One channel low: ////////////////////////////////////////////////////////
        // Resets state machine if timeout elapses. Goes back to previous state if the encoder channel that went low first goes high again.
        // Advances state machine if both encoder channels filtered logic are low at the same time.
        if (encoderState == 1) {
            if (micros() >= (stateTime + 200000)) {      // Timeout Reset (seconds)
                stateChange(0);
                break;
            }
            else if (bFirst && stateB) {                // Unexpected channel B Reset
                stateChange(0);
                break;
            }
            else if (!bFirst && stateA) {               // Unexpected channel A Reset
                stateChange(0);
                break;
            }
            else if (!stateB && !stateA) {              // Advance state machine 
                stateChange(2);
            }
        }
        //////////////////////////////////////////////////////// State 2 - Both channels low: ////////////////////////////////////////////////////////
        // Resets state machine if timeout elapses. Goes back to previous state if wrong encoder channel goes high first.
        // Advances state machine if the correct channel goes high.
        if (encoderState == 2) {
            if (micros() >= (stateTime + 110000)) {     // Timeout Reset (seconds)
                stateChange(0);
                break;
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
            if (micros() >= (stateTime + 40000)) {      // Timeout Reset (seconds)
                stateChange(0);
                break;
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
            if (bFirst) {
                #ifdef DEBUG                            // Counter ClockWise visual
                    counterString [counterStringIndex] = '>';
                    counterStringIndex++;
                    counterString [counterStringIndex] = '\0';
                    DEBUG_PRINTLN(counterString);
                    DEBUG_PRINTLN("CCW");
                #endif
            }
            else {
                #ifdef DEBUG                            // ClockWise visual
                    counterString [counterStringIndex] = '\0';
                    counterStringIndex--;
                    DEBUG_PRINTLN(counterString);
                    DEBUG_PRINTLN("CW");
                #endif
            }
            stateChange(0);
            break;
        }
    }
}