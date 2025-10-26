#ifndef EMU_JOYPAD
#define EMU_JOYPAD

#include "common.h"

#define JOYP_SELECT_BUTTONS 5
#define JOYP_SELECT_DPAD    4
#define JOYP_STARTDOWN      3
#define JOYP_SELECTUP       2
#define JOYP_BLEFT          1
#define JOYP_ARIGHT         0

struct Joypad;

struct Joypad* initJoypad();
void updateJoypadRegister(struct Joypad* joypad, uint8_t* memory);

#endif