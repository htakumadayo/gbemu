#ifndef EMU_TIMER
#define EMU_TIMER

#include <stdlib.h>
#include "SM83.h"
#include "registers.h"

extern const uint16_t TAC_clock[];

struct Timer{
    uint16_t TIMA_cooldown;  // Countdown for next TIMA increment
    int copy_wait;  // The obscure timer behaviour: wait 1 M-cycle until copying TMA to TIMA
};

void initTimer(struct Timer* timer);
void tickTimer(uint8_t* memory, struct Timer* timer);


#endif