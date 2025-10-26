#include "serial.h"
#include "SM83.h"
#include "registers.h"
#include <unistd.h>

#define SC_ENABLE 7

// I think there's a bug but IDK
// Doesn't matter anyway
void processSerialTransfer(uint8_t* memory, FILE* out){
    uint8_t SC = memoryRead(memory, ADDR_SC);
    if(BIT_N(SC, SC_ENABLE)){
        char letter = memoryRead(memory, ADDR_SB); 
        //fputs(&letter, out);  // output
        write(STDOUT_FILENO, &letter, 1); 

        // Set corresponding IF flag
        uint8_t new_IF = SET(memoryRead(memory, ADDR_IF), INT_SERIAL);
        memoryWrite(memory, ADDR_IF, new_IF);

        // Clear transfer enable flag
        uint8_t new_SC = RESET(SC, SC_ENABLE);
        memoryWrite(memory, ADDR_SC, new_SC);
    }
}