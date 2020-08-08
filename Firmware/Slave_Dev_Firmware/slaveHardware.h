/*
  slaveHardware.h - Library for Development Bench slave device.
*/

#include "Arduino.h"

// Library for Keypad
#include "Keypad.h"

#ifndef slaveHardware_h
#define slaveHardware_h

#define  samplesPerMs 40                                                // How many encoder samples to capture per millisecond.
#define  sampleTime 0.00025                                             // Samples time in seconds.
#define sampleBufferSize (byte) (samplesPerMs * (sampleTime * 1000))    // Size of the sample buffer for each encoder channel.

enum userEvent {
  None,
  S1,
  S2,
  S3,
  S4,
  S5,
  S6,
  S7,
  S8,
  S9,
  S10,
  S11,
  S12,
  S13,
  S14,
  S15,
  S16,
  enCW,
  enCCW,
  enBtn
};

/*
  Encoder - class for handling the rotary encoder.
  Initialize the encoder with the two channel pins and the button pin assignments
  Place checkEncoder(); in your program loop to periodically check the encoder.
  Encoder events are not buffered or queued.
  If they are not checked when they happen, they are lost.
*/
class Encoder {
    public:
        Encoder(byte EnApin, byte EnBpin, byte btnPin);
        userEvent checkEncoder(void);
    private:
        // Encoder pin assignments:
        byte _EnApin;
        byte _EnBpin;
        byte _btnPin;

        // Variables for the rotary encoder:
        bool samplesB[sampleBufferSize];
        bool samplesA[sampleBufferSize];
        byte bufferIndex;
        unsigned long lastSampleTime;
        unsigned long stateTime;
        byte encoderState;
        unsigned long encoderTriggerTime; 

        // Variables specifically for the encoder button:
        bool buttonLastState;
        const byte buttonDebounce;
        unsigned long buttonLastPressed;

        //  These variables are TRUE when their respetive events are detected:
        bool EnAtrig = true;    // EnA is acrive low
        bool EnBtrig = true;    // EnB is acrive low
        bool btnTrig = true;    // btn is acrive low

        void initializeBuffer(bool buffer[]);
        byte sampleChannel(byte channel, byte sampleQuantity);
        userEvent handleEncoderPress();
        void elementsToBuffer (bool EnB, bool EnA);
        bool allTrue (bool samples[]);
        bool anyTrue (bool samples[]);
        void stateChange(byte stateNumber);
        userEvent handleEncoder(void);
};

#endif