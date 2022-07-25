#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <Bounce.h>
#include <EEPROM.h>

#include "AudioEffectStutter.h"
#include "effect_stutter.h"
#include "CtrlLED.h"

AudioInputI2S                       audioInput;         //xy=471,791
AudioAnalyzePeak                    peak_L;                 //xy=803,448
AudioAnalyzePeak                    peak_R;                 //xy=818,503
AudioAnalyzeRMS                     rms1;                     //xy=847,552
AudioFilterBiquad                   biquad1;                //xy=886,745
AudioAnalyzePeak                    peak1;                    //xy=1018,896
AudioAmplifier                      amp1;                     //xy=1040,831
AudioMixer4                         mixer_posteq;                 //xy=1069,668
AudioMixer4                         mixer_poststutter;                 //xy=1069,668
AudioOutputI2S                      audioOutput;        //xy=1305,745
AudioEffectStutter                  stutter;

AudioConnection                     patchCord1(audioInput, 0, peak_L, 0);
AudioConnection                     patchCord2(audioInput, 0, rms1, 0);

AudioConnection                     patchCord3(audioInput, 0, biquad1, 0);
AudioConnection                     patchCord4(audioInput, 0, mixer_posteq, 0);
AudioConnection                     patchCord7(biquad1, 0, amp1, 0);
AudioConnection                     patchCord8(amp1, 0, peak1, 0);
AudioConnection                     patchCord9(amp1, 0, mixer_posteq, 1);
AudioConnection                     patchCord10(mixer_posteq, 0, stutter, 0);
AudioConnection                     patchCord10a(stutter, 0, mixer_poststutter, 0);
AudioConnection                     patchCord10b(mixer_posteq, 0, mixer_poststutter, 1);
AudioConnection                     patchCord11(mixer_poststutter, 0, audioOutput, 0);

AudioConnection                     patchCord5(audioInput, 1, peak_R, 0);
AudioConnection                     patchCord6(audioInput, 1, audioOutput, 1);
AudioControlSGTL5000                sgtl5000_1;         //xy=620,538

CtrlLED LED = CtrlLED();

bool IsClipping = false;
unsigned long ClippingTimer = 0;

bool SwitchPressed = false;
unsigned long SwitchTimer = 0;

bool FreezeBlinking = false;
unsigned long FreezeBlinkTimer = 0;

unsigned long ms = 0;

int PedalMode = 0; // 0 ... Stutter, 1 ... EQ setup
int LoopMode = 0; // 0 ... Latch, 1 ... Momentary, 2 ... Chaotic?

Bounce btn = Bounce(PIN_SWITCH, 50);

bool State = true, MomentarySnapped = false, Retrigger = false;
int Level = 16;
float PostEqGain = 1.0, RecordBlend = 1.0;

byte Freq1, Freq2;
byte Gain1, Gain2;
bool EqActive;

int Blend, LoopLength, FadeLength;

void setup()
{
    AudioMemory(14340);
    sgtl5000_1.enable();
    sgtl5000_1.inputSelect(AUDIO_INPUT_LINEIN);
    sgtl5000_1.volume(0.0);
    sgtl5000_1.micGain(0);
    sgtl5000_1.lineInLevel(0);
    sgtl5000_1.lineOutLevel(Level);
    
    pinMode(PIN_SWITCH, INPUT_PULLUP);
    
    pinMode(PIN_POT1, INPUT);
    pinMode(PIN_POT2, INPUT);
    pinMode(PIN_POT3, INPUT);
    pinMode(PIN_POT4, INPUT);
    
    pinMode(PIN_LED_R, OUTPUT);
    pinMode(PIN_LED_G, OUTPUT);
    pinMode(PIN_LED_B, OUTPUT);
    
    Gain1 = (EEPROM.read(EEPROM_GAIN1) - EEPROM_GAIN1_OFFSET) / EEPROM_GAIN1_SCALE;
    Gain2 = (EEPROM.read(EEPROM_GAIN2) - EEPROM_GAIN2_OFFSET) / EEPROM_GAIN2_SCALE;
    Freq1 = (EEPROM.read(EEPROM_FREQ1) - EEPROM_FREQ1_OFFSET) / EEPROM_FREQ1_SCALE;
    Freq2 = (EEPROM.read(EEPROM_FREQ2) - EEPROM_FREQ2_OFFSET) / EEPROM_FREQ2_SCALE;
    EqActive = EEPROM.read(EEPROM_EQSTATE);
    
    for (int i = 0; i < 8; i++) {
        LED.SetRGB(0.25f, 0.0f, 0.25f); delay(125);
        LED.Flush(); delay(125);
    }
    
    
    if (digitalRead(PIN_SWITCH) == LOW) { PedalMode = 1; }
    
    amp1.gain(PostEqGain);
    
    // Bypass EQ
    if (EqActive == 1)
    {        
        mixer_posteq.gain(0, 0.0f);
        mixer_posteq.gain(1, 1.0f);
    }
    else
    {        
        mixer_posteq.gain(0, 1.0f);
        mixer_posteq.gain(1, 0.0f);
    }
    
    mixer_poststutter.gain(0, 1.0f);
    mixer_poststutter.gain(1, 1.0f);
    
    biquad1.setPeaking(0, 6000 - Freq1 * 350, -Gain1 * 2, 2);
    biquad1.setPeaking(1, 4000 - Freq2 * 150, -Gain2 * 2, 6);
    
    if (PedalMode == 1)
    {
        for (int i = 0; i < 2; i++) {
            LED.SetRGB(0.0f, 0.25f, 0.25f); delay(125);
            LED.Flush(); delay(125);
        }
        
        if (EqActive == 0)  { LED.SetRGB(0.125f, 0.05f, 0.0f);  }
        else                { LED.SetRGB(0.0f, 0.125f, 0.125f); }
    }
    
    
    Serial.begin(9600);
}



int last = 0;
void loop() {    
    // Read pots
    int pot1 = analogRead(PIN_POT1);
    int pot2 = analogRead(PIN_POT2);
    int pot3 = analogRead(PIN_POT3);
    int pot4 = analogRead(PIN_POT4);
    
    int pot1conv = round(pot1 / 32.0f - 16);
    int pot2conv = round(pot2 / 32.0f - 16);
    int pot3conv = round(pot3 / 32.0f - 16);
    int pot4conv = round(pot4 / 32.0f - 16);
    
    
    if (rms1.available()) {}
    
    btn.update();

    
    
    
    if (PedalMode == 1)
    {
        // Toggle EQ
        if (btn.risingEdge())
        {
            if (EqActive == 0)
            {
                mixer_posteq.gain(0, 0.0f);
                mixer_posteq.gain(1, 1.0f);
                
                EqActive = 1;
                LED.SetRGB(0.125f, 0.05f, 0.0f);
            }
            else
            {
                mixer_posteq.gain(0, 1.0f);
                mixer_posteq.gain(1, 0.0f);
                
                EqActive = 0;
                LED.SetRGB(0.0f, 0.125f, 0.125f);
            }
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
                if ((millis() - SwitchTimer) > EQSAVE_TIMER) {
                    PedalMode = 0;
                                        
                    EEPROM.write(EEPROM_FREQ1, pot1conv * EEPROM_FREQ1_SCALE + EEPROM_FREQ1_OFFSET);
                    EEPROM.write(EEPROM_FREQ2, pot2conv * EEPROM_FREQ2_SCALE + EEPROM_FREQ2_OFFSET);
                    EEPROM.write(EEPROM_GAIN1, pot3conv * EEPROM_GAIN1_SCALE + EEPROM_GAIN1_OFFSET);
                    EEPROM.write(EEPROM_GAIN2, pot4conv * EEPROM_GAIN2_SCALE + EEPROM_GAIN2_OFFSET);
                    EEPROM.write(EEPROM_EQSTATE, EqActive);
                    
                    LED.SetG(0.0f); 
                    for (int i = 0; i < 4; i++) {
                        LED.SetRGB(0.25f, 0.0f, 0.125f); delay(125);
                        LED.Flush(); delay(125);
                    }
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
        ms = millis();
        
        // Loop mode selection update
        if (pot2conv <= -8) {
            LoopMode = 3;
        }
        else if (pot2conv > -8 && pot2conv <= 0) {
            LoopMode = 2;
        }
        else if (pot2conv > 0 && pot2conv <= 8) {
            LoopMode = 1;
        }
        else {
            LoopMode = 0;
            MomentarySnapped = false;
        }
        
        // Blend update
        Blend = pot3conv;
        
        // Retrigger mode / loop length update
        Retrigger = (pot4conv <= 0) ? true : false;
        LoopLength = round(((0x3FF - pot4) >> 1) * 2.5f) + LOOPLENGTH_MIN;
        
        // Crossfade length
        FadeLength = ((0x3FF - pot1) >> 2) + 1;
        RecordBlend = (1023 - pot1) / 1023.0f;
        // Serial.println(FadeLength);
        
        
        switch (LoopMode) {
            case 0: // Latch mode
                
                if (Retrigger && SwitchPressed && ((ms - SwitchTimer) > RETRIGGER_EXIT_TIME))
                {
                    stutter.unlatch(); LED.Flush();
                }
                
                // Stutter / Looper control
                if (btn.fallingEdge())
                {
                    SwitchPressed = true;
                    SwitchTimer = ms;
                    
                    if (!stutter.isActive()) {
                        stutter.snap();
                        LED.SetRGB(0.0f, 0.25f, 0.0f);
                        if (Retrigger) { LED.SetR(0.125f); }
                    } else {
                        stutter.unlatch(); LED.Flush();
                        
                        if (Retrigger) {
                            stutter.snap(); LED.SetRGB(0.125f, 0.25f, 0.0f);
                        }
                    }
                }
                else if (btn.risingEdge())
                {
                    SwitchPressed = false;
                    
                    if (stutter.isActive()) {
                        stutter.latch(); LED.SetRGB(0.0f, 0.0f, 0.25f);
                        if (Retrigger) { LED.SetR(0.125f); }
                    }
                }
                
                break;
                
            case 1: // Momentary mode, heads locked
            case 2: // Momentary mode, heads unlocked
            case 3: // Momentary mode, short tap loops
                
            
                if (MomentarySnapped && ((ms - SwitchTimer) > LoopLength)) {
                    MomentarySnapped = false;
                    stutter.latch(); LED.SetRGB(0.25f, 0.25f, 0.0f);
                }
                if (stutter.isLatched() && (ms - SwitchTimer) >= MOMENTARY_FREEZE_TIMER) {
                    MomentarySnapped = false;
                    if (LoopMode == 3) {// != 3
                    // {
                    //     stutter.unlatch(); LED.Flush();
                    // }
                    // else {
                        if (digitalRead(PIN_SWITCH) == LOW)
                        {
                            FreezeBlinking = false;
                            stutter.unlatch();
                            LED.Flush();
                        }
                    } else {
                        
                        if (!FreezeBlinking) // start blinking routine
                        {
                            FreezeBlinking = true;
                            FreezeBlinkTimer = ms;
                            LED.SetRGB(0.125f, 0.125f, 0.125f);
                        }
                        else {
                            int BlinkPhase = (ms - FreezeBlinkTimer) % 200;
                            if (BlinkPhase < 100) {
                                LED.SetRGB(0.125f, 0.125f, 0.125f);
                            }
                            else
                            {
                                LED.Flush();
                            }
                        }
                    }
                }
                
                if (btn.fallingEdge())
                {
                    if ((LoopMode == 1) || (LoopMode == 3)) { stutter.unlatch(); }
                    if (!MomentarySnapped) {
                        stutter.snap(); LED.SetRGB(0.25f, 0.0f, 0.25f);
                        MomentarySnapped = true;
                        SwitchTimer = ms;
                    } else {
                        MomentarySnapped = false;
                        stutter.snap(); LED.Flush();
                    }
                    
                } else if (btn.risingEdge()) {
                    if (((ms - SwitchTimer) < MOMENTARY_FREEZE_TIMER) && (LoopMode != 3))
                    {
                        MomentarySnapped = false;
                        if ((LoopMode == 1) || (LoopMode == 3)) { stutter.unlatch(); }
                        stutter.snap(); LED.Flush();
                    }
                }
                
                break;
        }
        
        
        // Adjust dry/wet blend
        if (ms - last > 50) {
            stutter.setFade(FadeLength / 256.0f);
            stutter.setBlend(RecordBlend);
            
            mixer_poststutter.gain(0, Blend <= 0 ? 1.0f : 1.0f - (abs(Blend) / 16.0f));
            mixer_poststutter.gain(1, Blend >= 0 ? 1.0f : 1.0f - (abs(Blend) / 16.0f));
            // Check pot values for crossfades, momentary/latch mode, dry/wet blend etc.
        }
    }
    
    
    
    // Monitor post EQ clipping
    if (EqActive == true)
    {
        if (peak1.available())
        {
            if (peak1.read() > 0.999)
            {
                PostEqGain -= 0.1; Level += 1;

                amp1.gain(PostEqGain);
                sgtl5000_1.lineOutLevel(Level);
                
                // Serial.println("Adapting post EQ gain:");
                // Serial.println(PostEqGain);
                // Serial.println(Level);
            }
        }
    }
    
    
    
    // Monitor clipping
    if (peak_L.available()) {
        if (peak_L.read() > 0.999) {
            ClippingTimer = millis();
            IsClipping = true;
            LED.SetRGB(0.25f, 0.0f, 0.0f);
        }
    }
    
    // Reset clipping LED
    if ((IsClipping) && (millis() - ClippingTimer > 1000)) {
        IsClipping = false; LED.SetR(0.0f); LED.Restore(false, true, true);
    }
}
