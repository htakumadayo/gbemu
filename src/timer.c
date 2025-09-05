#include "timer.h"

const uint16_t TAC_clock[] = {
    256, 4, 16, 64
};

void initTimer(struct Timer* timer){
    timer->copy_wait = 0;
    timer->TIMA_cooldown = 0;
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