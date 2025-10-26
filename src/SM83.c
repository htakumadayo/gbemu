#include "SM83.h"
#include "registers.h"
#include <string.h>

const uint16_t HANDLER_ADDR[] = {
    [INT_VBLANK] = 0x40,
    [INT_STAT] = 0x48,
    [INT_TIMER] = 0x50,
    [INT_SERIAL] = 0x58,
    [INT_JOYPAD] = 0x60,
};

const uint8_t to_r16stk[] = {
    [PAIR_BC] = PAIR_BC,
    [PAIR_DE] = PAIR_DE,
    [PAIR_HL] = PAIR_HL,
    [PAIR_SP] = PAIR_AF,  
};

void initCPU(struct SM83* cpu){
    cpu->reg.A = 0x01;
    cpu->reg.F = 0xB0;
    cpu->reg.C = 0x13;
    cpu->reg.B = 0x00;
    cpu->reg.D = 0;
    cpu->reg.E = 0xD8;
    cpu->reg.H = 0x01;
    cpu->reg.L = 0x4D;
    cpu->reg.SP = 0xFFFE;
    cpu->reg.PC = 0x0100;
    cpu->reg.IME = 0;
    cpu->reg.STAT = 0;
    cpu->halt_mode = 0;
}


uint8_t memoryRead(const uint8_t* memory, uint16_t address){
    return memory[address];
}

void memoryWrite(uint8_t* memory, uint16_t address, uint8_t value){
    memory[address] = value;
    return;
}

uint16_t pairRead(uint8_t MSB, uint8_t LSB){
    return ((uint16_t)MSB << 8) + LSB;
}

void pairWrite(uint8_t* MSB, uint8_t* LSB, uint16_t value){
    const uint16_t mask = 0x100 - 1;
    *LSB = (uint8_t)(mask & value);
    *MSB = (uint8_t)(mask & (value >> 8));
}

uint8_t lsb(uint16_t n){
    return n & 0x00FF;
}

uint8_t msb(uint16_t n){
    return ((n & 0xFF00) >> 8);
}

uint16_t register16r(struct Register* reg, uint8_t id){
    switch(id){
    case PAIR_PC:
        return reg->PC;
    case PAIR_SP:
        return reg->SP;
    case PAIR_AF:
        return pairRead(reg->A, reg->F);
    case PAIR_BC:
        return pairRead(reg->B, reg->C);
    case PAIR_DE:
        return pairRead(reg->D, reg->E);
    case PAIR_HL:
        return pairRead(reg->H, reg->L);
    case PAIR_WZ:
        return pairRead(reg->W, reg->Z);
    case PAIR_FF_C:
        return 0xFF00 + reg->C;
    default:
        ERR("(Register16r) Invalid Register read");
    };
}

void register16w(struct Register* reg, uint8_t id, uint16_t val){
    switch(id){
    case PAIR_PC:
        reg->PC = val; break;
    case PAIR_SP:
        reg->SP = val; break;
    case PAIR_AF:  // This is the only way to directly write to F. The lower nibble is always zero
        pairWrite(&reg->A, &reg->F, val);
        reg->F = (0xF0 & reg->F);
        break;
    case PAIR_BC:
        pairWrite(&reg->B, &reg->C, val); break;
    case PAIR_DE:
        pairWrite(&reg->D, &reg->E, val); break;
    case PAIR_HL:
        pairWrite(&reg->H, &reg->L, val); break;
    case PAIR_WZ:
        pairWrite(&reg->W, &reg->Z, val); break;
    default:
        ERR("(Register16w) Invalid Register write");

    };
}

uint8_t register8r(struct Register* reg, uint8_t id){
    switch(id){
    case REG_A:
        return reg->A;
    case REG_B:
        return reg->B;
    case REG_C:
        return reg->C;
    case REG_D:
        return reg->D;
    case REG_E:
        return reg->E;
    case REG_H:
        return reg->H;
    case REG_L:
        return reg->L;
    case REG_Z:
        return reg->Z;
    case REG_W:
        return reg->W;
    case REG_HL:
        ERR("(Register8r) IDK what to do");
    default:
        ERR("(Register8r) Invalid Register read");
    };
    return 0;
}

void register8w(struct Register* reg, uint8_t id, uint8_t val){
    switch(id){
    case REG_A:
        //printf("Write to A: 0x%x\n", val);
        reg->A = val; break;
    case REG_B:
        reg->B = val; break;
    case REG_C:
        reg->C = val; break;
    case REG_D:
        reg->D = val; break;
    case REG_E:
        reg->E = val; break;
    case REG_H:
        reg->H = val; break;
    case REG_L:
        reg->L = val; break;
    case REG_Z:
        //printf("Write to Z: 0x%x\n", val);
        reg->Z = val; break;
    case REG_W:
        //printf("Write to W: 0x%x\n", val);
        reg->W = val; break;
    case REG_HL:
        pairWrite(&reg->H, &reg->L, val); break;
    default:
        ERR("(Register8w) Invalid Register Addr.");
    };
}

uint8_t gt0(uint8_t n){  // GreaterThan0
    return (n > 0 ? 1 : 0);
}

#define FLG_Z 7
#define FLG_N 6
#define FLG_H 5
#define FLG_C 4


// 0: Bit not set, Otherwise: bit set
void setFlag(struct Register* reg, uint8_t Z, uint8_t N, uint8_t H, uint8_t C){
    uint8_t new_flag = (gt0(Z) << FLG_Z) + (gt0(N) << FLG_N) + (gt0(H) << FLG_H) + (gt0(C) << FLG_C);
    reg->F = new_flag;
}

uint8_t getFlag(struct Register* reg, uint8_t bit){
    return (reg->F & (1 << bit)) >> bit;
}

uint8_t getCflag(struct Register* reg){
    return getFlag(reg, FLG_C);
}
uint8_t getHflag(struct Register* reg){
    return getFlag(reg, FLG_H);
}
uint8_t getNflag(struct Register* reg){
    return getFlag(reg, FLG_N);
}
uint8_t getZflag(struct Register* reg){
    return getFlag(reg, FLG_Z);
}



uint8_t halfCarry8bitAdd(uint8_t a, uint8_t b, uint8_t c){
    uint8_t mask = 0x0F;
    uint8_t result = (mask & a) + (mask & b) + (mask & c);
    return result & 0x10;
}

uint8_t carry8bitAdd(uint8_t a, uint8_t b, uint8_t c){
    uint16_t result = (uint16_t)a + (uint16_t)b + (uint16_t)c;
    return (result & 0x0100) > 0;
}

uint8_t halfCarry8bitSub(uint8_t a, uint8_t b, uint8_t c){  // a - b < 0 <=> a < b
    return (int)(0x0F & a) - (int)(0x0F & b) - (int)(0x0F & c) < 0;
}

uint8_t carry8bitSub(uint8_t a, uint8_t b, uint8_t c){
    return (int)a - (int)b - (int)c < 0;
}

uint8_t halfCarry16bitAdd(uint16_t a, uint16_t b){
    uint16_t hc = ((0x0FFF & a) + (0x0FFF & b)) & (1 << 12);
    return (hc>0 ? 1 : 0);
}

uint8_t carry16bitAdd(uint16_t a, uint16_t b){
    uint32_t c = ((uint32_t)a + (uint32_t)b) & (1 << 16);
    return (c>0 ? 1 : 0);
}


void setInterruptFlag(uint8_t* memory, uint8_t bit){
    DIRECT_SET(memory[ADDR_IF], bit);
}

void debugRegister(struct Register* reg){
    printf("==== REGISTER ====\n");
    printf("AF: 0x%04x | ", pairRead(reg->A, reg->F));
    printf("Z: %d, N: %hd, H: %hd, C:%hd\n", getZflag(reg), getNflag(reg), getHflag(reg), getCflag(reg));
    printf("BC: 0x%04x\n", pairRead(reg->B, reg->C));
    printf("DE: 0x%04x\n", pairRead(reg->D, reg->E));
    printf("HL: 0x%04x\n", pairRead(reg->H, reg->L));
    printf("SP: 0x%04x\n", reg->SP);
    printf("PC: 0x%04x\n", reg->PC);
    printf("WZ: 0x%04x\n", pairRead(reg->W, reg->Z) );
}

void debugMemory(const uint8_t* memory, uint16_t start){
    printf("==== MEMORY ===\n");
    for(uint16_t row=0; row < 4; ++row){
        printf("%u | ", start + 16*row);
        for(uint16_t col=0; col<16; ++col){ // Display 2 by 2
            uint16_t addr = start + col + row*16;
            printf("%02x ", memoryRead(memory, addr)); 
        }
        printf("\n");
    }
}
