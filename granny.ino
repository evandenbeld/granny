#include <MIDI.h>
#include <MozziGuts.h>
#include <Oscil.h>
#include <Line.h>
#include <mozzi_midi.h>
#include <ADSR.h>
#include <mozzi_fixmath.h>
#include <tables/sin2048_int8.h>
#include <tables/saw2048_int8.h>
#include <tables/triangle2048_int8.h>
#include <tables/square_no_alias_2048_int8.h>

MIDI_CREATE_DEFAULT_INSTANCE();

#define WAVE_SWITCH 2
#define SINE_WAVE_LED 5
#define TRIANGLE_WAVE_LED 6
#define SAW_WAVE_LED 7
#define SQUARE_WAVE_LED 8
#define LED 13

#define CONTROL_RATE 128

#define ATTACK 50
#define DECAY 50
#define SUSTAIN 5000
#define RELEASE 200

#define ATTACK_LEVEL 255
#define DECAY_LEVEL 255


#define NUMBER_OF_POLYPHONY 6
Oscil<2048, AUDIO_RATE> oscillators[NUMBER_OF_POLYPHONY];
ADSR<CONTROL_RATE, CONTROL_RATE> envelopes[NUMBER_OF_POLYPHONY];
byte notes[] = {0, 0, 0, 0, 0, 0};
byte gain[] = {0, 0, 0, 0, 0, 0};
byte waveNumber = 1;
int sensorPin = A0;
int decayValue = 50;

void updateLed() {
    int numberOfNotes = 0;
    for (int i = 0; i < NUMBER_OF_POLYPHONY; i++) {
        numberOfNotes += notes[i];
    }

    if (numberOfNotes == 0) {
        digitalWrite(LED, LOW);
    } else {
        digitalWrite(LED, HIGH);
    }
}

void HandleNoteOn(byte channel, byte note, byte velocity) {

    for (int i = 0; i < NUMBER_OF_POLYPHONY; i++) {
        if (notes[i] == 0) {
            notes[i] = note;
            oscillators[i].setFreq_Q16n16(Q16n16_mtof(Q8n0_to_Q16n16(note)));
            envelopes[i].noteOn();
            break;
        }
    }
    updateLed();
}


void HandleNoteOff(byte channel, byte note, byte velocity) {


    for (int i = 0; i < NUMBER_OF_POLYPHONY; i++) {
        if (note == notes[i]) {
            notes[i] = 0;;
            envelopes[i].noteOff();
            break;
        }
    }
    updateLed();
}

void setup() {

    pinMode(LED, OUTPUT);

    MIDI.begin(MIDI_CHANNEL_OMNI);
    MIDI.setHandleNoteOn(HandleNoteOn);
    MIDI.setHandleNoteOff(HandleNoteOff);

    for (int i = 0; i < NUMBER_OF_POLYPHONY; i++) {
        envelopes[i].setADLevels(ATTACK_LEVEL, DECAY_LEVEL);
        envelopes[i].setTimes(ATTACK, DECAY, SUSTAIN, RELEASE);
        //FIXME envelope potentiometers

        //FIXME update using the rotary switch
        oscillators[i].setTable(SIN2048_DATA);
    }


    startMozzi(CONTROL_RATE);
}

//void setWave(byte wav_num) { // good practice to use local parameters to avoid global confusion
//    static byte wave_indicator_led = 1; // static so value persists between calls
//    // light the corresponding led, according to selected wave type
//    digitalWrite(wave_indicator_led, LOW);
//    // switch/case is faster thsan if/else
//    switch (wav_num) {
//        case 1:
//            wave_indicator_led = SINE_WAVE_LED;
//            oscil1.setTable(SIN2048_DATA);
//            oscil2.setTable(SIN2048_DATA);
//            oscil3.setTable(SIN2048_DATA);
//            oscil4.setTable(SIN2048_DATA);
//            oscil5.setTable(SIN2048_DATA);
//            oscil6.setTable(SIN2048_DATA);
//            break;
//        case 2:
//            wave_indicator_led = TRIANGLE_WAVE_LED;
//            oscil1.setTable(TRIANGLE2048_DATA);
//            oscil2.setTable(TRIANGLE2048_DATA);
//            oscil3.setTable(TRIANGLE2048_DATA);
//            oscil4.setTable(TRIANGLE2048_DATA);
//            oscil5.setTable(TRIANGLE2048_DATA);
//            oscil6.setTable(TRIANGLE2048_DATA);
//            break;
//        case 3:
//            wave_indicator_led = SAW_WAVE_LED;
//            oscil1.setTable(SAW2048_DATA);
//            oscil2.setTable(SAW2048_DATA);
//            oscil3.setTable(SAW2048_DATA);
//            oscil4.setTable(SAW2048_DATA);
//            oscil5.setTable(SAW2048_DATA);
//            oscil6.setTable(SAW2048_DATA);
//            break;
//        case 4:
//            wave_indicator_led = SQUARE_WAVE_LED;
//            oscil1.setTable(SQUARE_NO_ALIAS_2048_DATA);
//            oscil2.setTable(SQUARE_NO_ALIAS_2048_DATA);
//            oscil3.setTable(SQUARE_NO_ALIAS_2048_DATA);
//            oscil4.setTable(SQUARE_NO_ALIAS_2048_DATA);
//            oscil5.setTable(SQUARE_NO_ALIAS_2048_DATA);
//            oscil6.setTable(SQUARE_NO_ALIAS_2048_DATA);
//            break;
//    }
//    digitalWrite(wave_indicator_led, HIGH);
//}


void updateControl() {
    MIDI.read();

    for (int i = 0; i < NUMBER_OF_POLYPHONY; i++) {
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
