#include "chead.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void readData(FILE* p, void* tg, unsigned int pos, size_t cnt);

void readCart(FILE* c_p, struct Header* out){
    readData(c_p, out->entry, 0x100, 4);
    readData(c_p, out->nlogo, 0x104, 48);
    readData(c_p, out->title, 0x134, 16);
    readData(c_p, &out->ctype, 0x147, 1);
    readData(c_p, &out->romSize, 0x148, 1);
    readData(c_p, &out->ramSize, 0x149, 1);
    readData(c_p, &out->checksum, 0x14D, 1);
    size_t rom_bytes = 1024 * 32 * (1 << out->romSize);
    out->data = malloc(rom_bytes);
    readData(c_p, out->data, 0, rom_bytes);
}

uint8_t checkSum(FILE* c_p){
    uint8_t out = 0;
    uint8_t rom[25];
    size_t len = sizeof(rom)/sizeof(uint8_t);
    fseek(c_p, 0x134, SEEK_SET);
    fread(rom, 1, len, c_p);
    for(size_t i = 0; i<len; ++i){
        out -= rom[i] + 1;
    }
    return out;
}

void testCart(FILE* rom_ptr, struct Header* head_ptr){
    printf("GAME: %s\n", head_ptr->title);

    printf("=== NINTENDO LOGO ===\n");
    for(size_t i = 0; i < 3; ++i){
        for(size_t j = 0; j < 16; ++j){
            printf("%02x ", head_ptr->nlogo[16*i + j]); 
        }
        printf("\n");
    }
    printf("=====================\n");

    if(checkSum(rom_ptr) == head_ptr->checksum){
        printf("CHECKSUM PASSED\n");
    }
    else{
        printf("CHECKSUM FAILED\n");
    }
}


void readData(FILE* p, void* tg, unsigned int pos, size_t cnt){
    fseek(p, pos, SEEK_SET);
    fread(tg, 1, cnt, p);
}


