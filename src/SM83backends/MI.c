/*
    NOT USED/UNFINISHED
*/

#include "SM83.h"
#include "chead.h"
#include "misc/queue.h"
#include <stdio.h>
#include <string.h>


struct SM83{
    struct Register reg;
    struct Node* microQ;
};

#define NOTHING 0
#define IDU_INC 1
#define IDU_DEC 2
#define ALU_ASSIGN_VALUE 1
#define ALU_ASSIGN_REG 2
#define ALU_ADD 3
#define ALU_SUB 4
#define MISC_WZtoPC 1

struct MicroInst{
    uint8_t IDUop;  // 0: Nothing 1: Inc 2: Dec
    uint8_t IDUaddr;
    uint8_t ALUop; // 0: Nothing 1: Assign
    uint8_t ALUout;
    uint8_t ALUuseValue;  // If 0, ALUb represents register ID, raw number otherwise.
    uint8_t ALUa;
    uint8_t ALUb; //
    //uint8_t ALUval;
    int8_t fetchIR;
    uint8_t DATAwrite; // If 0, Read from memory, write to memory otherwise
    uint8_t DATAaddrReg;  // Do nothing if 0xF, register where the address is read from
    uint8_t DATAreg; // Register where the data is read from/written to
    uint8_t MISC;
};

int8_t executeMI(struct MicroInst* inst, struct SM83* cpu, uint8_t* memory){
    int8_t dataBusUsed = 0;

    // DATA Bus: Load data
    if(inst->DATAaddrReg != 0xF){
        uint16_t memory_addr = register16r(&cpu->reg, inst->DATAaddrReg);
        if(inst->DATAwrite){
            printf("Data write: Address: 0x%4x, Value: 0x%2x\n", memory_addr, register8r(&cpu->reg, inst->DATAreg));
            memoryWrite(memory, memory_addr, register8r(&cpu->reg, inst->DATAreg));
        }else{
            printf("Data read: Address: 0x%4x, Value: 0x%2x\n", memory_addr, memoryRead(memory, memory_addr));
            register8w(&cpu->reg, inst->DATAreg, memoryRead(memory, memory_addr));
        }
        dataBusUsed = 1;
    }

    // IDU operation
    if(inst->IDUop == IDU_INC || inst->IDUop == IDU_DEC){
        uint16_t val = register16r(&cpu->reg, inst->IDUaddr);
        if(inst->IDUop == IDU_INC){
            val += 1;
        }
        else{
            val -= 1;
        }
        // ((inst->IDUop == IDU_INC) ? ++val : --val);
        register16w(&cpu->reg, inst->IDUaddr, val);
    }

    // ALU operation
    switch(inst->ALUop){
    case NOTHING:
        break;
    // case ALU_ASSIGN_VALUE:
    //     register8w(&cpu->reg, inst->ALUout, inst->ALUval);
    //     break;
    case ALU_ASSIGN_REG:
    {
        uint8_t val = register8r(&cpu->reg, inst->ALUa);
        register8w(&cpu->reg, inst->ALUout, val);
        printf("ALU ASSIGN: 0x%4x , val: 0x%2x\n", inst->ALUout, val );
        break;
    }
    case ALU_ADD:
    case ALU_SUB:
    {
        uint8_t num1 = register8r(&cpu->reg, inst->ALUa);
        uint8_t num2 = (inst->ALUuseValue ? inst->ALUb : register8r(&cpu->reg, inst->ALUb));
        uint8_t result = (inst->ALUop == ALU_ADD ? (num1 + num2) : (num1 - num2));
        register8w(&cpu->reg, inst->ALUout, result);
        break;
    }
    default:
        ERR("ALU NOT IMPL");
        break;
    };

    switch(inst->MISC){
    case NOTHING:
        break;
    case MISC_WZtoPC:
        cpu->reg.PC = pairRead(register8r(&cpu->reg,REG_W), register8r(&cpu->reg, REG_Z));
        printf("WZ to PC\n");
        break;
    default:
        ERR("MISC NOT IMPL");
        break;
    };

    return dataBusUsed;
} 

void decodeIR(struct SM83* cpu);

int step(struct SM83* cpu, uint8_t* memory){
     

    // int do_exit=0;
    // Queue empty?
    struct MicroInst* inst = (struct MicroInst*)dequeue(&cpu->microQ);
    int8_t dataBusUsed = 0;
    if(inst != NULL){
        // Execute MI
        dataBusUsed = executeMI(inst, cpu, memory);
        free(inst);
    }

    
    // Fetch IR
    if(!dataBusUsed && (cpu->microQ == NULL)){ //  && inst->fetchIR)
        uint16_t IR_addr = register16r(&cpu->reg, PAIR_PC);
        cpu->reg.IR = memoryRead(memory, IR_addr);
        printf("IR_addr: 0x%2x\n", IR_addr);
        decodeIR(cpu);
        cpu->reg.PC++;
    }

    debugRegister(&cpu->reg);
    printf("Memory at 0x%4x: 0x%2x\n", 0x100, memory[0x100]);
    printf("Memory at 0x%4x: 0x%2x\n", 0x101, memory[0x101]);
    printf("Memory at 0x%4x: 0x%2x\n", 0x102, memory[0x102]);

    // if(cpu->microQ == NULL){
    //     printf("Emulator finished (end of instructions)\n");
    //     return 1;
    // }
   
    return 0;
}

#define MATCH(inst, target) ((inst) == (target))
#define MATCH3(inst, target) (((inst) & (0x7)) == (target))
#define IDU_OP(inst, addr, op) {(inst)->IDUop = (op); (inst)->IDUaddr = (addr);}
#define PCINC(inst) IDU_OP(inst, PAIR_PC, IDU_INC);
#define DATA(inst, write2mem, register, addrReg) {(inst)->DATAwrite=(write2mem); (inst)->DATAreg=(register); (inst)->DATAaddrReg=(addrReg);}
#define ALU_ASSIGN(inst, out, a) {(inst)->ALUop=ALU_ASSIGN_REG; (inst)->ALUout=(out); (inst)->ALUa=(a);}

struct MicroInst* emptyMI(){
    struct MicroInst* nop = malloc(sizeof(struct MicroInst));
    nop->IDUop = NOTHING;
    nop->ALUop = NOTHING;
    nop->DATAaddrReg = 0xF;
    nop->fetchIR = 1;
    //PCINC(nop);
    return nop;
}

// BY CONVENTION, THE IR FETCHING DONE IN M1 M-CYCLE IS NOT PERFORMED HERE.
// INSTEAD, IT IS DONE IN STEP() 
void decodeIR(struct SM83* cpu){
    uint8_t raw_inst = cpu->reg.IR;
    struct Node** head = &cpu->microQ;

    printf("RAW IR: 0x%2x\n", raw_inst);

    if(raw_inst == 0){
        enqueue(&cpu->microQ, emptyMI());  // NOP instruction
        return;
    }
    // LD operations
    if(MATCHM(raw_inst, 0x46, 0xC7)){ // LD r, (HL)
        struct MicroInst* M2 = emptyMI();
        DATA(M2, 0, REG_Z, PAIR_HL);
        struct MicroInst* M3 = emptyMI();
        ALU_ASSIGN(M3, ((raw_inst & 0x38) >> 3), REG_Z);
        enqueue(head, M2); enqueue(head, M3);
        printf("LD r, (HL)\n");
    }
    else if(MATCHM(raw_inst, 0x70, 0xF8)){ // LD (HL), r
        struct MicroInst* M2 = emptyMI();
        DATA(M2, 1, raw_inst & 0x7, PAIR_HL);
        enqueue(head, M2);
        printf("LD (HL), r\n");
    }
    else if(MATCHM(raw_inst, 0x36, 0xFF)){ // LD (HL), n
        struct MicroInst* M2 = emptyMI(); 
        PCINC(M2);
        DATA(M2, 0, REG_Z, PAIR_PC);
        struct MicroInst* M3 = emptyMI();
        DATA(M3, 1, REG_Z, PAIR_HL);
        enqueue(head, M2); enqueue(head, M3);
        printf("LD (HL), imm8\n");
    }
    else if(MATCHM(raw_inst, 0xA, 0xCF)){ // LD A, [r16mem]  // TODO, TEST
        struct MicroInst* M2 = emptyMI();
        struct MicroInst* M3 = emptyMI();
        uint8_t memreg = ((raw_inst & 0x30) >> 4);
        if(memreg == 2 || memreg == 3){
            IDU_OP(M2, PAIR_HL, ((memreg == 2) ? IDU_INC : IDU_DEC));
            memreg = PAIR_HL;
        }
        DATA(M2, 0, REG_Z, memreg);
        ALU_ASSIGN(M3, REG_A, REG_Z);
        enqueue(head, M2); enqueue(head, M3);
        printf("LD A, [r16mem]\n");
    }
    else if(MATCHM(raw_inst, 0x2, 0xCF)){ // LD [r16mem], A // TEST
        struct MicroInst* M2 = emptyMI();
        uint8_t memreg = ((raw_inst & 0x30) >> 4);
        printf("LD [r16mem], A\n");
        if(memreg == 2 || memreg == 3){
            IDU_OP(M2, PAIR_HL, ((memreg == 2) ? IDU_INC : IDU_DEC));
            memreg = PAIR_HL;
        }
        DATA(M2, 1, REG_A, memreg);
        enqueue(head, M2);
    }
    else if(MATCHM(raw_inst, 0x6, 0xC7)){ // LD r8, imm8
        struct MicroInst* M2 = emptyMI(); PCINC(M2);
        DATA(M2, 0, REG_Z, PAIR_PC);
        
        struct MicroInst* M3 = emptyMI();
        ALU_ASSIGN(M3, ((raw_inst & 0x38) >> 3), REG_Z);
        printf("LD 0x%4x, 0x%4x\n", ((raw_inst & 0x38) >> 3), REG_Z);
        enqueue(head, M2); enqueue(head, M3);
    }
    else if(MATCHM(raw_inst, 0x40, 0xC0)){ // LD r8, r8
        struct MicroInst* M2 = emptyMI();
        ALU_ASSIGN(M2, ((raw_inst & 0x38) >> 3), (raw_inst & 0x7));
        printf("LD 0x%4x, 0x%4x\n", ((raw_inst & 0x38) >> 3), (raw_inst & 0x7));
        enqueue(head, M2);
    }
    else if(MATCH(raw_inst, 0xFA)){ // LDH A, (nn)
        struct MicroInst *M2 = emptyMI(), *M3 = emptyMI(), *M4 = emptyMI(), *M5 = emptyMI();
        PCINC(M2); DATA(M2, 0, REG_Z, PAIR_PC);
        PCINC(M3); DATA(M3, 0, REG_W, PAIR_PC);
        DATA(M4, 0, REG_Z, PAIR_WZ);
        ALU_ASSIGN(M5, REG_A, REG_Z);
        enqueue(head, M2); enqueue(head, M3); enqueue(head, M4); enqueue(head, M5);
    }
    else if(MATCH(raw_inst, 0xEA)){ // LDH (nn), A
        struct MicroInst *M2 = emptyMI(), *M3 = emptyMI(), *M4 = emptyMI();
        PCINC(M2); DATA(M2, 0, REG_Z, PAIR_PC);
        PCINC(M3); DATA(M3, 0, REG_W, PAIR_PC);
        DATA(M4, 1, REG_A, PAIR_WZ);
        enqueue(head, M2); enqueue(head, M3); enqueue(head, M4);
    }
    else if(MATCH(raw_inst, 0xF2)){ // LDH A, (C)
        struct MicroInst *M2 = emptyMI(), *M3 = emptyMI();
        DATA(M2, 0, REG_Z, PAIR_FF_C);
        ALU_ASSIGN(M2, REG_A, REG_Z);
        enqueue(head, M2); enqueue(head, M3);
    }
    else if(MATCH(raw_inst, 0xE2)){ // LDH (C), A
        struct MicroInst *M2 = emptyMI();
        DATA(M2, 1, REG_A, PAIR_FF_C);
        enqueue(head, M2);
    }
    else if(MATCH(raw_inst, 0xF0)){  // LDH A, (n)
        struct MicroInst *M2 = emptyMI(), *M3 = emptyMI(), *M4 = emptyMI();
        PCINC(M2); DATA(M2, 0, REG_Z, PAIR_PC);                                                                                                                                                                                                            
        DATA(M3, 0, REG_Z, PAIR_FF_Z);
        ALU_ASSIGN(M4, REG_A, REG_Z);
        enqueue(head, M2); enqueue(head, M3); enqueue(head, M4);
    }
    else if(MATCH(raw_inst, 0xE0)){ // LDH (n), A

    }
    // INC/DEC
    // !!!! >>> TODO: FLAGS <<<< !!!!
    if(MATCH3(raw_inst, 0x4) || MATCH3(raw_inst, 0x5)){
        printf("INC/DEC\n");
        struct MicroInst* M2 = emptyMI();
        uint8_t target_register =  ((raw_inst & 0x38) >> 3);
        M2->ALUout = target_register;
        M2->ALUa = target_register; M2->ALUb = 1; M2->ALUuseValue = 1;
        M2->ALUop = (raw_inst & 1 ? ALU_SUB : ALU_ADD);
        enqueue(head, M2);
    }

    // JP
    if(MATCH(raw_inst, 0xC3)){
        printf("JUMP\n");
        struct MicroInst *M2 = emptyMI(), *M3 = emptyMI(), *M4 = emptyMI(); //, *M5 = emptyMI();
        PCINC(M2); PCINC(M3); PCINC(M4);
        DATA(M2, 0, REG_Z, PAIR_PC);
        DATA(M3, 0, REG_W, PAIR_PC);
        M4->MISC = MISC_WZtoPC; M4->fetchIR = 0;  // !!!!!! Maybe there is a timing problem because M5 omitted
        enqueue(head, M2); enqueue(head, M3); enqueue(head, M4);// enqueue(head, M5);
    }
}

