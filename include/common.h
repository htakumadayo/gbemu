#ifndef EMU_COMMON_H
#define EMU_FOMMON_H 1
#include <stdio.h>
#include <stdint.h>

//typedef unsigned char uint8_t;

#define ERR(msg) {fprintf(stderr, "ERROR: %s\n", msg); exit(-1);}
#define NOT_IMPL {fprintf(stderr, "NOT YET IMPLEMENTED\n"); exit(-2);}

#endif 
