#include <Arduino.h>
#include "effect_stutter.h"

void AudioEffectStutter::update(void)
{
#if defined(__ARM_ARCH_7EM__)
    audio_block_t *block;
    int16_t *pa, *cache;
    int16_t sample, s_cache;
    int index;
    
    block = receiveWritable(0);
    if (!block) { return; }
    
    switch (state)
    {
        case 0:
            // Passthrough mode
            // transmit(block);
            release(block);
            
            break;
            
        case 1:
            // Snap mode: Move active block to queue and pass audio through
            if (queue[position]) {
                cache = (int16_t *)queue[position]->data;
                pa = (int16_t *)(block->data);
                
                for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
                    sample = *pa;
                    s_cache = *cache;
                    *pa = (int16_t)(min(max((int32_t)(sample * RecordBlend + s_cache * (1.0f - RecordBlend)) * 1.5f, -32768), 32767));
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
                
                // pa = (int16_t *)(block->data);
                // 
                // Serial.print(*pa); Serial.print(" : ");
                // Serial.print(*(pa+1)); Serial.print(" : ");
                // Serial.print(*(pa+2)); Serial.print(" : ");
                // Serial.print(*(pa+3)); Serial.print(" : ");
                // Serial.print(*(pa+4)); Serial.print(" : ");
                // Serial.print(*(pa+5)); Serial.print(" : ");
                // Serial.print(*(pa+6)); Serial.print(" : ");
                // Serial.println(*(pa+7));
                // 
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
            release(block);
            
            index = (head + offset) % STUTTER_QUEUE_END;
            
            if (queue[index]) { transmit(queue[index]); }
            
            head = (head < (length - 1)) ? head + 1 : 0;
            
            break;
    }
#elif defined(KINETISL)
    audio_block_t *block;

    block = receiveReadOnly(0);
    if (block) release(block);
#endif
}


void AudioEffectStutter::snap()
{
    if (state == 1) { return; }
    
    FadeInDone = false;
    
    offset = position;
    state = 1;
}

bool AudioEffectStutter::latch()
{
	__disable_irq();

    int16_t *pa;
    int16_t sample;
    audio_block_t *block;
    
    if (!state) { return false; }
    if (position == offset) { return false; }
    
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
        
        // Serial.print(*pa); Serial.print(" : ");
        // Serial.print(*(pa+1)); Serial.print(" : ");
        // Serial.print(*(pa+2)); Serial.print(" : ");
        // Serial.print(*(pa+3)); Serial.print(" : ");
        // Serial.print(*(pa+4)); Serial.print(" : ");
        // Serial.print(*(pa+5)); Serial.print(" : ");
        // Serial.print(*(pa+6)); Serial.print(" : ");
        // Serial.println(*(pa+7));
    }
    
    head = 0;
    state = 2;
    
    __enable_irq();
    return true;
}

void AudioEffectStutter::unlatch()
{
    if (!state) { return; }
    
    offset = 0;
    head = 0;
    position = 0;
    length = 0;
    state = 0;
}

bool AudioEffectStutter::isActive()  { return (state > 0)   ? true : false; }
bool AudioEffectStutter::isSnapped() { return (state == 1)  ? true : false; }
bool AudioEffectStutter::isLatched() { return (state == 2)  ? true : false; }

void AudioEffectStutter::setFade(float Fade) { this->Fade = Fade; }

void AudioEffectStutter::setBlend(float Blend)
{
	__disable_irq();
    RecordBlend = Blend;
    __enable_irq();
}