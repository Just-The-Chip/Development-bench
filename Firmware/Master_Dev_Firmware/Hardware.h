/*
  Hardware.h - Library for Nerd_Night[] hardware.
*/

#include "Arduino.h"

#ifndef Hardware_h
#define Hardware_h

#define  samplesPerMs 40                                                // How many encoder samples to capture per millisecond.
#define  sampleTime 0.00025                                             // Samples time in seconds.
#define sampleBufferSize (byte) (samplesPerMs * (sampleTime * 1000))    // Size of the sample buffer for each encoder channel.

/*
  Encoder - class for handling the rotary encoder.
  Initialize the encoder with the two channel pins and the button pin assignments,
  along with callbacks for clockwise and counter-clockwise rotation, and the button press.
  Place checkEncoder(); in your program loop to periodically check the encoder.
  Encoder events are not buffered or queued.
  If they are not checked when they happen, they are lost.
*/
class Encoder {
    public:
        Encoder(byte EnApin, byte EnBpin, byte btnPin, void (*CWcallback) (void), void (*CCWcallback) (void), void (*buttonCallback) (void));
        void checkEncoder();
    private:
        // Encoder pin assignments:
        byte _EnApin;
        byte _EnBpin;
        byte _btnPin;

        // Callback pointers
        void (*_CWptr) (void);
        void (*_CCWptr) (void);
        void (*_btnPtr) (void);

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
        void handleEncoderPress();
        void elementsToBuffer (bool EnB, bool EnA);
        bool allTrue (bool samples[]);
        bool anyTrue (bool samples[]);
        void stateChange(byte stateNumber);
        void handleEncoder();
};

#endif