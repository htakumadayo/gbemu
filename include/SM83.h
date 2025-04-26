#ifndef EMU_SM83_H
#define EMU_SM83_H

#include "common.h"
#include "misc/queue.h"

/* TODO:
 * What to implement?
 * - Control Unit
 * - Register
 * - ALU
 * - IDU
*/

#define CLOCKF (1 << 20)

// Pair IDS
#define PC 0
#define SP 1
#define AF 2
#define BC 3
#define DE 4
#define HL 5

struct Register{
    uint16_t PC;  // Program Counter
    uint16_t SP;  // Stack Pointer
    uint8_t  A ;
    uint8_t  F ;
    uint8_t  B ;
    uint8_t  C ;
    uint8_t  D ;
    uint8_t  E ;
    uint8_t  H ;
    uint8_t  L ;
    uint8_t  IR;
    uint8_t  IE;
};

struct MicroInst{
    uint8_t IDUop;  // 0: Nothing 1: Inc 2: Dec
    uint8_t IDaddr;
    uint8_t ALUop; // Not implemented
    uint8_t ALUout;
    uint8_t ALUa;
    uint8_t ALUb;
};

struct SM83{
    struct Register reg;
    struct Node* microQ;
};
// ALU 
// IDU

void step(); // One M-cycle


#endif
