#ifndef EMU_SM83_H
#define EMU_SM83_H

#include "common.h"
#include <stdlib.h>

/* TODO:
 * What to implement?
 * - Control Unit
 * - Register
 * - ALU
 * - IDU
*/

/*
SOME FEATURES ARE NOT IMPLEMENTED YET, SUCH AS HALT BUG, RESTRICTED MEMORY ACCESS, etc...
*/



// M-cycle frequency
#define CLOCKF (1 << 20)
#define MEMORY_SIZE (1 << 16) // In bytes

// INTERRUPT FLAG BITS
#define INT_JOYPAD 4
#define INT_SERIAL 3
#define INT_TIMER 2
#define INT_LCD 1
#define INT_VBLANK 0

// Number of possible interrupts (except stat)
#define INT_NUMBER 5

#define INT_STAT 5


#define HALT_SIGNAL -1


extern const uint16_t HANDLER_ADDR[]; 
// Had to remap for pop and push ops (3=af instead of sp)
extern const uint8_t to_r16stk[]; 

// 8 bit reg. IDS
#define REG_B 0
#define REG_C 1
#define REG_D 2
#define REG_E 3
#define REG_H 4
#define REG_L 5
#define REG_HL 6
#define REG_A 7
#define REG_IR 8
#define REG_Z 9
#define REG_W 10

// Pair IDS
#define PAIR_BC 0
#define PAIR_DE 1
#define PAIR_HL 2
#define PAIR_SP 3
#define PAIR_AF 4
#define PAIR_PC 5
#define PAIR_WZ 6
#define PAIR_FF_C 7
#define PAIR_FF_Z 8

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
    uint8_t  IME;
    uint8_t  STAT;
    uint8_t  Z;  // Temporary register to store data(?)
    uint8_t  W;
};

struct SM83{
    struct Register reg;

    int halt_mode;
};


uint8_t memoryRead(const uint8_t* memory, uint16_t address);
void memoryWrite(uint8_t* memory, uint16_t address, uint8_t value);

uint16_t pairRead(uint8_t MSB, uint8_t LSB);
void pairWrite(uint8_t* MSB, uint8_t* LSB, uint16_t value);

uint8_t lsb(uint16_t n);
uint8_t msb(uint16_t n);

uint16_t register16r(struct Register* reg, uint8_t id);
void register16w(struct Register* reg, uint8_t id, uint16_t val);
uint8_t register8r(struct Register* reg, uint8_t id);
void register8w(struct Register* reg, uint8_t id, uint8_t val);

void setFlag(struct Register* reg, uint8_t Z, uint8_t N, uint8_t H, uint8_t C);
uint8_t getCflag(struct Register* reg);
uint8_t getHflag(struct Register* reg);
uint8_t getNflag(struct Register* reg);
uint8_t getZflag(struct Register* reg);

/* Flags stuff*/
#define ZERO(value) ((value) == 0)
// a + b + c
// Returns NON-ZERO value if there IS half carry
uint8_t halfCarry8bitAdd(uint8_t a, uint8_t b, uint8_t c);
// Returns NON-ZERO value if there IS carry
uint8_t carry8bitAdd(uint8_t a, uint8_t b, uint8_t c);
// a - b - c
uint8_t halfCarry8bitSub(uint8_t a, uint8_t b, uint8_t c);
// a - b - c
uint8_t carry8bitSub(uint8_t a, uint8_t b, uint8_t c);

// 12th bit carry
uint8_t halfCarry16bitAdd(uint16_t a, uint16_t b);
uint8_t carry16bitAdd(uint16_t a, uint16_t b);

#define BIT_N(target, N) ((target & (1 << N)) >> N)
#define SET(target, N) ((1 << (N)) | (target))
#define RESET(target, N) (     (~(1 << (N))) & (target)    )
 
void debugRegister(struct Register* reg);
void debugMemory(const uint8_t* memory, uint16_t start);

// Defined in backend code
// Executes one step(whose definition depend on the implementation of the background) of the CPU
// Returns an integer: positive value means the number of M-Cycle to ignore until the next call
// (Strictly) Negative values mean other stuff (TBD)
int step(struct SM83* cpu, uint8_t* memory);

#define MATCHM(inst, target, mask) (((inst) & (mask)) == (target))


#endif
