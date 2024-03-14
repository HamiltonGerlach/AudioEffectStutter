#include "Arduino.h"
#include "Ramp.h"
#include "Timer.h"

Ramp::Ramp(float Value, float Speed) {
    this->Value = Value;
    this->Speed = Speed;
    this->SaveValue = Value;
    
    this->State = 0;
}


void Ramp::Reset() {
    this->Value = this->SaveValue;
    this->State = 0;
}

bool Ramp::Update()
{
    if (!this->State) return false;
    if (this->timer.Check(this->Timeout))
    {
        this->Value = this->Target;
        this->State = 0;
    }
    else
    {
        this->Value = this->Value + ((this->Target - this->SaveValue) * (this->timer.Delta()) * (Speed / 1000.0f));
        this->timer.Reset();
        
        if ( ((this->Direction == -1) && (this->Value <= this->Target)) || ((this->Direction == 1) && (this->Value >= this->Target)) )
        {
            this->Value = this->Target;
            this->State = 0;
        }
    }
    
    return this->State;
}


float Ramp::Poll()
{
    return this->Value;
}


int Ramp::GetDirection()
{
    return this->Direction;
}



void Ramp::Start(float Target)
{
    this->Target = Target;
    this->Direction = (this->Target < this->Value) ? -1 : 1;
    this->State = 1;
    this->SaveValue = this->Value;
    this->timer.Reset();
}


void Ramp::Start(float Target, float Init, int Direction)
{
    this->Target = Target;
    this->Direction = Direction;
    this->State = 1;
    this->SaveValue = Init;
    this->timer.Reset();
}


void Ramp::Set(float Value)
{
    this->Value = Value;
    this->State = 0;
}

void Ramp::SetSpeed(float Speed)
{
    this->Speed = Speed;
}