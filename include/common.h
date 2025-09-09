#ifndef EMU_COMMON_H
#define EMU_FOMMON_H 1
#include <stdio.h>
#include <stdint.h>

//typedef unsigned char uint8_t;

#define ERR(msg) {fprintf(stderr, "ERROR: %s\n", msg); exit(-1);}
#define NOT_IMPL(what) {fprintf(stderr, "NOT YET IMPLEMENTED: %s\n", what); exit(-2);}

#define MAX(a, b) ((b) > (a) ? (b) : (a))

#define BIT_N(target, N) ((target & (1 << N)) >> N)
#define SET(target, N) ((1 << (N)) | (target))
#define RESET(target, N) (     (~(1 << (N))) & (target)    )

#endif 
