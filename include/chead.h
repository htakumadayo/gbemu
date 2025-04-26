#ifndef EMU_CHEAD_H
#define EMU_CHEAD_H

#include "common.h"


struct Header{
    uint8_t*    data;
    uint8_t     entry[4];
    uint8_t     nlogo[48];
    char        title[16];
    uint8_t     ctype;
    uint8_t     romSize;
    uint8_t     ramSize;
    uint8_t     checksum;
};

void readCart(FILE* cart_ptr, struct Header* out);
uint8_t checkSum(FILE* cart_ptr);
void testCart(FILE* rom_ptr, struct Header* header);


#endif
