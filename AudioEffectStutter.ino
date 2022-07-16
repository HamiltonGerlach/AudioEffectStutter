#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <Bounce.h>
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
AudioConnection                     patchCord3a(audioInput, 0, stutter, 0);
AudioConnection                     patchCord4(audioInput, 0, mixer1, 0);
AudioConnection                     patchCord5(audioInput, 1, peak_R, 0);
AudioConnection                     patchCord6(audioInput, 1, audioOutput, 1);
AudioConnection                     patchCord7(biquad1, amp1);
AudioConnection                     patchCord8(amp1, peak1);
AudioConnection                     patchCord9(amp1, 0, mixer1, 1);
AudioConnection                     patchCord10(stutter, 0, mixer1, 2);
AudioConnection                     patchCord11(mixer1, 0, audioOutput, 0);
AudioControlSGTL5000                sgtl5000_1;         //xy=620,538


bool IsClipping = false;
unsigned long ClippingTimer = 0;

int PedalMode = 0; // 0 ... EQ, 1 ... Stutter

Bounce btn = Bounce(14, 40);

bool State = true;
int Level = 16;
float Gain = 1.0;

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
    
    pinMode(14, INPUT_PULLUP);
    pinMode(15, INPUT);
    pinMode(16, INPUT);
    pinMode(17, INPUT);
    pinMode(18, INPUT);
    
    delay(2000);
    if (digitalRead(14) == LOW) { PedalMode = 1; }
    Serial.println(PedalMode);
    
    amp1.gain(Gain);
    
    if (PedalMode == 0)
    {
        mixer1.gain(0, 0.0f);
        mixer1.gain(1, 1.0f);
        mixer1.gain(2, 0.0f);
    }
    else
    {
        mixer1.gain(0, 0.0f);
        mixer1.gain(1, 0.0f);
        mixer1.gain(2, 1.0f);
    }
}

int last = 0;
void loop() {
    if (peak_L.available()) {
        if (peak_L.read() > 0.999) {
            ClippingTimer = millis();
            IsClipping = true;
            analogWrite(1, 1024);
            analogWrite(3, 1024);
            analogWrite(2, 0);
        }
    }
    
    if (millis() - ClippingTimer > 1000) {
        IsClipping = false;
        analogWrite(2, 1024);
    }
    
    
    int pot1 = analogRead(15);
    int pot1conv = round(pot1 / 32 - 15.5);
    int pot2 = analogRead(16);
    int pot2conv = round(pot2 / 32 - 15.5);
    int pot3 = analogRead(17);
    int pot3conv = round(pot3 / 32 - 15.5);
    int pot4 = analogRead(18);
    int pot4conv = round(pot4 / 32 - 15.5);


    btn.update();

    
    
    if (PedalMode == 0)
    {
        if (btn.fallingEdge())
        {
            State = !State;
            Serial.println(State);
        
            if (State == true)
            {
                mixer1.gain(1, 0.0f);
                mixer1.gain(0, 1.0f);
            }
            else
            {
                mixer1.gain(0, 0.0f);
                mixer1.gain(1, 1.0f);
            }
        }
        
        if (State != true)
        {
            if (peak1.available())
            {
                float PeakPostEQ = peak1.read();
                
                Serial.println(PeakPostEQ);
                
                if (PeakPostEQ > 0.999)
                {
                    Gain -= 0.1;
                    Level += 1;
    
                    amp1.gain(Gain);
                    sgtl5000_1.lineOutLevel(Level);
    
                    Serial.println(Gain);
                    Serial.println(Level);
                }
            }
        }
        
        
        if (millis() - last > 50) {
            biquad1.setPeaking(0, 6000 - pot1conv * 350, -pot3conv, 2);
            biquad1.setPeaking(1, 4000 - pot2conv * 150, -pot4conv, 6);
            last = millis();
        }
    }
    else
    {
        if (btn.fallingEdge())
        {
            Serial.println("Button press");
            if (!stutter.isActive()) { stutter.snap(); } else { stutter.unlatch(); }
        }
        else if (btn.risingEdge()) {
            Serial.println("Button release");
            if (stutter.isActive()) { stutter.latch(); }
        }
    }
    
    
}
