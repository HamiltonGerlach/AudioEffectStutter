#include "Arduino.h"
#include "CtrlLED.h"
#include "AudioEffectStutter.h"


CtrlLED::CtrlLED() {
    this->Flush();
}


void CtrlLED::Flash() {
    analogWrite(PIN_LED_R, Ducy(1.0f));
    analogWrite(PIN_LED_G, Ducy(1.0f));
    analogWrite(PIN_LED_B, Ducy(1.0f));
    
    return;
}

void CtrlLED::Flush() {
    analogWrite(PIN_LED_R, Ducy(0.0f));
    analogWrite(PIN_LED_G, Ducy(0.0f));
    analogWrite(PIN_LED_B, Ducy(0.0f));
    
    return;
}


void CtrlLED::SetRGB(float Red, float Green, float Blue)
{
    analogWrite(PIN_LED_R, Ducy(Red));
    analogWrite(PIN_LED_G, Ducy(Green));
    analogWrite(PIN_LED_B, Ducy(Blue));
}

void CtrlLED::SetRGB(unsigned int RGB)
{
    float r, g, b;
    
    r = Red(RGB);
    g = Green(RGB);
    b = Blue(RGB);
    
    analogWrite(PIN_LED_R, Ducy(r / 255.0f));
    analogWrite(PIN_LED_G, Ducy(g / 255.0f));
    analogWrite(PIN_LED_B, Ducy(b / 255.0f));
}

void CtrlLED::SetR(float r)
{
    analogWrite(PIN_LED_R, Ducy(r));
}
void CtrlLED::SetG(float g)
{
    analogWrite(PIN_LED_G, Ducy(g));
}
void CtrlLED::SetB(float b)
{
    analogWrite(PIN_LED_B, Ducy(b));
}



byte Red(unsigned int Hex) { return (Hex & 0xFF0000) >> 16; }
byte Green(unsigned int Hex) { return (Hex & 0x00FF00) >> 8; }
byte Blue(unsigned int Hex) { return (Hex & 0x0000FF); }

unsigned int HexRGB(byte R, byte G, byte B) { return ((R << 16) + (G << 8) + B); }