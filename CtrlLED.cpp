#include "Arduino.h"
#include "CtrlLED.h"
#include "AudioEffectStutter.h"


CtrlLED::CtrlLED() {
    this->Flush();
    red = 0.0f; green = 0.0f; blue = 0.0f;
}


void CtrlLED::Flash() {
    analogWrite(PIN_LED_R, Ducy(1.0f));
    analogWrite(PIN_LED_G, Ducy(1.0f));
    analogWrite(PIN_LED_B, Ducy(1.0f));
    
    red = 1.0f; green = 1.0f; blue = 1.0f;
    
    return;
}

void CtrlLED::Flush() {
    analogWrite(PIN_LED_R, Ducy(0.0f));
    analogWrite(PIN_LED_G, Ducy(0.0f));
    analogWrite(PIN_LED_B, Ducy(0.0f));
    
    red = 0.0f; green = 0.0f; blue = 0.0f;
    
    return;
}


void CtrlLED::SetRGB(float Red, float Green, float Blue)
{
    analogWrite(PIN_LED_R, Ducy(Red));
    analogWrite(PIN_LED_G, Ducy(Green));
    analogWrite(PIN_LED_B, Ducy(Blue));
    
    red = Red; green = Green; blue = Blue;
}

void CtrlLED::SetRGB(unsigned int RGB)
{
    red = Red(RGB);
    green = Green(RGB);
    blue = Blue(RGB);
    
    analogWrite(PIN_LED_R, Ducy(red / 255.0f));
    analogWrite(PIN_LED_G, Ducy(green / 255.0f));
    analogWrite(PIN_LED_B, Ducy(blue / 255.0f));
}

void CtrlLED::SetR(float r)
{
    analogWrite(PIN_LED_R, Ducy(r));
    red = r;
}
void CtrlLED::SetG(float g)
{
    analogWrite(PIN_LED_G, Ducy(g));
    green = g;
}
void CtrlLED::SetB(float b)
{
    analogWrite(PIN_LED_B, Ducy(b));
    blue = b;
}

void CtrlLED::Restore(bool r, bool g, bool b)
{
    if (r) { this->SetR(red); }
    if (g) { this->SetG(green); }
    if (b) { this->SetB(blue); }
}


byte Red(unsigned int Hex) { return (Hex & 0xFF0000) >> 16; }
byte Green(unsigned int Hex) { return (Hex & 0x00FF00) >> 8; }
byte Blue(unsigned int Hex) { return (Hex & 0x0000FF); }

unsigned int HexRGB(byte R, byte G, byte B) { return ((R << 16) + (G << 8) + B); }