#include "SM83.h"
#include "registers.h"

/*
The "naive" implementation of the CPU. Does not bother with memory access timing, only instruction level timing.
*/

// In reality, interrupt is enabled only after the next instruction. 
// Interrupt will be serviced only when IME = 1. The next step() call will subtract one from IME to emulate that behavior.
#define ENABLE_INTERRUPT 2   

int execInst(struct Register* reg, uint8_t* memory, uint8_t inst);

int execNormal(struct Register* reg, uint8_t* memory, uint8_t inst);
int execExt(struct Register* reg, uint8_t* memory, uint8_t inst);

void callFunc(struct Register* reg, uint8_t* memory, uint16_t addr);

int step(struct SM83* cpu, uint8_t* memory){
    struct Register* reg = &cpu->reg;

    // Halt check
    uint8_t IE = memoryRead(memory, ADDR_IE);
    uint8_t IF = memoryRead(memory, ADDR_IF);
    if(cpu->halt_mode){
        if((IE & IF) > 0){  // Exit halt mode if trigger is enabled & requested
            cpu->halt_mode = 0;
        }
        else{  // Wait 1 M-cycle
            return 1;
        }
    }

    // Check for interrupt
    uint8_t *IME = &cpu->reg.IME;
    if(*IME > 1){
        --*IME;
    }
    else if(*IME == 1){  // OK
        // FETCH what interrupt to be serviced
        for(uint8_t int_id=0; int_id < INT_NUMBER; ++int_id){  // 0 has the highest priority
            if(BIT_N(IE, int_id) && BIT_N(IF, int_id)){  // Condition met
                memoryWrite(memory, ADDR_IF, RESET(IF, int_id));  // Clear corresponding IF flag
                *IME = 0;  // Reset IME
                callFunc(reg, memory, HANDLER_ADDR[int_id]);
                return 4;  // 2 for 2 nops, 2 for pushing to stack
            }
        }
    }

    uint8_t inst = memoryRead(memory, reg->PC++);

#ifdef CPU_DEBUG
    //printf("OPCODE: 0x%02x\n", inst);
#endif

    int result = execInst(reg, memory, inst);
    if(result == HALT_SIGNAL){
        cpu->halt_mode = 1;
        return 1;  // Length of halt
    }

    return result;
}

uint8_t readMemInc(uint8_t* memory, struct Register* reg){
    return memoryRead(memory, reg->PC++);
}

int8_t readSignedInc(uint8_t* memory, struct Register* reg){
    uint8_t data = readMemInc(memory, reg);
    return *((int8_t*)&data);
}

uint8_t add8bitSetFlag(struct Register* reg, uint8_t a, uint8_t b, uint8_t use_carry){
    uint8_t carry = (use_carry ? getCflag(reg) : 0);
    uint8_t result = a + b + carry;
    setFlag(reg, ZERO(result), 0, halfCarry8bitAdd(a, b, carry), carry8bitAdd(a, b, carry));
    return result;
}

uint8_t sub8bitSetFlag(struct Register* reg, uint8_t a, uint8_t b, uint8_t use_carry){
    uint8_t carry = (use_carry ? getCflag(reg) : 0);
    uint8_t result = a - b - carry;
    setFlag(reg, ZERO(result), 1, halfCarry8bitSub(a, b, carry), carry8bitSub(a, b, carry));
    return result;
}

void add8bitToA(struct Register* reg, uint8_t other, uint8_t use_carry){
    reg->A = add8bitSetFlag(reg, reg->A, other, use_carry);
}

void sub8bitToA(struct Register* reg, uint8_t other, uint8_t use_carry){
    reg->A = sub8bitSetFlag(reg, reg->A, other, use_carry);
}

uint8_t incdecSetFlag(struct Register* reg, uint8_t target, uint8_t decrement){
    uint8_t result = (decrement ? target - 1 : target + 1);
    setFlag(reg, ZERO(result), decrement, (decrement ? halfCarry8bitSub(target, 1, 0) : halfCarry8bitAdd(target, 1, 0)), getCflag(reg));
    return result;
}


int execInst(struct Register* reg, uint8_t* memory, uint8_t inst){
    if(inst == 0xCB){  // extended instruction set
        return execExt(reg, memory, readMemInc(memory, reg));
    }
    else{
        return execNormal(reg, memory, inst);
    }
}

#define B11000000 0xC0
#define B00111000 0x38
#define B00000111 0x07
#define B11000111 (B11000000 + B00000111)
#define B11111000 (B11000000 + B00111000)
#define MID3(inst) ((inst & B00111000) >> 3)
#define LOW3(inst) (inst & B00000111)
#define CC(inst) ((inst & B00011000) >> 3)

#define B11001111 0xCF
#define B00110000 0x30

#define B11100111 0xE7
#define B00011000 0x18

#define RR_16BIT(i) (((i) & B00110000) >> 4)


uint8_t testCC(struct Register* reg, uint8_t inst){ // 00: NZ, 01: Z, 10: NC, 11: C
    uint8_t cc = CC(inst);
    switch (cc)
    {
    case 0:  // NZ
        return getZflag(reg) == 0;
    case 1:  // Z
        return getZflag(reg) != 0;
    case 2:  // NC
        return getCflag(reg) == 0;
    case 3:  // C
        return getCflag(reg) != 0;

    default:
        ERR("INVALID CC");
    }
}

// @param ext  1=used by extension opcode(CB), 0=used by regular opcode
uint8_t bitRotLeft(struct Register* reg, uint8_t tg_byte, uint8_t c_mode, int ext){
    uint8_t last_bit = BIT_N(tg_byte, 7);
    uint8_t first_bit = (c_mode ? last_bit : getCflag(reg));
    uint8_t result = (tg_byte << 1) + first_bit;
    setFlag(reg, (ext ? ZERO(result) : 0), 0, 0, last_bit);
    return result;
}

// @param ext  1=used by extension opcode(CB), 0=used by regular opcode
uint8_t bitRotRight(struct Register* reg, uint8_t tg_byte, uint8_t c_mode, int ext){
    uint8_t first_bit = BIT_N(tg_byte, 0);
    uint8_t last_bit = (  (c_mode ? first_bit : getCflag(reg)) << 7  );
    uint8_t result = (tg_byte >> 1) + last_bit;
    setFlag(reg, (ext ? ZERO(result) : 0), 0, 0, first_bit);
    return result;
}

uint16_t mem16Incr(struct Register* reg, uint8_t* memory){
    uint8_t alsb, amsb;
    alsb = readMemInc(memory, reg);
    amsb = readMemInc(memory, reg);
    return pairRead(amsb, alsb);
}

void stack16w(struct Register* reg, uint8_t* memory, uint16_t data){
    memoryWrite(memory, --reg->SP, msb(data));
    memoryWrite(memory, --reg->SP, lsb(data));
}

uint16_t stack16r(struct Register* reg, uint8_t* memory){
    uint8_t lsb = memoryRead(memory, reg->SP++);
    uint8_t msb = memoryRead(memory, reg->SP++);
    return pairRead(msb, lsb);
}

void callFunc(struct Register* reg, uint8_t* memory, uint16_t addr){
    stack16w(reg, memory, reg->PC);
    reg->PC = addr;
}

// N: Number of machine cycles that the instruction consumes
// Returns the number of M-cycles that have to be ignored 
#define OP_LEN(N) {return (N - 1);}

int execNormal(struct Register* reg, uint8_t* memory, uint8_t inst){
    int8_t e;

    uint8_t data;
    uint8_t amsb, alsb;
    uint8_t n;
    uint8_t h_flag, c_flag;
    uint8_t result;
    uint8_t adj;
    uint8_t nxt_c_flag;
    uint8_t n_flag;
    uint8_t dlsb, dmsb;
    uint8_t target;
    uint8_t pair;
    uint8_t r;
    uint8_t code;
    uint8_t b;
    uint8_t r1, r2;

    uint16_t addr;
    uint16_t op;
    uint16_t nn;
    uint16_t data16;
    uint16_t hl_val;
    uint16_t rr_val;
    uint16_t result16;

    switch(inst){
    case 0:  // NOP
        // nothing
        OP_LEN(1)
    case 0x76: // HALT
        return HALT_SIGNAL;
    case 0x36: // LD (HL), imm8
        data = readMemInc(memory, reg);
        memoryWrite(memory, register16r(reg, PAIR_HL), data);
        OP_LEN(3)
    case 0x0A: // LD A, (BC)
        reg->A = memoryRead(memory, register16r(reg, PAIR_BC));
        OP_LEN(2)
    case 0x1A: // LD A, (DE)    
        reg->A = memoryRead(memory, register16r(reg, PAIR_DE));
        OP_LEN(2)
    case 0x02: // LD (BC), A
        memoryWrite(memory, register16r(reg, PAIR_BC), reg->A);
        OP_LEN(2)
    case 0x12: // LD (DE), A
        memoryWrite(memory, register16r(reg, PAIR_DE), reg->A);
        OP_LEN(2)
    case 0xFA: // LD A, (nn)
        alsb = readMemInc(memory, reg);
        amsb = readMemInc(memory, reg);
        addr = pairRead(amsb, alsb);
        reg->A = memoryRead(memory, addr);
        //printf("LD A, (nn) %02x\n", addr);
        OP_LEN(4)
    case 0xEA: // LD (nn), A
        alsb = readMemInc(memory, reg);
        amsb = readMemInc(memory, reg);
        addr = pairRead(amsb, alsb);
        memoryWrite(memory, addr, reg->A);
        OP_LEN(4)
    case 0xF2:  // LDH A, (C)
        reg->A = memoryRead(memory, pairRead(0xFF, reg->C));
        OP_LEN(2)
    case 0xE2: // LDH (C), A
        memoryWrite(memory, pairRead(0xFF, reg->C), reg->A);
        OP_LEN(2)
    case 0xF0: // LDH A, (n)
        n = readMemInc(memory, reg);
        data = memoryRead(memory, pairRead(0xFF, n));
        reg->A = data;
        OP_LEN(3)
    case 0xE0: // LDH (n), A
        n = readMemInc(memory, reg);
        memoryWrite(memory, pairRead(0xFF, n), reg->A);
        OP_LEN(3)
    case 0x3A:  // LD A, (HL-)
        addr = register16r(reg, PAIR_HL);
        reg->A = memoryRead(memory, addr);
        register16w(reg, PAIR_HL, addr - 1);
        OP_LEN(2)
    case 0x32: // LD (HL-), A   
        addr = register16r(reg, PAIR_HL);
        memoryWrite(memory, addr, reg->A);
        register16w(reg, PAIR_HL, addr - 1);
        OP_LEN(2)
    case 0x2A: // LD A, (HL+)
        addr = register16r(reg, PAIR_HL);
        reg->A = memoryRead(memory, addr);
        register16w(reg, PAIR_HL, addr + 1);
        OP_LEN(2)
    case 0x22: // LD (HL+), A   
        addr = register16r(reg, PAIR_HL);
        memoryWrite(memory, addr, reg->A);
        register16w(reg, PAIR_HL, addr + 1);
        OP_LEN(2)
    case 0x08: // LD (nn), SP
        alsb = readMemInc(memory, reg);
        amsb = readMemInc(memory, reg);
        addr = pairRead(amsb, alsb);
        memoryWrite(memory, addr, lsb(register16r(reg, PAIR_SP)));
        memoryWrite(memory, addr + 1, msb(register16r(reg, PAIR_SP)));
        OP_LEN(5)
    case 0xF9: // LD SP, HL
        register16w(reg, PAIR_SP, register16r(reg, PAIR_HL));
        OP_LEN(2)
    case 0xF8: // LD HL, SP+e    000001000
        e = readSignedInc(memory, reg);
        op = reg->SP;  // OPerand
        
        h_flag = (((0xF & e) + (0xF & op)) >> 4);
        c_flag = (((0xFF & (uint16_t)e) + (0xFF & op)) >> 8);
        setFlag(reg, 0, 0, h_flag, c_flag);
        register16w(reg, PAIR_HL, op + *(int8_t*)&e);
        OP_LEN(3)
    case 0x86: // ADD (HL)
        data = memoryRead(memory, register16r(reg, PAIR_HL));
        add8bitToA(reg, data, 0);
        OP_LEN(2)
    case 0xC6:  // ADD n
        n = readMemInc(memory, reg);
        add8bitToA(reg, n, 0);
        OP_LEN(2)
    case 0x8E:  // ADC (HL)
        data = memoryRead(memory, register16r(reg, PAIR_HL));
        add8bitToA(reg, data, 1);
        OP_LEN(2)
    case 0xCE: // ADC n
        n = readMemInc(memory, reg);
        add8bitToA(reg, n, 1);
        OP_LEN(2)
    case 0x96: // SUB (HL)
        data = memoryRead(memory, register16r(reg, PAIR_HL));
        sub8bitToA(reg, data, 0);
        OP_LEN(2)
    case 0xD6:  // SUB n
        n = readMemInc(memory, reg);
        sub8bitToA(reg, n, 0);
        OP_LEN(2)
    case 0x9E:  // SBC (HL)
        data = memoryRead(memory, register16r(reg, PAIR_HL));
        sub8bitToA(reg, data, 1);
        OP_LEN(2)
    case 0xDE: // SBC n
        n = readMemInc(memory, reg);
        sub8bitToA(reg, n, 1);
        OP_LEN(2)
    case 0xBE:  // CP (HL)
        data = memoryRead(memory, register16r(reg, PAIR_HL));
        (void)sub8bitSetFlag(reg, reg->A, data, 0);
        OP_LEN(2)
    case 0xFE:  // CP n
        n = readMemInc(memory, reg);
        (void)sub8bitSetFlag(reg, reg->A, n, 0);
        OP_LEN(2)
    case 0x34:  // INC (HL)
        addr = register16r(reg, PAIR_HL);
        data = memoryRead(memory, addr);
        result = incdecSetFlag(reg, data, 0);
        memoryWrite(memory, addr, result);
        OP_LEN(3)
    case 0x35:  // DEC (HL)
        addr = register16r(reg, PAIR_HL);
        data = memoryRead(memory, addr);
        result = incdecSetFlag(reg, data, 1);
        memoryWrite(memory, addr, result);
        OP_LEN(3)
    case 0xA6:  // AND (HL)
        addr = register16r(reg, PAIR_HL);
        data = memoryRead(memory, addr);
        result = (reg->A & data);
        reg->A = result;
        setFlag(reg, ZERO(result), 0, 1, 0);
        OP_LEN(2)
    case 0xE6:  // AND n
        n = readMemInc(memory, reg);
        result = (reg->A & n);
        reg->A = result;
        setFlag(reg, ZERO(result), 0, 1, 0);
        OP_LEN(2)
    case 0xB6:  // OR (HL)
        addr = register16r(reg, PAIR_HL);
        data = memoryRead(memory, addr);
        result = (reg->A | data);
        reg->A = result;
        setFlag(reg, ZERO(result), 0, 0, 0);
        OP_LEN(2)
    case 0xF6:  // OR n
        n = readMemInc(memory, reg);
        result = (reg->A | n);
        reg->A = result;
        setFlag(reg, ZERO(result), 0, 0, 0);
        OP_LEN(2)
    case 0xAE:  // XOR (HL)
        addr = register16r(reg, PAIR_HL);
        data = memoryRead(memory, addr);
        result = (reg->A ^ data);
        reg->A = result;
        setFlag(reg, ZERO(result), 0, 0, 0);
        OP_LEN(2)
    case 0xEE:  // XOR n
        n = readMemInc(memory, reg);
        result = (reg->A ^ n);
        reg->A = result;
        setFlag(reg, ZERO(result), 0, 0, 0);
        OP_LEN(2)
    case 0x3F:  // CCF
        setFlag(reg, getZflag(reg), 0, 0, !getCflag(reg));
        OP_LEN(1)
    case 0x37:  // SCF
        setFlag(reg, getZflag(reg), 0, 0, 1);
        OP_LEN(1)
    case 0x27: // DAA
        adj = 0;
        nxt_c_flag = 0;
        n_flag = getNflag(reg);
        if(getHflag(reg) || (ZERO(n_flag) && (reg->A & 0xF) > 0x9)){
            adj += 0x06;
        }
        if(getCflag(reg) || (ZERO(n_flag) && reg->A > 0x99)){
            adj += 0x60;
            nxt_c_flag = 1;
        }
        if(n_flag){
            reg->A -= adj;
        }
        else{
            reg->A += adj;
        }
        setFlag(reg, ZERO(reg->A), n_flag, 0, nxt_c_flag);
        OP_LEN(1)
    case 0x2F: // CPL
        reg->A = ~reg->A;
        setFlag(reg, getZflag(reg), 1, 1, getCflag(reg));
        OP_LEN(1)
    case 0xE8: // ADD SP, e
        e = readSignedInc(memory, reg);
        op = reg->SP;
        
        h_flag = (((0xF & e) + (0xF & op)) >> 4);  // modularize
        c_flag = (((0xFF & (uint16_t)e) + (0xFF & op)) >> 8);
        
        reg->SP = op + e;
        setFlag(reg, 0, 0, h_flag, c_flag);
        OP_LEN(4)
    case 0x07:  // RLCA
        reg->A = bitRotLeft(reg, reg->A, 1, 0);
        OP_LEN(1)
    case 0x17: // RLA
        reg->A = bitRotLeft(reg, reg->A, 0, 0);
        OP_LEN(1)
    case 0x0F: // RRCA
        reg->A = bitRotRight(reg, reg->A, 1, 0);
        OP_LEN(1)
    case 0x1F: // RRA
        reg->A = bitRotRight(reg, reg->A, 0, 0);
        OP_LEN(1)
    case 0xC3: // JP nn
        reg->PC = mem16Incr(reg, memory);
        OP_LEN(4)
    case 0xE9: // JP (HL)
        reg->PC = register16r(reg, PAIR_HL);
        OP_LEN(1)
    case 0x18:   // JR e
        e = readSignedInc(memory, reg);
        reg->PC = reg->PC + e;
        OP_LEN(3)
    case 0xCD: // CALL nn
        nn = mem16Incr(reg, memory);
        callFunc(reg, memory, nn);
        OP_LEN(6)
    case 0xC9:  // RET   OK
        reg->PC = stack16r(reg, memory);
        OP_LEN(4)
    case 0xD9:  // RETI
        reg->PC = stack16r(reg, memory);
        reg->IME = ENABLE_INTERRUPT;
        //NOT_IMPL("RETI")  // IME THING
        OP_LEN(4)
    case 0xF3:  // DI
        reg->IME = 0;
        OP_LEN(1)
    case 0xFB:  // EI
        reg->IME = ENABLE_INTERRUPT;
        OP_LEN(1)
    default:
        break;
    };

    switch(inst & B11001111){
    case 0x01: // LD rr, nn
        dlsb = readMemInc(memory, reg);
        dmsb = readMemInc(memory, reg);
        target = ((inst & B00110000) >> 4);
        register16w(reg, target, pairRead(dmsb, dlsb));
        OP_LEN(3)
    case 0xC5:  // PUSH rr
        target = to_r16stk[((inst & B00110000) >> 4)];
        data16 = register16r(reg, target);
        stack16w(reg, memory, data16);
        OP_LEN(4)
    case 0xC1:  // POP rr
        target = to_r16stk[((inst & B00110000) >> 4)];
        data16 = stack16r(reg, memory);
        register16w(reg, target, data16);
        OP_LEN(3)
    case 0x03:  // INC rr
        pair = RR_16BIT(inst);
        register16w(reg, pair, register16r(reg, pair) + 1);
        OP_LEN(2)
    case 0x0B:  // DEC rr
        pair = RR_16BIT(inst);
        register16w(reg, pair, register16r(reg, pair) - 1);
        OP_LEN(2)
    case 0x09: // ADD HL, rr
        pair = RR_16BIT(inst);
        hl_val = register16r(reg, PAIR_HL);
        rr_val = register16r(reg, pair);
        result16 = hl_val + rr_val;
        setFlag(reg, getZflag(reg), 0, halfCarry16bitAdd(hl_val, rr_val), carry16bitAdd(hl_val, rr_val));
        register16w(reg, PAIR_HL, result16);
        OP_LEN(2)
    default:
        break;
    };

    switch(inst & B11100111){
    case 0xC2:  // JP cc, nn　　00: NZ, 01: Z, 10: NC, 11: C
        nn = mem16Incr(reg, memory);
        if(testCC(reg, inst)){
            reg->PC = nn;
            OP_LEN(4)
        }
        OP_LEN(3)
    case 0x20:  // JR cc, e
        e = readSignedInc(memory, reg);
        if(testCC(reg, inst)){
            reg->PC = reg->PC + e;
            OP_LEN(3)
        }
        OP_LEN(2)
    case 0xC4:  // CALL cc, nn
        nn = mem16Incr(reg, memory);
        if(testCC(reg, inst)){
            callFunc(reg, memory, nn);
            OP_LEN(6)
        }
        OP_LEN(3)
    case 0xC0:  // RET cc
        if(testCC(reg, inst)){
            reg->PC = stack16r(reg, memory);
            OP_LEN(5)
        }
        OP_LEN(2)
    default:
        break;
    };

    switch(inst & B11000111){
    case 0x46: // LD r, (HL)
        r = MID3(inst);
        register8w(reg, r, memoryRead(memory, register16r(reg, PAIR_HL)));
        OP_LEN(2)
    case 0x06:  // LD r, imm8
        r = MID3(inst);
        data = readMemInc(memory, reg);
        register8w(reg, r, data);
        OP_LEN(2)
    case 0x04:  // INC r
        r = MID3(inst);
        register8w(reg, r, incdecSetFlag(reg, register8r(reg, r), 0));
        OP_LEN(1)
    case 0x05:  // DEC r
        r = MID3(inst);
        result = incdecSetFlag(reg, register8r(reg, r), 1);
        register8w(reg, r, result);
        OP_LEN(1)
    case 0xC7:  // RST n
        code = MID3(inst);
        n = (code >> 1) * 0x10 + (code & 1) * 0x08;
        stack16w(reg, memory, reg->PC);
        reg->PC = pairRead(0x00, n);
        OP_LEN(4)
    default:
        break;
    };

    switch(inst & B11111000){
    case 0x70:  // LD (HL), r
        r = LOW3(inst);
        memoryWrite(memory, register16r(reg, PAIR_HL), register8r(reg, r));
        OP_LEN(2)
    case 0x80: // ADD r
        r = inst & B00000111;
        b = register8r(reg, r);
        add8bitToA(reg, b, 0);
        OP_LEN(1)
    case 0x88: // ADC r
        r = inst & B00000111;
        add8bitToA(reg, register8r(reg, r), 1);
        OP_LEN(1)
    case 0x90: // SUB r
        r = inst & B00000111;
        sub8bitToA(reg, register8r(reg, r), 0);
        OP_LEN(1)
    case 0x98: // SBC r
        r = inst & B00000111;
        sub8bitToA(reg, register8r(reg, r), 1);
        OP_LEN(1)
    case 0xB8: // CP r
        r = inst & B00000111;
        (void)sub8bitSetFlag(reg, reg->A, register8r(reg, r), 0);
        OP_LEN(1)
    case 0xA0:  // AND r
        r = LOW3(inst);
        result = (reg->A & register8r(reg, r));
        reg->A = result;
        setFlag(reg, ZERO(result), 0, 1, 0);
        OP_LEN(1)
    case 0xB0:  // OR r
        r = LOW3(inst);
        result = (reg->A | register8r(reg, r));
        reg->A = result;
        setFlag(reg, ZERO(result), 0, 0, 0);
        OP_LEN(1)
    case 0xA8:  // XOR r
        r = LOW3(inst);
        result = (reg->A ^ register8r(reg, r));
        reg->A = result;
        setFlag(reg, ZERO(result), 0, 0, 0);
        OP_LEN(1)
    default:
        break;
    };

    if(MATCHM(inst, 0x40, B11000000)){  // LD r1, r2; r1=r2
        r1 = MID3(inst);
        r2 = (inst & B00000111);
        register8w(reg, r1, register8r(reg, r2));
        OP_LEN(1)
    }

    ERR("Unknown instruction")
}

// Helper function for ext opcodes
void rotateRegister(struct Register* reg, uint8_t inst, uint8_t left, uint8_t c_mode){
    uint8_t r = (B00000111 & inst);
    uint8_t r_value = register8r(reg, r);
    uint8_t result = (left ? bitRotLeft(reg, r_value, c_mode, 1) : bitRotRight(reg, r_value, c_mode, 1));
    register8w(reg, r, result);
}

void rotateMemoryHL(struct Register* reg, uint8_t* memory, uint8_t left, uint8_t c_mode){
    uint16_t addr = register16r(reg, PAIR_HL);
    uint8_t value = memoryRead(memory, addr);
    uint8_t result = (left ? bitRotLeft(reg, value, c_mode, 1) : bitRotRight(reg, value, c_mode, 1));
    memoryWrite(memory, addr, result);
}

uint8_t shiftByte(struct Register* reg, uint8_t data, uint8_t left, uint8_t logical){
    uint8_t cflag = BIT_N(data, (left ? 7 : 0));
    uint8_t result = (left ? (data << 1) : (data >> 1));
    if((logical == 0) && (left == 0)){
        result += (data & 0x80); // why?
    }
    setFlag(reg, ZERO(result), 0, 0, cflag);
    return result;
}

void shiftReg(struct Register* reg, uint8_t inst, uint8_t left, uint8_t logical){
    uint8_t r = (B00000111 & inst);
    uint8_t data = register8r(reg, r);
    uint8_t result = shiftByte(reg, data, left, logical);
    register8w(reg, r, result);
}

void shiftHL(struct Register* reg, uint8_t* memory, uint8_t left, uint8_t logical){
    uint16_t addr = register16r(reg, PAIR_HL);
    uint8_t data = memoryRead(memory, addr);
    memoryWrite(memory, addr, shiftByte(reg, data, left, logical));
}

uint8_t swapNibbles(struct Register* reg, uint8_t data){
    uint8_t result = (data >> 4) + ((0x0F & data) << 4);
    setFlag(reg, ZERO(result), 0, 0, 0);
    return result;
}

void testBit(struct Register* reg, uint8_t bit, uint8_t data){
    setFlag(reg, ZERO(BIT_N(data, bit)), 0, 1, getCflag(reg));
}

void resetBit(uint8_t bit, uint8_t* data){
    *data = RESET(*data, bit);
}

void setBit(uint8_t bit, uint8_t* data){
    resetBit(bit, data);
    *data += (1 << bit);
}

int execExt(struct Register* reg, uint8_t* memory, uint8_t inst){
    uint8_t result;
    uint8_t r;
    uint8_t b;
    uint8_t data;

    uint16_t addr;

    switch(inst){
    case 0x06: // RLC (HL)
        rotateMemoryHL(reg, memory, 1, 1);
        OP_LEN(4)
    case 0x0E:  // RRC (HL)
        rotateMemoryHL(reg, memory, 0, 1);
        OP_LEN(4)
    case 0x16:  // RL (HL)
        rotateMemoryHL(reg, memory, 1, 0);
        OP_LEN(4)
    case 0x1E:  // RR (HL)
        rotateMemoryHL(reg, memory, 0, 0);
        OP_LEN(4)
    case 0x26:  // SLA (HL)
        shiftHL(reg, memory, 1, 0);
        OP_LEN(4)
    case 0x2E:  // SRA (HL)
        shiftHL(reg, memory, 0, 0);
        OP_LEN(4)
    case 0x36: // SWAP (HL)
        addr = register16r(reg, PAIR_HL);
        result = swapNibbles(reg, memoryRead(memory, addr));
        memoryWrite(memory, addr, result);
        OP_LEN(4)
    case 0x3E: // SRL (HL)
        shiftHL(reg, memory, 0, 1);
        OP_LEN(4)
    case 0x38: // SRL r
        shiftReg(reg, inst, 0, 1);
        OP_LEN(2)
    default:
        break;
    };

    switch(inst & B11111000){
    case 0x00:  // RLC r
        rotateRegister(reg, inst, 1, 1);
        OP_LEN(2)
    case 0x08:  // RRC r
        rotateRegister(reg, inst, 0, 1);
        OP_LEN(2)
    case 0x10:  // RL r
        rotateRegister(reg, inst, 1, 0);
        OP_LEN(2)
    case 0x18:  // RR r
        rotateRegister(reg, inst, 0, 0);
        OP_LEN(2)
    case 0x20:  // SLA r
        shiftReg(reg, inst, 1, 0);
        OP_LEN(2)
    case 0x28:  // SRA r
        shiftReg(reg, inst, 0, 0);
        OP_LEN(2)
    case 0x38:  // SRL r
        shiftReg(reg, inst, 0, 1);
        OP_LEN(2)
    case 0x30: // SWAP r
        r = (B00000111 & inst);
        result = swapNibbles(reg, register8r(reg, r));
        register8w(reg, r, result);
        OP_LEN(2)
    default:
        break;
    };
    
    switch(inst & B11000111){
    case 0x46: // BIT b,(HL)
        b = MID3(inst);
        data = memoryRead(memory, register16r(reg, PAIR_HL));
        testBit(reg, b, data);
        OP_LEN(3)
    case 0x86: // RES b, (HL)
        b = MID3(inst);
        addr = register16r(reg, PAIR_HL);
        data = memoryRead(memory, addr);
        resetBit(b, &data);
        memoryWrite(memory, addr, data);
        OP_LEN(4)
    case 0xC6: // SET b, (HL)
        b = MID3(inst);
        addr = register16r(reg, PAIR_HL);
        data = memoryRead(memory, addr);
        setBit(b, &data);
        memoryWrite(memory, addr, data);
        OP_LEN(4)
    default:
        break;
    };

    switch(inst & B11000000){
    case 0x40: // BIT b,r
        b = MID3(inst);
        r = (B00000111 & inst);
        testBit(reg, b, register8r(reg, r));
        OP_LEN(2)
    case 0x80:  // RES b, r
        b = MID3(inst);
        r = (B00000111 & inst);
        data = register8r(reg, r);
        resetBit(b, &data);
        register8w(reg, r, data);
        OP_LEN(2)
    case 0xC0:  // SET b, r
        b = MID3(inst);
        r = (B00000111 & inst);
        data = register8r(reg, r);
        setBit(b, &data);
        register8w(reg, r, data);
        OP_LEN(2)
    default:
        break;
    };

    printf("[ERROR OPCODE]: 0xCB %02x\n", inst);
    ERR("Unknown instruction (ext)")
}
