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
#include "Log.h"
#include "Ramp.h"

AudioControlSGTL5000                sgtl5000_1;         //xy=620,538
AudioInputI2S                       audioInput;         //xy=471,791
AudioAnalyzePeak                    peak_L;                 //xy=803,448
AudioAnalyzePeak                    peak_R;                 //xy=818,503
AudioFilterBiquad                   biquad1;                //xy=886,745
AudioAnalyzePeak                    peak1;                    //xy=1018,896
AudioAnalyzeRMS                     RMS_l;
AudioAnalyzeRMS                     RMS_r;
AudioAmplifier                      amp1;                     //xy=1040,831
AudioMixer4                         mixer_posteq;                 //xy=1069,668
AudioMixer4                         mixer_poststutter;                 //xy=1069,668
AudioMixer4                         mixer_poststutter_R;                 //xy=1069,668
AudioMixer4                         mixer_trem_loop_L;
AudioMixer4                         mixer_trem_loop_R;
AudioOutputI2S                      audioOutput;        //xy=1305,745

AudioEffectStutter                  stutter;
AudioEffectStutter                  stutter_R;

AudioConnection                     patchCord1(audioInput, 0, peak_L, 0);
AudioConnection                     patchCord2(audioInput, 1, peak_R, 0);

AudioConnection                     patchCord3(audioInput, 0, biquad1, 0);
AudioConnection                     patchCord4(biquad1, 0, amp1, 0);
AudioConnection                     patchCord5(amp1, 0, peak1, 0);

AudioConnection                     patchCord6(audioInput, 0, mixer_posteq, 0);
AudioConnection                     patchCord7(amp1, 0, mixer_posteq, 1);

AudioConnection                     patchCord8(mixer_posteq, 0, stutter, 0);
AudioConnection                     patchCord9(stutter, 0, mixer_poststutter, 0);
AudioConnection                     patchCord10(mixer_posteq, 0, mixer_poststutter, 1);
AudioConnection                     patchCord11(mixer_poststutter, 0, mixer_trem_loop_L, 0);

AudioConnection                     patchCord12(audioInput, 1, stutter_R, 0);
AudioConnection                     patchCord13(audioInput, 1, mixer_poststutter_R, 1);
AudioConnection                     patchCord14(stutter_R, 0, mixer_poststutter_R, 0);
AudioConnection                     patchCord15(mixer_poststutter_R, 0, mixer_trem_loop_R, 0);

AudioFilterBiquad                   split_low_L, split_low_R, split_high_L, split_high_R;
AudioMixer4                         combine_L, combine_R;

AudioConnection                     patchCord80(mixer_poststutter, 0, split_low_L, 0);
AudioConnection                     patchCord81(mixer_poststutter, 0, split_high_L, 0);

AudioConnection                     patchCord82(mixer_poststutter_R, 0, split_low_R, 0);
AudioConnection                     patchCord83(mixer_poststutter_R, 0, split_high_R, 0);

AudioConnection                     patchCord84(split_low_L, 0, combine_L, 0);
AudioConnection                     patchCord85(split_high_L, 0, combine_L, 1);

AudioConnection                     patchCord86(split_low_R, 0, combine_R, 0);
AudioConnection                     patchCord87(split_high_R, 0, combine_R, 1);

AudioConnection                     patchCord88(combine_L, 0, mixer_trem_loop_L, 1);
AudioConnection                     patchCord89(combine_R, 0, mixer_trem_loop_R, 1);

AudioConnection                     patchCord90(audioInput, 0, RMS_l, 0);
AudioConnection                     patchCord91(audioInput, 1, RMS_r, 0);

AudioConnection                     patchCord92(mixer_trem_loop_L, 0, audioOutput, 0);
AudioConnection                     patchCord93(mixer_trem_loop_R, 0, audioOutput, 1);


FilterTwoPole* rmsFilt = new FilterTwoPole;
FilterTwoPole* filtPot = new FilterTwoPole[4];


Bounce SwitchA = Bounce(PIN_SWITCH_1, SWITCH_DEB_TIME);
Bounce SwitchB = Bounce(PIN_SWITCH_2, SWITCH_DEB_TIME);
Bounce Toggle  = Bounce(PIN_TOGGLE, SWITCH_DEB_TIME);

CtrlLED LED = CtrlLED();

float PhaseMultiplier = 1.0f;
float Value = 0.0f;
CtrlLED LED = CtrlLED();

bool IsClipping = false;
unsigned long ClippingTimer = 0;

bool SwitchPressed = false;
unsigned long SwitchTimer = 0;

unsigned long ms = 0;

int PedalMode = 0; // 0 ... Stutter, 1 ... EQ setup
int LoopMode = 0; // 0 ... Latch, 1 ... Momentary, 2 ... Chaotic?


bool State = true, MomentarySnapped = false, Retrigger = false;
int Level = 13;
float PostEqGain = 1.0f, RecordBlend = 1.0f;
float FadeInLength = 1.0f, FadeOutLength = 1.0f;
float Attack = 1.0f, Decay = 1.0f;
float peakl, peakr, rmsfact = 1.0f, lll, rrr;

byte Freq1, Freq2;
byte Gain1, Gain2;
bool EqActive, TremMode = 0, TremActive = 0;

int Blend, LoopLength, FadeLength;

float Depth, Speed, Sense, SplitFreq, StutterVol;

bool IsRamping = false, SwitchBPressed = false;

Ramp* Ramper = new Ramp(0.0f, 1.0f);
Ramp* StutterRamper = new Ramp(0.0f, 10.0f);

void setup()
{
    for (int i = 0; i < 4; i++) {
        filtPot[i].setAsFilter(LOWPASS_BUTTERWORTH, ANALOG_FILTER_FREQ);
    }
    rmsFilt->setAsFilter(LOWPASS_BUTTERWORTH, 0.95f);
    
    AudioMemory(14340);
    sgtl5000_1.enable();
    sgtl5000_1.inputSelect(AUDIO_INPUT_LINEIN);
    sgtl5000_1.volume(0.0);
    sgtl5000_1.micGain(0);
    sgtl5000_1.lineInLevel(0);
    sgtl5000_1.lineOutLevel(Level);
    
    pinMode(PIN_TOGGLE, INPUT_PULLUP);
    pinMode(PIN_SWITCH_1, INPUT_PULLUP);
    pinMode(PIN_SWITCH_2, INPUT_PULLUP);
    
    pinMode(PIN_LED_R, OUTPUT);
    pinMode(PIN_LED_G, OUTPUT);
    pinMode(PIN_LED_B, OUTPUT);
    
    analogWriteFrequency(PIN_LED_R, 50000);
    analogWriteFrequency(PIN_LED_G, 50000);
    analogWriteFrequency(PIN_LED_B, 50000);
    
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
    
    
    if (digitalRead(PIN_SWITCH_1) == LOW) { PedalMode = 1; }
    
    if (digitalRead(PIN_TOGGLE) == LOW) {
        TremMode = 1;
        
        mixer_trem_loop_L.gain(0, 0.0f);
        mixer_trem_loop_L.gain(1, 1.25f);
        
        mixer_trem_loop_R.gain(0, 0.0f);
        mixer_trem_loop_R.gain(1, 1.25f);
    }
    else {
        TremMode = 0;
        
        mixer_trem_loop_L.gain(0, 1.0f);
        mixer_trem_loop_L.gain(1, 0.0f);
        
        mixer_trem_loop_R.gain(0, 1.0f);
        mixer_trem_loop_R.gain(1, 0.0f);
    }
    
    amp1.gain(PostEqGain);
    
    // Bypass EQ
    if (EqActive == 1) {
        mixer_posteq.gain(0, 0.0f);
        mixer_posteq.gain(1, 1.0f);
    }
    else {        
        mixer_posteq.gain(0, 1.0f);
        mixer_posteq.gain(1, 0.0f);
    }
    
    mixer_poststutter.gain(0, 1.0f);
    mixer_poststutter.gain(1, 1.0f);
    
    mixer_poststutter_R.gain(0, 1.0f);
    mixer_poststutter_R.gain(1, 1.0f);
    
    // Harmonic tremolo
    split_low_L.setLowpass(0, 820.0f, 1.0f);
    split_high_L.setHighpass(0, 780.0f, 1.0f);
    
    split_low_R.setLowpass(0, 820.0f, 1.0f);
    split_high_R.setHighpass(0, 780.0f, 1.0f);
    //
    
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
    
    #if DEBUG
    Serial.begin(9600);
    #endif
    
    sgtl5000_1.adcHighPassFilterDisable();
    
    SwitchA.update();
    SwitchB.update();
    Toggle.update();
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
        
        vPotNorm[i] = filtPot[i].output();
        
        // Range check and border snap
        if (vPotNorm[i] < ANALOG_SNAP_THRESHOLD) { vPotNorm[i] = 0.0f; }
        if (vPotNorm[i] > (1.0f - ANALOG_SNAP_THRESHOLD)) { vPotNorm[i] = 1.0f; }
        
        vPot[i] = vPotNorm[i] * ANALOG_GAIN_RECPR - ANALOG_OFFSET;
        
        #ifdef DEBUG_POT_VALUES
            DPRINT(vPotNorm[i]); DPRINT(" - ");
        #endif
    }
    
    #ifdef DEBUG_POT_VALUES
        int test = digitalRead(PIN_TOGGLE);
        DPRINT(test); DPRINT(" - ");
        
        test = digitalRead(PIN_SWITCH_1);
        DPRINT(test); DPRINT(" - ");
        
        test = digitalRead(PIN_SWITCH_2);
        DPRINT(test); DPRINT(" - ");
        
        DPRINTLN(".");
    #endif
    
    
    
    // Update routine
    SwitchA.update();
    SwitchB.update();
    Toggle.update();
    
    
    
    if (Toggle.risingEdge())
    {
        TremMode = 0;
        TremActive = 0;
        
        Ramper->Start(0.0f);
        
        LED.Flush();
    }
    else if (Toggle.fallingEdge())
    {
        Ramper->Start(1.0f);
        
        TremActive = 1;
        TremMode = 1;
    }
    
    
    
    if (PedalMode == 1)
    {
        // Toggle EQ
        if (SwitchA.risingEdge())
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
        
        // Update EQ
        if (millis() - last > 50) {
            biquad1.setPeaking(0, EQ_PEAK1_FREQ - vPot[0] * EQ_PEAK1_FREQ_SCALE, -vPot[2] * EQ_PEAK1_GAINSCALE, EQ_PEAK1_Q);
            biquad1.setHighShelf(1, EQ_PEAK2_FREQ - vPot[1] * EQ_PEAK2_FREQ_SCALE, -vPot[3] * EQ_PEAK1_GAINSCALE - 5.0f, EQ_PEAK2_Q);
            last = millis();
        }
        
        
        // Exit EQ setup mode
        if (digitalRead(PIN_SWITCH_1) == LOW) {
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
        
        if (!TremMode)
        {
            Attack = (1 - vPotNorm[POT_ATTACK]);
            if (Attack < 0.05f) { Attack = 0.05f; }
            Decay = (1 - vPotNorm[POT_DECAY]);
            if (Decay > 0.99f) { Decay = 1.0f; }
            
            Blend = 1.0f - vPot[POT_MIXER];
            RecordBlend = vPotNorm[POT_BLEND];
            
            if (SwitchB.fallingEdge())
            {                
                stutter.setDecay(1.0f);
                stutter_R.setDecay(1.0f);
            
                SwitchBPressed = true;
                stutter.drop(); stutter_R.drop(); LED.Flush();
                stutter.snap(); stutter_R.snap(); LED.SetRGB(0.1f, 0.3f, 0.1f);
                
                StutterRamper->Set(StutterRamper->Poll());
                StutterRamper->Start(1.0f, 0.0f, 1);
                StutterRamper->SetSpeed(1.0f);
            }
            else if (SwitchB.risingEdge())
            {
                SwitchBPressed = false;
                StutterRamper->Start(1.0f, 0.0f, 1);
                StutterRamper->Start(0.0f, 1.0f, -1);
                StutterRamper->SetSpeed(Decay);
            }
            
        }
        else
        {
            Depth = vPotNorm[POT_MIXER];
            Sense = vPotNorm[POT_BLEND];
            Speed = 1.0f - vPotNorm[POT_ATTACK];
            SplitFreq = vPotNorm[POT_DECAY];
        }
        
        if (!SwitchBPressed)
        {
            if (SwitchPressed && ((ms - SwitchTimer) > RETRIGGER_EXIT_TIME))
            {
                stutter.drop(); stutter_R.drop(); LED.Flush();
                SwitchPressed = false;
            }
            
            if (stutter.isLatched()) {
                LED.SetRGB(0.0f, 0.0f, 0.1f);
            }
            // Stutter / Looper control
            if (SwitchA.fallingEdge())
            {
                SwitchPressed = true;
                SwitchTimer = ms;
                
                stutter.drop(); stutter_R.drop();
                stutter.snap(); stutter_R.snap(); LED.SetRGB(0.0f, 0.1f, 0.0f);
                
                StutterRamper->Set(StutterRamper->Poll());
                StutterRamper->Start(1.0f, 0.0f, 1);
            }
            else if (SwitchA.risingEdge())
            {
                SwitchPressed = false;
                
                stutter.latch(); stutter_R.latch(); LED.SetRGB(0.0f, 0.0f, 0.1f);
                
            }
            
            
            if (StutterRamper->GetDirection() == 1) {
                StutterRamper->SetSpeed(Attack * 2.0f + 0.01f);
            } else {
                StutterRamper->SetSpeed(Decay);
            }
        }
        else
        {
            if ((StutterVol >= 1.0f) && (StutterRamper->GetDirection() == 1) & !stutter.isLatched()) {
                stutter.latch(); stutter_R.latch(); LED.SetRGB(0.0f, 0.0f, 0.1f);
            }
        }
        
        
        // if (stutter.isFinished())   { stutter.resetFinished(); LED.Flush(); }
        // if (stutter_R.isFinished()) { stutter_R.resetFinished(); LED.Flush(); }
        
        StutterRamper->Update();
        
        
        
        StutterVol = StutterRamper->Poll();
        DPRINT(StutterVol);
        DPRINT("; ");
        DPRINTLN(StutterRamper->GetDirection());
        
        if (!SwitchBPressed)
        {
            if ((StutterVol <= 0.0f) && (StutterRamper->GetDirection() == -1)) {
                stutter.drop(); stutter_R.drop(); LED.Flush();
            }
            if ((StutterVol >= 1.0f) && (StutterRamper->GetDirection() == 1) && stutter.isLatched()) {
                StutterRamper->Start(0.0f, 1.0f, -1);
                StutterRamper->SetSpeed(Decay);
                LED.SetRGB(0.0f, 0.0f, 0.1f);
            }
        }
        
        // Adjust dry/wet blend
        if (ms - last > 20) {
            IsRamping = Ramper->Update();
            Value = Ramper->Poll();
            
            if (IsRamping)
            {
                mixer_trem_loop_L.gain(0, 1.0f - Value);
                mixer_trem_loop_L.gain(1, Value * 1.25f);
                
                mixer_trem_loop_R.gain(0, 1.0f - Value);
                mixer_trem_loop_R.gain(1, Value * 1.25f);
            }
            
            
            // Update stutter blend etc.
            stutter.setBlend(RecordBlend); stutter_R.setBlend(RecordBlend);
            
            mixer_poststutter.gain(0, Blend <= 0 ? 1.0f * StutterVol : StutterVol * (1.0f - (abs(Blend) / 16.0f)));
            mixer_poststutter_R.gain(0, Blend <= 0 ? 1.0f * StutterVol : StutterVol * (1.0f - (abs(Blend) / 16.0f)));
            
            if (!stutter.isLatched() || !stutter.isActive())
            {
                mixer_poststutter.gain(1, 1.0f);
                mixer_poststutter_R.gain(1, 1.0f);
            }
            else
            {
                
                float gainCalc = (1.0f - StutterVol) + (1.0f - (abs(Blend) / 16.0f));
                
                mixer_poststutter.gain(1, Blend >= 0 ? 1.0f : (gainCalc > 1.0f ? 1.0f : gainCalc));
                mixer_poststutter_R.gain(1, Blend >= 0 ? 1.0f : (gainCalc > 1.0f ? 1.0f : gainCalc));
            }
            
            
            last = millis();
            // Check pot values for crossfades, momentary/latch mode, dry/wet blend etc.
        }
    }
    
    
    
    
    
    if (TremMode)
    {
        split_low_L.setLowpass(0, TREM_FREQ_OFFSET + TREM_FREQ_GAIN * SplitFreq + TREM_FREQ_GAP, 1.0f);
        split_high_L.setHighpass(0, TREM_FREQ_OFFSET + TREM_FREQ_GAIN * SplitFreq - TREM_FREQ_GAP, 1.0f);
        
        split_low_R.setLowpass(0, TREM_FREQ_OFFSET + TREM_FREQ_GAIN * SplitFreq + TREM_FREQ_GAP, 1.0f);
        split_high_R.setHighpass(0, TREM_FREQ_OFFSET + TREM_FREQ_GAIN * SplitFreq - TREM_FREQ_GAP, 1.0f);
        
        
        if (RMS_l.available()) {
            lll = RMS_l.read();
            rrr = RMS_r.read();
            rmsFilt->input(0.9f + ((lll / 18.0f + rrr / 18.0f) * (Sense / 2.0f)));
            rmsfact = rmsFilt->output() / (0.5f + Speed * 16.0f);
        }
        
        if (SwitchB.risingEdge())
        {
            if (TremActive) {
                TremActive = 0;
                
                Ramper->Start(0.0f);
                
                LED.Flush();
            } else {
                TremActive = 1;
                
                Ramper->Start(1.0f);
            }
        }
        
        if (TremActive)
        {
            if (SplitFreq > 0.999f) {
                PhaseMultiplier = 0.0f;
            } else {
                PhaseMultiplier = 1.0f;
            }
            
            combine_L.gain(0, Depth * (sin(ms / 45.0f * rmsfact) + 1.0f) / (Depth + 0.5f) + (1 - Depth));
            combine_L.gain(1, Depth * (sin(ms / 45.0f * rmsfact + PI * PhaseMultiplier) + 1.0f) / (Depth + 0.5f) + (1 - Depth));
            
            combine_R.gain(0, Depth * (sin(ms / 45.0f * rmsfact) + 1.0f) / (Depth + 0.5f) + (1 - Depth));
            combine_R.gain(1, Depth * (sin(ms / 45.0f * rmsfact + PI * PhaseMultiplier) + 1.0f) / (Depth + 0.5f) + (1 - Depth));
            
            float led_wobble = 0.5f * (sin(ms / 45.0f * rmsfact) + 1.0f) / 1.5f + 0.25f;
            float led_wobble_pi = 0.5f * (sin(ms / 45.0f * rmsfact + PI * PhaseMultiplier) + 1.0f) / 1.5f + 0.25f;
            
            if (!IsClipping) { LED.SetRGB(1.0f * led_wobble * (lll + rrr) * 0.5f, 0.2f * led_wobble * lll, 1.4f * led_wobble * rrr); }
        }
        else
        {
            combine_L.gain(0, 1.0f);
            combine_L.gain(1, 1.0f);
            
            combine_R.gain(0, 1.0f);
            combine_R.gain(1, 1.0f);
        }
        
    }
    
    // Monitor input clipping
    if (peak_L.available()) {
        peakl = peak_L.read();
        if (peakl > 0.99999f) {
            DPRINTLN("Peak L");
            ClippingTimer = millis();
            IsClipping = true;
            LED.SetRGB(0.25f, 0.0f, 0.0f);
        }
    }// Monitor input clipping
    if (peak_R.available()) {
        peakr = peak_R.read();
        if (peakr > 0.99999f) {
            DPRINTLN("Peak R");
            ClippingTimer = millis();
            IsClipping = true;
            LED.SetRGB(0.25f, 0.0f, 0.0f);
        }
    }
    
    
    // Reset clipping LED
    if ((IsClipping) && (millis() - ClippingTimer > 1000)) {
        IsClipping = false; LED.SetR(0.0f); LED.Restore(false, true, true);
    }
    
    
    // Monitor post EQ clipping
    // if (EqActive == true)
    // {
    //     if (peak1.available())
    //     {
    //         if (peak1.read() > 0.999)
    //         {
    //             PostEqGain -= 0.1; Level = Level > 0 ? Level -= 1 : 0;
    // 
    //             amp1.gain(PostEqGain);
    //             sgtl5000_1.lineOutLevel(Level);
    // 
    //             DPRINTLN("Adapting post EQ gain:");
    //             DPRINTLN(PostEqGain);
    //             DPRINTLN(Level);
    //         }
    //     }
    // }
}
