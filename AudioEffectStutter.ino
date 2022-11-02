#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <Bounce.h>
#include <EEPROM.h>
#include <Filters.h>
#include <vector>

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
AudioConnection                     patchCord5(audioInput, 1, peak_R, 0);
AudioConnection                     patchCord6(audioInput, 1, audioOutput, 1);
AudioConnection                     patchCord7(biquad1, 0, amp1, 0);
AudioConnection                     patchCord8(amp1, 0, peak1, 0);
AudioConnection                     patchCord9(amp1, 0, mixer_posteq, 1);
AudioConnection                     patchCord10(mixer_posteq, 0, stutter, 0);
AudioConnection                     patchCord10a(stutter, 0, mixer_poststutter, 0);
AudioConnection                     patchCord10b(mixer_posteq, 0, mixer_poststutter, 1);
AudioConnection                     patchCord11(mixer_poststutter, 0, audioOutput, 0);

AudioControlSGTL5000                sgtl5000_1;         //xy=620,538


FilterTwoPole* filtPot = new FilterTwoPole[4];

Bounce btn = Bounce(PIN_SWITCH, SWITCH_DEB_TIME);

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


bool State = true, MomentarySnapped = false, Retrigger = false;
int Level = 16;
float PostEqGain = 1.0f, RecordBlend = 1.0f;
float FadeInLength = 1.0f, FadeOutLength = 1.0f;
float Attack, Decay;

byte Freq1, Freq2;
byte Gain1, Gain2;
bool EqActive;

int Blend, LoopLength, FadeLength;


void setup()
{
    for (int i = 0; i < 4; i++) {
        filtPot[i].setAsFilter(LOWPASS_BUTTERWORTH, ANALOG_FILTER_FREQ);
    }
    
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
    
    // Init routine
    for (int i = 0; i < 8; i++) {
        LED.SetRGB(0.25f, 0.0f, 0.25f); delay(LED_BLINKTIME);
        LED.Flush(); delay(LED_BLINKTIME);
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
    
    biquad1.setPeaking(0, EQ_PEAK1_FREQ - Freq1 * EQ_PEAK1_FREQ_SCALE, -Gain1 * EQ_PEAK1_GAINSCALE, EQ_PEAK1_Q);
    biquad1.setHighShelf(1, EQ_PEAK2_FREQ - Freq2 * EQ_PEAK2_FREQ_SCALE, -Gain2 * EQ_PEAK2_GAINSCALE - 5.0f, EQ_PEAK2_Q);
    
    if (PedalMode == 1)
    {
        for (int i = 0; i < 2; i++) {
            LED.SetRGB(0.0f, 0.25f, 0.25f); delay(LED_BLINKTIME);
            LED.Flush(); delay(LED_BLINKTIME);
        }
        
        if (EqActive == 0)  { LED.SetRGB(0.125f, 0.05f, 0.0f);  }
        else                { LED.SetRGB(0.0f, 0.125f, 0.125f); }
    }
    
    
    Serial.begin(9600);
}



int last = 0;
float vPot[4], vPotNorm[4];

void loop() {
    // Read pots and filter values
    for (int i = 0; i < 4; i++) {
        float reading = analogRead(PIN_POT[i]) * ANALOG_RESCALE + ANALOG_SHIFT;
        
        if (reading < ANALOG_SNAP_THRESHOLD * 2) { reading = 0.0f; }
        if (reading > 1.0f - ANALOG_SNAP_THRESHOLD * 2) { reading = 1.0f; }
        
        filtPot[i].input(reading);
    }
    
    for (int i = 0; i < 4; i++) {
        vPotNorm[i] = filtPot[i].output();
        
        // Range check and border snap
        if (vPotNorm[i] < ANALOG_SNAP_THRESHOLD) { vPotNorm[i] = 0.0f; }
        if (vPotNorm[i] > (1.0f - ANALOG_SNAP_THRESHOLD)) { vPotNorm[i] = 1.0f; }
        
        vPot[i] = vPotNorm[i] * ANALOG_GAIN_RECPR - ANALOG_OFFSET;
    }
    
    
    // Update routine    
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
                LED.SetRGB(0.125f, 0.00f, 0.01f);
            }
            else
            {
                mixer_posteq.gain(0, 1.0f);
                mixer_posteq.gain(1, 0.0f);
                
                EqActive = 0;
                LED.SetRGB(0.0f, 0.125f, 0.125f);
            }
        }
        
        // Serial.println(vPot[3]);
        
        // Update EQ
        if (millis() - last > 50) {
            biquad1.setPeaking(0, EQ_PEAK1_FREQ - vPot[0] * EQ_PEAK1_FREQ_SCALE, -vPot[2] * EQ_PEAK1_GAINSCALE, EQ_PEAK1_Q);
            biquad1.setHighShelf(1, EQ_PEAK2_FREQ - vPot[1] * EQ_PEAK2_FREQ_SCALE, -vPot[3] * EQ_PEAK1_GAINSCALE - 5.0f, EQ_PEAK2_Q);
            last = millis();
        }
        
        
        // Exit EQ setup mode
        if (digitalRead(PIN_SWITCH) == LOW) {
            if (SwitchPressed) {
                if ((millis() - SwitchTimer) > EQSAVE_TIMER) {
                    PedalMode = 0;
                                        
                    EEPROM.write(EEPROM_FREQ1, vPot[0] * EEPROM_FREQ1_SCALE + EEPROM_FREQ1_OFFSET);
                    EEPROM.write(EEPROM_FREQ2, vPot[1] * EEPROM_FREQ2_SCALE + EEPROM_FREQ2_OFFSET);
                    EEPROM.write(EEPROM_GAIN1, vPot[2] * EEPROM_GAIN1_SCALE + EEPROM_GAIN1_OFFSET);
                    EEPROM.write(EEPROM_GAIN2, vPot[3] * EEPROM_GAIN2_SCALE + EEPROM_GAIN2_OFFSET);
                    EEPROM.write(EEPROM_EQSTATE, EqActive);
                    
                    LED.SetG(0.0f); 
                    for (int i = 0; i < 4; i++) {
                        LED.SetRGB(0.25f, 0.0f, 0.125f); delay(LED_BLINKTIME);
                        LED.Flush(); delay(LED_BLINKTIME);
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
        
                
        FadeOutLength = (1.0f - vPotNorm[0]) * 1280 + LOOPLENGTH_MIN;
        FadeInLength = (1.0f - vPotNorm[1]) * 1280 + LOOPLENGTH_MIN;
        
        
        Attack = (1.0f - vPotNorm[1]);
        if (Attack < 0.1f) { Attack = 0.1f; }
        Decay = (1.0f - vPotNorm[0]);
        if (Decay > 0.99f) { Decay = 1.0f; }
        
        Blend = vPot[2];        
        RecordBlend = 1.0f - vPotNorm[3];
        
        
        stutter.setAttack(Attack);
        stutter.setDecay(Decay);
                
        if (SwitchPressed && ((ms - SwitchTimer) > RETRIGGER_EXIT_TIME))
        {
            stutter.drop(); LED.Flush();
            SwitchPressed = false;
        }
        
        // if (SwitchPressed && ((ms - SwitchTimer) > LoopLength)) {
        //     MomentarySnapped = false;
        //     stutter.latch(); LED.SetRGB(0.25f, 0.25f, 0.0f);
        // }
        // 
        // 
        
        if (stutter.isLatched()) {
            LED.SetRGB(0.0f, 0.0f, 0.25f);
        }
        
        
        
        // Stutter / Looper control
        if (btn.fallingEdge())
        {
            SwitchPressed = true;
            SwitchTimer = ms;
            
            if (!stutter.isActive()) {
                stutter.snap();
                LED.SetRGB(0.0f, 0.1f, 0.0f);
                
                // if (Retrigger) { LED.SetR(0.125f); }
            } else {
                // if (stutter.isLatched()) { 
                //     stutter.dub();
                //     LED.SetRGB(0.125f, 0.25f, 0.25f);
                // }
                // else {
                stutter.drop(); LED.Flush();
                stutter.snap(); LED.SetRGB(0.0f, 0.1f, 0.0f);
                // }
                // else
                // {
                //     stutter.unlatch();
                // }
                    
                    // if (Retrigger) {
                    //     stutter.snap();
                    //     LED.SetRGB(0.125f, 0.25f, 0.0f);
                    // }
                // }
            }
        }
        else if (btn.risingEdge())
        {
            SwitchPressed = false;
            
            // if (stutter.isActive()) {
            stutter.unlatch(); LED.Flush();
                
                // LED.SetRGB(0.0f, 0.0f, 0.25f);
                // if (Retrigger) { LED.SetR(0.125f); }
            // }
        }
        
        if (!stutter.isActive()) {
            LED.Flush();
        }
        
        
        // Adjust dry/wet blend
        if (ms - last > 20) {
            // stutter.setFade(FadeLength / 256.0f);
            stutter.setBlend(RecordBlend);
            
            mixer_poststutter.gain(0, Blend <= 0 ? 1.0f * stutter.getGain() : stutter.getGain() * (1.0f - (abs(Blend) / 16.0f)));
            
            if (!stutter.isLatched() || !stutter.isActive())
            {
                mixer_poststutter.gain(1, 1.0f);
            }
            else
            {
                
                float gainCalc = (1.0f - stutter.getGain()) + (1.0f - (abs(Blend) / 16.0f));
                
                // Serial.print(gainCalc);
                // Serial.print("    ");
                // Serial.print(Blend);
                // Serial.print("    ");
                // Serial.print(stutter.getGain());
                // Serial.println("");
                //
                
                mixer_poststutter.gain(1, Blend >= 0 ? 1.0f : (gainCalc > 1.0f ? 1.0f : gainCalc));
            }
                
            
            last = millis();
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
    
                Serial.println("Adapting post EQ gain:");
                Serial.println(PostEqGain);
                Serial.println(Level);
            }
        }
    }
    
    
    
    // Monitor input clipping
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
