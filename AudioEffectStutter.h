#ifndef audioeffectstutter_h
#define audioeffectstutter_h

#define PIN_TOGGLE 5
#define PIN_SWITCH 9
#define PIN_SWITCH_1 22
#define PIN_SWITCH_2 9

#define PIN_POT1 14
#define PIN_POT2 15
#define PIN_POT3 16
#define PIN_POT4 17

#define POT_MIXER 0
#define POT_BLEND 3
#define POT_ATTACK 2
#define POT_DECAY 1

// #define DEBUG_POT_VALUES 1

const int PIN_POT[4] = {14, 15, 16, 17};


#define EEPROM_GAIN1 0
#define EEPROM_GAIN2 1
#define EEPROM_FREQ1 2
#define EEPROM_FREQ2 3
#define EEPROM_EQSTATE 4

#define EEPROM_GAIN1_SCALE 1
#define EEPROM_GAIN1_OFFSET 16
#define EEPROM_GAIN2_SCALE 1
#define EEPROM_GAIN2_OFFSET 16
#define EEPROM_FREQ1_SCALE 1
#define EEPROM_FREQ1_OFFSET 16
#define EEPROM_FREQ2_SCALE 1
#define EEPROM_FREQ2_OFFSET 16

#define PIN_LED_R 2
#define PIN_LED_G 3
#define PIN_LED_B 4

#define EQSAVE_TIMER 3000
#define RETRIGGER_EXIT_TIME 3000
#define LOOPLENGTH_MIN 65
#define MOMENTARY_FREEZE_TIMER 2000
#define LED_BLINKTIME 125

#define ANALOG_FILTER_FREQ 2.0f
#define ANALOG_FILTER_WINDOW (20.0f / ANALOG_FILTER_FREQ)
#define ANALOG_FILTER_PRESCALE 1.000977517107f
#define ANALOG_FILTER_POST_RESCALE 1.001878522229f
#define ANALOG_GAIN_RECPR 32.0f
#define ANALOG_MAX 1024.0f
#define ANALOG_RESCALE 9.775171065498047e-4
#define ANALOG_OFFSET 16.0f
#define ANALOG_SHIFT (-0.000f)
#define ANALOG_SNAP_THRESHOLD 0.01f

#define EQ_PEAK1_FREQ 6000
#define EQ_PEAK2_FREQ 6000

#define EQ_PEAK1_FREQ_SCALE 350
#define EQ_PEAK2_FREQ_SCALE 250

#define EQ_PEAK1_GAINSCALE 2
#define EQ_PEAK2_GAINSCALE 2

#define EQ_PEAK1_Q 2
#define EQ_PEAK2_Q 1

#define SWITCH_DEB_TIME 35

#endif