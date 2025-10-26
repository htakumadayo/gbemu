#ifndef EMU_COMMON_H
#define EMU_FOMMON_H 1
#include <stdio.h>
#include <stdint.h>

//typedef unsigned char uint8_t;

#define ERR(msg) {fprintf(stderr, "ERROR: %s\n", msg); exit(-1);}
#define NOT_IMPL(what) {fprintf(stderr, "NOT YET IMPLEMENTED: %s\n", what); exit(-2);}

#define MAX(a, b) ((b) > (a) ? (b) : (a))

// Returns the N-th bit of byte target
#define BIT_N(target, N) ((target & (1 << N)) >> N)
// Set the N-th bit of byte target to 1 and return
#define SET(target, N) ((1 << (N)) | (target))
// Set the N-th bit of byte target to 0 and return
#define RESET(target, N) (     (~(1 << (N))) & (target)    )

// Match with mask applied: check if the masked part of target is equal to cp.
#define MATCHM(target, cp, mask) (((target) & (mask)) == (cp))

// Same as SET but instead of returning, directly modify the target byte
#define DIRECT_SET(byte, bit_n) {(byte) = (   SET((byte), (bit_n))   );}
// Same as RESET but instead of returning, directly modify the target byte
#define DIRECT_RESET(byte, bit_n) {(byte) = (   RESET((byte), (bit_n))   );}

#endif 
