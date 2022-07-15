#include <Arduino.h>
#include "effect_stutter.h"

void AudioEffectStutter::update(void)
{
#if defined(__ARM_ARCH_7EM__)
	audio_block_t *blocka;
    int i;
    
	blocka = receiveReadOnly(0);
	if (!blocka) { return; }
    
    switch (state)
    {
        case 0:
            // Passthrough mode
            transmit(blocka);
            release(blocka);
            
            break;
            
        case 1:
            // Snap mode: Move active block to queue and pass audio through
            if (queue[position]) { release(queue[position]); }
            
            queue[position] = blocka;
            transmit(blocka);
            
            position = (position < (STUTTER_QUEUE_SIZE - 1)) ? position + 1 : 0;
            length = (length < (STUTTER_QUEUE_SIZE - 1)) ? length + 1 : length;
            
            break;
            
        case 2:
            // Latch mode: loop recorded blocks
            release(blocka);
            
            i = (head + offset) % (STUTTER_QUEUE_SIZE - 1);
            
            if (queue[i]) { transmit(queue[i]); }
            
            head = (head < (length - 1)) ? head + 1 : 0;
            
            break;
    }
#elif defined(KINETISL)
	audio_block_t *block;

	block = receiveReadOnly(0);
	if (block) release(block);
#endif
}


void AudioEffectStutter::snap() {
    if (state == 1) { return; }
    
    offset = position;
    state = 1;
}

bool AudioEffectStutter::latch() {
    if (!state) { return false; }
    if (position == offset) { return false; }
    
    head = 0;
    state = 2;
    
    return true;
}

void AudioEffectStutter::unlatch() {
    if (!state) { return; }
    
    offset = 0;
    head = 0;
    position = 0;
    length = 0;
    state = 0;
}

bool AudioEffectStutter::isActive() {
    return (state > 0) ? true : false;
}

bool AudioEffectStutter::isSnapped() {
    return (state == 1) ? true : false;
}

bool AudioEffectStutter::isLatched() {
    return (state == 2) ? true : false;
}