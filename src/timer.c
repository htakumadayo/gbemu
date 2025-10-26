#include "timer.h"
#include <stdlib.h>

const uint16_t TAC_clock[] = {
    256, 4, 16, 64
};

struct Timer* initTimer(){
    struct Timer* timer = malloc(sizeof(struct Timer));
    timer->copy_wait = 0;
    timer->TIMA_cooldown = 0;
    timer->div_real_value = 0;
    timer->last_div_tick_time = 0;
    return timer;
}

void tickTimer(uint8_t* memory, struct Timer* timer){
    if((memory[ADDR_TAC] & 0x4) == 0)
        return;

    if(timer->copy_wait > 0){  // Copied and interrupt requested only one cycle after
        memory[ADDR_TIMA] = memory[ADDR_TMA];
        memory[ADDR_IF] = SET(memory[ADDR_IF], INT_TIMER);
        timer->copy_wait = 0;
    }

    if(timer->TIMA_cooldown == 0){ 
        memory[ADDR_TIMA] += 1;
        timer->TIMA_cooldown = TAC_clock[(memory[ADDR_TAC] & 0x3)];

        if(memory[ADDR_TIMA] == 0){
            timer->copy_wait = 1;
        }
    }

    --timer->TIMA_cooldown;
}

void tickDiv(uint8_t* memory, struct Timer* timer, uint64_t current_time){
     // Check for DIV modification
    // >> TODO << : This method is not very reliable. Might have to modify it
    if(timer->div_real_value != memory[ADDR_DIV]){
        memory[ADDR_DIV] = 0;
        timer->div_real_value = 0;
    }

    // Increment DIV
    const uint64_t DIV_inc_period_ns = (1/DIV_FREQ)*1e9;
    if(timer->last_div_tick_time - current_time > DIV_inc_period_ns){
        timer->last_div_tick_time = current_time;
        ++memory[ADDR_DIV];
        timer->div_real_value = memory[ADDR_DIV];
    }
}