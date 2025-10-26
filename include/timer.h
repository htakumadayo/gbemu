#ifndef EMU_TIMER
#define EMU_TIMER

#include "SM83.h"
#include "registers.h"

extern const uint16_t TAC_clock[];

#define DIV_FREQ 16384.0

struct Timer{
    uint16_t TIMA_cooldown;  // Countdown for next TIMA increment
    int copy_wait;  // The obscure timer behaviour: wait 1 M-cycle until copying TMA to TIMA
    uint8_t div_real_value; // Variable used to detect whether a writing happened on the DIV register.
    uint64_t last_div_tick_time;
};

struct Timer* initTimer();
void tickTimer(uint8_t* memory, struct Timer* timer);
void tickDiv(uint8_t* memory, struct Timer* timer, uint64_t current_time);

#endif