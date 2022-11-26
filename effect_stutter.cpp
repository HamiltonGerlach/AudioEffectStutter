#include <Arduino.h>
#include "effect_stutter.h"
#include "Log.h"

void AudioEffectStutter::update(void)
{
    audio_block_t *block, *fadeBlock;
    int16_t *pa, *cache, *pma;
    int16_t sample, s_cache;
    int index, dist, mirror;
    float fadeA, fadeB;
    
    block = receiveWritable(0);
    if (!block) { return; }
    
    switch (state)
    {
        case 0:
            // Passthrough mode
            release(block);
            break;
            
        case 1:
            // Snap mode: Move active block to queue and pass audio through
            if (queue[position]) {
                cache = (int16_t *)queue[position]->data;
                pa = (int16_t *)(block->data);
                
                transmit(queue[position]);
                
                for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
                    sample = *pa;
                    s_cache = *cache;
                    *pa = (int16_t)(min(max((int32_t)(sample * (RecordBlend + 1.0f) / 2.0f + s_cache * (1.0f - RecordBlend)) * 1.0f, -32768), 32767));
                    pa++; cache++;
                }
                
                
                release(queue[position]);
            }
            
            if (!FadeInDone) {
                pa = (int16_t *)(block->data);
                
                *pa++ = 0;
                sample = *pa;
                *pa++ = sample / 1024;
                sample = *pa;
                *pa++ = sample / 256;
                sample = *pa;
                *pa++ = sample / 64;
                sample = *pa;
                *pa++ = sample / 16;
                sample = *pa;
                *pa++ = sample / 8;
                sample = *pa;
                *pa++ = sample / 2;
                
                FadeInDone = true;
            }
            
            recent = position;
            queue[position] = block;
            // transmit(block);
            
            position = (position < STUTTER_QUEUE_END) ? position + 1 : 0;
            length = (length < STUTTER_QUEUE_END) ? length + 1 : length;
            
            break;
            
        case 2:
            // Latch mode: loop recorded blocks
            index = (head + offset) % STUTTER_QUEUE_END;
            release(block);
            
            if (queue[index]) { transmit(queue[index]); }
            
            head = (head < (length - 1)) ? head + 1 : 0;
            
            if (head == 0) {
                if (this->Gain != 1.0f)
                {
                    if (this->Direction) // Decay
                    {
                        if (this->Decay != 1.0f) { this->Gain *= this->Decay; }
                        if (this->Gain <= 0.01f) { this->drop(); finished = true; }
                    }
                    else
                    {
                        if (this->Attack != 1.0f) { this->Gain += this->Attack; }
                        if (this->Gain >= 1.0f) { this->Direction = true; }
                    }
                }
                else
                {
                    if (!this->Direction) { this->Direction = true; }
                    if (this->Direction)
                    {
                        if (this->Decay != 1.0f) { this->Gain *= this->Decay; }
                        if (this->Gain <= 0.01f) { this->drop(); finished = true; }
                    }
                }
                
                if (this->Gain > 1.0f) { this->Gain = 1.0f; }
            }
            
            break;
    }
}


void AudioEffectStutter::snap()
{
    if (state == 1) { return; }
    
    DPRINTLN("Snap");
    
    __disable_irq();
    
    FadeInDone = false;
    offset = position;
    state = 1;
    
    Gain = Attack;
    
    Direction = false;
    __enable_irq();
}

bool AudioEffectStutter::latch()
{
    int16_t *pa;
    int16_t sample;
    audio_block_t *block;
    
    DPRINTLN("Latch");
    
    if (!state)             { return false; }
    if (position == offset) { return false; }
    
	__disable_irq();
    block = queue[recent];
    if (block) {
        pa = (int16_t *)(block->data);
        
        pa = (int16_t *)(block->data + 7);
        *pa-- = 0;
        sample = *pa;
        *pa-- = sample / 1024;
        sample = *pa;
        *pa-- = sample / 256;
        sample = *pa;
        *pa-- = sample / 64;
        sample = *pa;
        *pa-- = sample / 16;
        sample = *pa;
        *pa-- = sample / 8;
        sample = *pa;
        *pa-- = sample / 2;
        
        pa = (int16_t *)(block->data);
    }
    
    
    Direction = false;
    head = 0;
    state = 2;
    
    __enable_irq(); return true;
}

void AudioEffectStutter::dub()
{
    DPRINTLN("Dub");
    
	__disable_irq();
    if (state > 1) {
        
        state = 3;
    }
    __enable_irq();
}

void AudioEffectStutter::unlatch()
{    
    int16_t *pa;
    int16_t sample;
    audio_block_t *block;
    
    DPRINTLN("Unlatch");
    
    if (position == offset) { return false; }
    
	__disable_irq();
    if (state)
    {
        if (state != 2) {        
            state = 2;
        }
        
        
        
        //
        block = queue[recent];
        if (block) {
            pa = (int16_t *)(block->data);
            
            pa = (int16_t *)(block->data + 7);
            *pa-- = 0;
            sample = *pa;
            *pa-- = sample / 1024;
            sample = *pa;
            *pa-- = sample / 256;
            sample = *pa;
            *pa-- = sample / 64;
            sample = *pa;
            *pa-- = sample / 16;
            sample = *pa;
            *pa-- = sample / 8;
            sample = *pa;
            *pa-- = sample / 2;
            
            pa = (int16_t *)(block->data);
        }
        
        //
    }
    Direction = false;
    __enable_irq();
}

void AudioEffectStutter::drop()
{
    if (!state) { return; }
    
    DPRINTLN("Drop");
    
	__disable_irq();
    
    Gain = 0.0f;
    offset = 0;
    head = 0;
    position = 0;
    length = 0;
    state = 0;
    __enable_irq();
}

bool AudioEffectStutter::isActive()  { return (state > 0)   ? true : false; }
bool AudioEffectStutter::isSnapped() { return (state == 1)  ? true : false; }
bool AudioEffectStutter::isLatched() { return (state >= 2)  ? true : false; }
bool AudioEffectStutter::isDubbing() { return (state == 3)  ? true : false; }
bool AudioEffectStutter::isFinished() { return finished; }

void AudioEffectStutter::resetFinished()
{
    __disable_irq();
    finished = false;
    __enable_irq();
}

void AudioEffectStutter::setFade(float Fade)
{
    __disable_irq();
    this->Fade = Fade;
    __enable_irq();
}

void AudioEffectStutter::setBlend(float Blend)
{
	__disable_irq();
    RecordBlend = Blend;
    __enable_irq();
}

void AudioEffectStutter::setDecay(float Decay)
{
	__disable_irq();
    this->Decay = Decay;
    __enable_irq();
}

void AudioEffectStutter::setAttack(float Attack)
{
	__disable_irq();
    this->Attack = Attack;
    __enable_irq();
}

float AudioEffectStutter::getGain()
{
	__disable_irq();
    return this->Gain;
    __enable_irq();
}