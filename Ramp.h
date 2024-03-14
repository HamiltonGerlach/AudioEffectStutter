#ifndef RAMP_H
#define RAMP_H

#include "Arduino.h"
#include "Timer.h"

class Ramp {
    public:
        void Reset();
        bool Update();
        void Start(float Target);
        void Start(float Target, float Init, int Direction);
        float Poll();
        void Set(float Value);
        void SetSpeed(float Speed);
        int GetDirection();
        Ramp(float Value, float Speed);
    protected:
        bool State;
        int Direction;
        float Speed;
        float Value;
        float SaveValue;
        float Target;
        float SaveTarget;
        
        Timer timer;
        
        float Timeout = 10000;
};


#endif
