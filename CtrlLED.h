#ifndef CTRL_LED_H
#define CTRL_LED_H

enum PinLED { R, G, B };
struct fRGB { float R, G, B; };

class CtrlLED {
    public:
        void Flash();
        void Flush();
        
        void SetRGB(float Red, float Green, float Blue);
        void SetRGB(unsigned int RGB);
        
        void SetR(float r);
        void SetG(float g);
        void SetB(float b);
        
        CtrlLED();
    protected:
        bool State;
};


byte Red(unsigned int Hex);
byte Green(unsigned int Hex);
byte Blue(unsigned int Hex);

unsigned int HexRGB(byte R, byte G, byte B);

inline int Ducy(float Percent) { return 256 - (int)floor(Percent * 256); }

#endif
