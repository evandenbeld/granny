#include <MIDI.h>
#include <MozziGuts.h>
#include <Oscil.h>
#include <Line.h>
#include <mozzi_midi.h>
#include <ADSR.h>
#include <mozzi_fixmath.h>
#include <AutoMap.h>
#include <tables/sin2048_int8.h>
#include <tables/saw2048_int8.h>
#include <tables/triangle2048_int8.h>
#include <tables/square_analogue512_int8.h>
#include <tables/brownnoise8192_int8.h>

#define CONTROL_RATE 128

#define PIN_LED 13

#define PIN_SINE_WAVE_SWITCH 2
#define PIN_SAW_WAVE_SWITCH 3
#define PIN_TRIANGLE_WAVE_SWITCH 4
#define PIN_SQUARE_WAVE_SWITCH 5
#define PIN_NOISE_SWITCH 6

#define ATTACK_PIN 0
#define DECAY_PIN 1
#define SUSTAIN_PIN 5
#define RELEASE_PIN 2

#define ATTACK 50
#define DECAY 50
#define SUSTAIN 5000
#define RELEASE 200
#define ATTACK_LEVEL 255
#define DECAY_LEVEL 255

#define NUMBER_OF_POLYPHONY 6

enum WAVE_TABLE {
    SINE,
    SAW,
    TRIANGLE,
    SQUARE,
    NOISE
};


Oscil<2048, AUDIO_RATE> oscillators[NUMBER_OF_POLYPHONY];
ADSR<CONTROL_RATE, CONTROL_RATE> envelopes[NUMBER_OF_POLYPHONY];
byte notes[] = {0, 0, 0, 0, 0, 0};
byte gain[] = {0, 0, 0, 0, 0, 0};
int sensorPin = A0;
int decayValue = 50;
unsigned int attack, decay, sustain, release_ms;
AutoMap adsrKnobMapper(0, 1023, 50, 5000);

MIDI_CREATE_DEFAULT_INSTANCE();

void updateLed() {
    int numberOfNotes = 0;
    for (int i = 0; i < NUMBER_OF_POLYPHONY; i++) {
        if (notes[i] != 0) numberOfNotes++;
    }

    if (numberOfNotes == 0) {
        digitalWrite(PIN_LED, LOW);
    } else {
        digitalWrite(PIN_LED, HIGH);
    }
}

void HandleNoteOn(byte channel, byte note, byte velocity) {

    for (int i = 0; i < NUMBER_OF_POLYPHONY; i++) {
        if (notes[i] == 0) {
            notes[i] = note;
            oscillators[i].setFreq_Q16n16(Q16n16_mtof(Q8n0_to_Q16n16(note)));
            envelopes[i].noteOn();
            i = NUMBER_OF_POLYPHONY;
        }
    }
    updateLed();
}


void HandleNoteOff(byte channel, byte note, byte velocity) {


    for (int i = 0; i < NUMBER_OF_POLYPHONY; i++) {
        if (note == notes[i]) {
            notes[i] = 0;;
            envelopes[i].noteOff();
            i = NUMBER_OF_POLYPHONY;
        }
    }
    updateLed();
}

void setup() {

    pinMode(PIN_LED, OUTPUT);
    pinMode(PIN_SINE_WAVE_SWITCH, INPUT_PULLUP);
    pinMode(PIN_SAW_WAVE_SWITCH, INPUT_PULLUP);
    pinMode(PIN_TRIANGLE_WAVE_SWITCH, INPUT_PULLUP);
    pinMode(PIN_SQUARE_WAVE_SWITCH, INPUT_PULLUP);
    pinMode(PIN_NOISE_SWITCH, INPUT_PULLUP);

    MIDI.begin(MIDI_CHANNEL_OMNI);
    MIDI.setHandleNoteOn(HandleNoteOn);
    MIDI.setHandleNoteOff(HandleNoteOff);

    for (int i = 0; i < NUMBER_OF_POLYPHONY; i++) {
        envelopes[i].setADLevels(ATTACK_LEVEL, DECAY_LEVEL);
        envelopes[i].setTimes(ATTACK, DECAY, SUSTAIN, RELEASE);
        //FIXME envelope potentiometers

        oscillators[i].setTable(SIN2048_DATA);
    }


    startMozzi(CONTROL_RATE);
}

void updateWaveTable() {


    WAVE_TABLE waveTable = SINE;
    if (digitalRead(PIN_SINE_WAVE_SWITCH) == LOW) {
        waveTable = SINE;
    } else if (digitalRead(PIN_SAW_WAVE_SWITCH) == LOW) {
        waveTable = SAW;
    } else if (digitalRead(PIN_TRIANGLE_WAVE_SWITCH) == LOW) {
        waveTable = TRIANGLE;
    } else if (digitalRead(PIN_SQUARE_WAVE_SWITCH) == LOW) {
        waveTable = SQUARE;
    } else if (digitalRead(PIN_NOISE_SWITCH) == LOW) {
        waveTable = NOISE;
    }


    for (int i = 0; i < NUMBER_OF_POLYPHONY; i++) {
        switch (waveTable) {
            case SINE:
                oscillators[i].setTable(SIN2048_DATA);
                break;
            case TRIANGLE:
                oscillators[i].setTable(TRIANGLE2048_DATA);
                break;
            case SAW:
                oscillators[i].setTable(SAW2048_DATA);
                break;
            case SQUARE:
                oscillators[i].setTable(SQUARE_ANALOGUE512_DATA);
                break;
            case NOISE:
                oscillators[i].setTable(BROWNNOISE8192_DATA);
                break;
        }
    }
}


void updateControl() {

    updateWaveTable();

    int knob_value = mozziAnalogRead(ATTACK_PIN);
    attack = adsrKnobMapper(knob_value);
    knob_value = mozziAnalogRead(DECAY_PIN);
    decay = adsrKnobMapper(knob_value);
    knob_value = mozziAnalogRead(SUSTAIN_PIN);
    sustain = adsrKnobMapper(knob_value);
    knob_value = mozziAnalogRead(RELEASE_PIN);
    release_ms = adsrKnobMapper(knob_value);

    MIDI.read();

    for (int i = 0; i < NUMBER_OF_POLYPHONY; i++) {
        envelopes[i].setTimes(attack, decay, sustain, release_ms);
        envelopes[i].update();
        gain[i] = envelopes[i].next();
    }
}


int updateAudio() {

    long audio = 0;
    for (int i = 0; i < NUMBER_OF_POLYPHONY; i++) {
        audio += (long) gain[i] * oscillators[i].next();
    }

    return audio >> 10; //FIXME 6 for hifi
}


void loop() {
    audioHook();
} 
