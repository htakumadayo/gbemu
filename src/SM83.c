#include "SM83.h"




void execute(struct Register* reg){
}

uint16_t pairRead(uint8_t MSB, uint8_t LSB){
    return ((uint16_t)MSB << 8) + LSB;
}

void pairWrite(uint8_t* MSB, uint8_t* LSB, uint16_t value){
    const uint16_t mask = 0x100 - 1;
    *LSB = (uint8_t)(mask & value);
    *MSB = (uint8_t)(mask & (value >> 8));
}

// 0: PC, 1: SP, 2: AF, 3: BC, 4: DE, 5: HL
uint16_t register16r(struct Register* reg, uint8_t id){
    switch(id){
    case PC:
        return reg->PC;
    case SP:
        return reg->SP;
    case AF:
        return pairRead(*reg->A, *reg->F);
    case BC:
        return pairRead(*reg->B, *reg->C);
    case DE:
        return pairRead(*reg->D, *reg->E);
    case HL:
        return pairRead(*reg->H, *reg->L);
    default:
        ERR("Invalid Register read");
    };
}

void register16w(struct Register* reg, uint8_t id, uint16_t val){
    switch(id){
    case PC:
        reg->PC = val; break;
    case SP:
        reg->SP = val; break;
    case AF:
        pairWrite(reg->A, reg->F, val); break;
    case BC:
        pairWrite(reg->B, reg->C, val); break;
    case DE:
        pairWrite(reg->D, reg->E, val); break;
    case HL:
        pairWrite(reg->H, reg->L, val); break;
    default:
        ERR("Invalid Register write");

    };
}

void executeMI(MicroInst* inst, struct SM83* cpu){
    
    // IDU operation
    if(inst->IDUop == 1 || inst->IDUop == 2){
        uint16_t val = register16r(&cpu->register, inst->IDaddr);
        (inst->IDUop == 1 ? ++val : --val);
        register16w(&cpu->register, val);
    }


    
} 



void step(struct SM83* cpu){
    // Queue empty?
    MicroInst* inst = (MicroInst*)dequeue(cpu->microQ);
    if(inst != NULL){
        // Execute MI

    }

    // Fetch IR

    
}
