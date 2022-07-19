#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <Bounce.h>
#include <EEPROM.h>

#include "AudioEffectStutter.h"
#include "effect_stutter.h"

AudioInputI2S                       audioInput;         //xy=471,791
AudioAnalyzePeak                    peak_L;                 //xy=803,448
AudioAnalyzePeak                    peak_R;                 //xy=818,503
AudioAnalyzeRMS                     rms1;                     //xy=847,552
AudioFilterBiquad                   biquad1;                //xy=886,745
AudioAnalyzePeak                    peak1;                    //xy=1018,896
AudioAmplifier                      amp1;                     //xy=1040,831
AudioMixer4                         mixer1;                 //xy=1069,668
AudioOutputI2S                      audioOutput;        //xy=1305,745
AudioEffectStutter                  stutter;

AudioConnection                     patchCord1(audioInput, 0, peak_L, 0);
AudioConnection                     patchCord2(audioInput, 0, rms1, 0);

AudioConnection                     patchCord3(audioInput, 0, biquad1, 0);
AudioConnection                     patchCord4(audioInput, 0, mixer1, 0);
AudioConnection                     patchCord7(biquad1, amp1);
AudioConnection                     patchCord8(amp1, peak1);
AudioConnection                     patchCord9(amp1, 0, mixer1, 1);
AudioConnection                     patchCord10(mixer1, 0, stutter, 0);
AudioConnection                     patchCord11(stutter, 0, audioOutput, 0);

AudioConnection                     patchCord5(audioInput, 1, peak_R, 0);
AudioConnection                     patchCord6(audioInput, 1, audioOutput, 1);
AudioControlSGTL5000                sgtl5000_1;         //xy=620,538


bool IsClipping = false;
unsigned long ClippingTimer = 0;

bool SwitchPressed = false;
unsigned long SwitchTimer = 0;

int PedalMode = 0; // 0 ... Stutter, EQ setup

Bounce btn = Bounce(PIN_SWITCH, 50);

bool State = true;
int Level = 16;
float PostEqGain = 1.0;

byte Freq1, Freq2;
byte Gain1, Gain2;
bool EqActive;

void setup()
{
    AudioMemory(14340);
    sgtl5000_1.enable();
    sgtl5000_1.inputSelect(AUDIO_INPUT_LINEIN);
    sgtl5000_1.volume(0.0);
    sgtl5000_1.micGain(0);
    sgtl5000_1.lineInLevel(0);
    sgtl5000_1.lineOutLevel(Level);
    Serial.begin(9600);
    
    pinMode(PIN_SWITCH, INPUT_PULLUP);
    pinMode(PIN_POT1, INPUT);
    pinMode(PIN_POT2, INPUT);
    pinMode(PIN_POT3, INPUT);
    pinMode(PIN_POT4, INPUT);
    
    Gain1 = (EEPROM.read(EEPROM_GAIN1) - EEPROM_GAIN1_OFFSET) / EEPROM_GAIN1_SCALE;
    Gain2 = (EEPROM.read(EEPROM_GAIN2) - EEPROM_GAIN2_OFFSET) / EEPROM_GAIN2_SCALE;
    Freq1 = (EEPROM.read(EEPROM_FREQ1) - EEPROM_FREQ1_OFFSET) / EEPROM_FREQ1_SCALE;
    Freq2 = (EEPROM.read(EEPROM_FREQ2) - EEPROM_FREQ2_OFFSET) / EEPROM_FREQ2_SCALE;
    EqActive = EEPROM.read(EEPROM_EQSTATE);
        
    delay(2000);
    
    if (digitalRead(PIN_SWITCH) == LOW) { PedalMode = 1; }
    
    Serial.println(EqActive);
    Serial.println(PedalMode);
    
    amp1.gain(PostEqGain);
    
    // Bypass EQ
    if (EqActive == 0)
    {        
        mixer1.gain(0, 0.0f);
        mixer1.gain(1, 1.0f);
    }
    else
    {        
        mixer1.gain(0, 1.0f);
        mixer1.gain(1, 0.0f);
    }
    
    
    analogWrite(2, 1024);
    analogWrite(3, 1024);
    analogWrite(4, 1024);
    
    biquad1.setPeaking(0, 6000 - Freq1 * 350, -Gain1 * 2, 2);
    biquad1.setPeaking(1, 4000 - Freq2 * 150, -Gain2 * 2, 6);
}

int last = 0;
void loop() {
    // Monitor clipping
    if (peak_L.available()) {
        if (peak_L.read() > 0.999) {
            ClippingTimer = millis();
            IsClipping = true;
            analogWrite(3, 1024);
            analogWrite(4, 1024);
            analogWrite(2, 0);
        }
    }
    
    // Reset clipping LED
    if (millis() - ClippingTimer > 1000) {
        IsClipping = false;
        analogWrite(2, 1024);
    }
    
    
    // Read pots
    int pot1 = analogRead(PIN_POT1);
    int pot2 = analogRead(PIN_POT2);
    int pot3 = analogRead(PIN_POT3);
    int pot4 = analogRead(PIN_POT4);
    
    int pot1conv = round(pot1 / 32 - 15.5);
    int pot2conv = round(pot2 / 32 - 15.5);
    int pot3conv = round(pot3 / 32 - 15.5);
    int pot4conv = round(pot4 / 32 - 15.5);
    
    
    if (rms1.available())
    {
        // Debug output converted pot values
        // Serial.print("Pot 1: ");
        // Serial.print(pot1conv);
        // Serial.print(", Pot 2: ");
        // Serial.print(pot2conv);
        // Serial.print(", Pot 3: ");
        // Serial.print(pot3conv);
        // Serial.print(", Pot 4: ");
        // Serial.print(pot4conv);
        // Serial.println(" ");
    }
    
    btn.update();

    
    
    if (PedalMode == 1)
    {
        // Toggle EQ
        if (btn.risingEdge())
        {
            if (EqActive == 0)
            {
                mixer1.gain(1, 0.0f);
                mixer1.gain(0, 1.0f);
                
                EqActive = 1;
                
                Serial.println("Switching EQ on.");
            }
            else
            {
                mixer1.gain(0, 0.0f);
                mixer1.gain(1, 1.0f);
                
                EqActive = 0;
                
                Serial.println("Switching EQ off.");
            }
        }
        
        if (EqActive == 0) {
            analogWrite(3, 1024);
            analogWrite(4, 512);
        }
        else {
            analogWrite(3, 1024);
            analogWrite(4, 512);
        }
        
        
        // Update EQ
        if (millis() - last > 50) {
            biquad1.setPeaking(0, 6000 - pot1conv * 350, -pot3conv * 2, 2);
            biquad1.setPeaking(1, 4000 - pot2conv * 150, -pot4conv * 2, 6);
            last = millis();
        }
        
        
        // Exit EQ setup mode
        if (digitalRead(PIN_SWITCH) == LOW) {
            if (SwitchPressed) {
                if ((millis() - SwitchTimer) > EQSAVETIMER) {
                    PedalMode = 0;
                    
                    Serial.println("Writing EEPROM data.");
                    Serial.println(EqActive);
                    Serial.println(pot1conv);
                    Serial.println(pot2conv);
                    Serial.println(pot3conv);
                    Serial.println(pot4conv);
                    
                    EEPROM.write(EEPROM_FREQ1, pot1conv * EEPROM_FREQ1_SCALE + EEPROM_FREQ1_OFFSET);
                    EEPROM.write(EEPROM_FREQ2, pot2conv * EEPROM_FREQ2_SCALE + EEPROM_FREQ2_OFFSET);
                    EEPROM.write(EEPROM_GAIN1, pot3conv * EEPROM_GAIN1_SCALE + EEPROM_GAIN1_OFFSET);
                    EEPROM.write(EEPROM_GAIN2, pot4conv * EEPROM_GAIN2_SCALE + EEPROM_GAIN2_OFFSET);
                    EEPROM.write(EEPROM_EQSTATE, EqActive);
                }
            }
            else {
                SwitchTimer = millis(); SwitchPressed = true;
            }
        }
        else {
            SwitchPressed = false;
        }
    }
    else
    {
        // Stutter / Looper control
        if (btn.fallingEdge())
        {
            if (!stutter.isActive()) { stutter.snap(); 
                analogWrite(3, 0);
                analogWrite(4, 1024);
            } else {
                stutter.unlatch();
                analogWrite(3, 1024);
                analogWrite(4, 1024);
            }
        }
        else if (btn.risingEdge())
        {
            if (stutter.isActive()) {
                stutter.latch();
                analogWrite(3, 1024);
                analogWrite(4, 0);
            } else {
                analogWrite(2, 1024);
                analogWrite(4, 1024);
            }
        }
        
        
        if (millis() - last > 50) {
            // Check pot values for crossfades, momentary/latch mode, dry/wet blend etc.
        }
    }
    
    
    
    if (EqActive == true)
    {
        // Monitor post EQ clipping
        if (peak1.available())
        {
            float PeakPostEQ = peak1.read();
            
            if (PeakPostEQ > 0.999)
            {
                PostEqGain -= 0.1;
                Level += 1;

                amp1.gain(PostEqGain);
                sgtl5000_1.lineOutLevel(Level);

                Serial.println("Adapting post EQ gain:");
                Serial.println(PostEqGain);
                Serial.println(Level);
            }
        }
    }
    
}
