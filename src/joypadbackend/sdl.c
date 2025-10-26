#include <SDL3/SDL_keyboard.h>
#include <stdlib.h>
#include "registers.h"
#include "joypad.h"
#include "SM83.h"


#define BUTTON_START  SDL_SCANCODE_V
#define BUTTON_SELECT SDL_SCANCODE_C
#define BUTTON_B      SDL_SCANCODE_Z
#define BUTTON_A      SDL_SCANCODE_X
#define BUTTON_UP     SDL_SCANCODE_UP
#define BUTTON_DOWN   SDL_SCANCODE_DOWN
#define BUTTON_LEFT   SDL_SCANCODE_LEFT
#define BUTTON_RIGHT  SDL_SCANCODE_RIGHT


struct Joypad{
    const bool* sdl_key_states;
};

struct Joypad* initJoypad(){
    struct Joypad* joypad = malloc(sizeof(struct Joypad));
    joypad->sdl_key_states = SDL_GetKeyboardState(NULL);
    return joypad;
}

void updateJoypadRegister(struct Joypad* joypad, uint8_t* memory){
    uint8_t d_pad_status;
    uint8_t buttons_status;
    uint8_t* joyp_register = memory + ADDR_JOYP;
    uint8_t joyp_register_value = *joyp_register;
    const bool* states = joypad->sdl_key_states;

    // 1 and 0 are inverted at this stage. 1 should mean released and 0 pressed
    d_pad_status = ((1 << 3) * states[BUTTON_DOWN]) + ((1 << 2) * states[BUTTON_UP]) +
                    ((1 << 1) * states[BUTTON_LEFT]) + ((1 << 0) * states[BUTTON_RIGHT]);

    buttons_status = ((1 << 3) * states[BUTTON_START]) + ((1 << 2) * states[BUTTON_SELECT]) +
                    ((1 << 1) * states[BUTTON_B]) + ((1 << 0) * states[BUTTON_A]);

    d_pad_status = ~d_pad_status;
    buttons_status = ~buttons_status;
    
    uint8_t d_pad_disabled = BIT_N(joyp_register_value, JOYP_SELECT_DPAD);
    uint8_t buttons_disabled = BIT_N(joyp_register_value, JOYP_SELECT_BUTTONS);

    #define APPLY_SELECT(status, disabled) (   ((disabled)*0xFF) | (status)  )

    uint8_t result = APPLY_SELECT(d_pad_status, d_pad_disabled) & APPLY_SELECT(buttons_status, buttons_disabled);
    *joyp_register = (joyp_register_value & 0xF0) + (result & 0x0F);

    // Now request interrupt
    // If any of the bits 0~3 went from high to low
    uint8_t high2low = (0xF&joyp_register_value) & ( ~(0xF&result) );
    if(high2low != 0){
        DIRECT_SET(memory[ADDR_IF], INT_JOYPAD);
    }
}

