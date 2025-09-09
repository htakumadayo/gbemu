#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "chead.h"
#include "SM83.h"
#include "ppu.h"
#include "serial.h"
#include "timer.h"


void execute(size_t rom_bytes, uint8_t* rom_data);

int main(int argc, char* argv[]){
    if(argc < 2){
        printf("The program expects a path to a gb rom.\n");
        return 1;
    }

    char* rom_path = argv[1];
    FILE* rom_ptr = fopen(rom_path, "rb");
    if(rom_ptr == NULL){
        printf("The specified rom does not exist.\n");
        return 1;
    }

    struct Header header;
    size_t rom_bytes = readCart(rom_ptr, &header);
    testCart(rom_ptr, &header);

    execute(rom_bytes, header.data);

    fclose(rom_ptr);
    free(header.data);
    printf(">> EMULATOR TERMINATED <<\n");
    return 0;
}



void execute(size_t rom_bytes, uint8_t* rom_data){
    uint8_t memory[MEMORY_SIZE];
    memcpy(memory, rom_data, rom_bytes);

    // Later dynamically allocate this
    struct SM83 cpu;
    initCPU(&cpu);

#ifdef CPU_DEBUG
    uint16_t start;
    printf("Memory debug start byte:"); (void)scanf("%hd", &start);
    fflush(stdin);

    FILE* log_file = fopen("regdump.txt", "w");
    const size_t buffer_size = 85;
    char* string = malloc(buffer_size);
#endif

    struct Timer timer;
    initTimer(&timer);

    struct LCD* p_lcd = initLCD();

    int user_quit = 0;
    int cpu_idle_cycle = 0;
    const int MCYCLES_PER_FRAME = (CLOCKF / 60);
    const uint64_t FRAME_PERIOD = (1.0 / 60.0) * 1e9;

    while(cpu_idle_cycle != -1 && user_quit != -1){   // 60 FPS loop
        uint64_t t1 = curtime();
        beginFrame(p_lcd);
        
        for(uint32_t i=0; i < MCYCLES_PER_FRAME; ++i){  // MCYCLES-loop for that frame
            tickTimer(memory, &timer);
            --cpu_idle_cycle;
            if(cpu_idle_cycle == -1){
                cpu_idle_cycle = step(&cpu, memory);
                // DEBUG thing
#ifdef CPU_DEBUG
                struct Register* reg = &cpu.reg;
                uint16_t pc = reg->PC;
                int res = snprintf(string, buffer_size, "A: %02x F: %02x B: %02x C: %02x D: %02x E: %02x H: %02x L: %02x SP: %04x PC: 00:%04x (%02x %02x %02x %02x)\n", reg->A, reg->F, reg->B, reg->C, reg->D, reg->E, reg->H, reg->L, reg->SP, reg->PC, memory[pc], memory[pc+1], memory[pc+2], memory[pc+3]);
                if(res < 0){
                    printf("Log failed\n");
                }
                //fputs(string, log_file);
                
                //debugRegister(&cpu.reg);
                //debugMemory(memory, start);
                //printf("Return: %d\n", cpu_idle_cycle);
                //(void)scanf("%d", &_);
#endif
            }

            // Serial output
            processSerialTransfer(memory, stdout);

            // PPU 4 DOTS HERE
            /*
            Draw to dummy array -> upload to gpu on updateLCD()
            */
            process4Dots(p_lcd, memory);

        }  // END of MCYCLES-loop

        user_quit = LCDevent(p_lcd);
        updateLCD(p_lcd);
        int64_t sleep_time = (int64_t)FRAME_PERIOD - (int64_t)(curtime() - t1);

        sleep(MAX(sleep_time, 0));
    } // END of 60 FPS loop

#ifdef CPU_DEBUG
    fclose(log_file);
    free(string);
#endif

    destroyLCD(p_lcd);
    free(p_lcd);
}


