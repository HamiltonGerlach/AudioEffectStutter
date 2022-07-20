#ifndef audioeffectstutter_h
#define audioeffectstutter_h

#define PIN_SWITCH 14
#define PIN_POT1 15
#define PIN_POT2 16
#define PIN_POT3 17
#define PIN_POT4 22

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

#define EQSAVETIMER 3000

inline int Ducy(float Percent) { return 256 - (int)floor(Percent * 256); }

#endif