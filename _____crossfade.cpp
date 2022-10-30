


fadeB = (float)head / length;
fadeA = 1.0f - fadeB;

mirror = (int)(offset + fadeA * length) % STUTTER_QUEUE_END;


// Serial.print(index); Serial.print("; "); Serial.println(mirror);
// dist = floor(length / 2) - index;
// mirror = (int)(floor(length / 2) + 1 + dist) % STUTTER_QUEUE_END;


// if (queue[index]) { pa = (int16_t *)(queue[index]->data); } //{ transmit(queue[index]); }
// if (!queue[mirror]) { release(block); break; }

fadeBlock = allocate();
release(block);


if (queue[index] && queue[mirror])
{
    cache = (int16_t *)fadeBlock->data;
    pa = (int16_t *)queue[index]->data;
    pma = (int16_t *)queue[index]->data;
    
    
    for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
        sample = *pa;
        s_cache = *pma;
        *cache = (int16_t)(sample * fadeA + s_cache * fadeB);
        //Serial.print(*pma); Serial.print("; ");
        pa++; pma++; cache++;
    }
    Serial.println("");
    
} 
// else
// {
//     if (queue[index]) {
//         cache = (int16_t *)fadeBlock->data;
//         pa = (int16_t *)queue[index]->data;
//         memcpy(cache, pa, sizeof(int16_t) * AUDIO_BLOCK_SAMPLES);
//     }
// }

transmit(fadeBlock);
release(fadeBlock);

head = (head < (length - 1)) ? head + 1 : 0;
break;






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