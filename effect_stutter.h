#ifndef effect_stutter_h_
#define effect_stutter_h_
#include "Arduino.h"
#include "AudioStream.h"
#include "utility/dspinst.h"


#if defined(__IMXRT1062__)
  // 4.00 second maximum on Teensy 4.0
  #define STUTTER_QUEUE_SIZE  14300 // (176512 / AUDIO_BLOCK_SAMPLES)
#elif defined(__MK66FX1M0__)
  // 2.41 second maximum on Teensy 3.6
  #define DELAY_QUEUE_SIZE  (106496 / AUDIO_BLOCK_SAMPLES)
#elif defined(__MK64FX512__)
  // 1.67 second maximum on Teensy 3.5
  #define DELAY_QUEUE_SIZE  (73728 / AUDIO_BLOCK_SAMPLES)
#elif defined(__MK20DX256__)
  // 0.45 second maximum on Teensy 3.1 & 3.2
  #define DELAY_QUEUE_SIZE  (19826 / AUDIO_BLOCK_SAMPLES)
#else
  // 0.14 second maximum on Teensy 3.0
  #define DELAY_QUEUE_SIZE  (6144 / AUDIO_BLOCK_SAMPLES)
#endif

class AudioEffectStutter : public AudioStream
{
public:
	AudioEffectStutter() : AudioStream(1, inputQueueArray) { }
	virtual void update(void);
    void snap(); // snap (set loop start)
    bool latch(); // latch (loop from start to current position)
    void unlatch(); // release and return to passthrough mode
    bool isActive(); // is snapped (listening) or latched (looping)
    bool isSnapped(); // is snapped (listening)
    bool isLatched(); // is latched (looping)
private:
	audio_block_t *inputQueueArray[1];
	audio_block_t *queue[STUTTER_QUEUE_SIZE];
    uint16_t position = 0, offset = 0, head = 0, length = 0; // 0 ... STUTTER_QUEUE_SIZE
    int state = 0; // 0 ... passthrough, 1 ... snap, 2 ... latch
};

#endif
