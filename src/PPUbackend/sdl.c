#include <SDL3/SDL.h>
#include "ppu.h"
#include <stdlib.h>

struct LCD{
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture* texture;

    uint64_t last_frame_time;
    int title_update_timer;
    uint64_t time_tot;
};


struct LCD* initLCD(){
    struct LCD* p_ppu = malloc(sizeof(struct LCD));

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return NULL;
    }

    if (!SDL_CreateWindowAndRenderer("examples/renderer/clear", GB_WIDTH, GB_HEIGHT, SDL_WINDOW_RESIZABLE, &p_ppu->window, &p_ppu->renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return NULL;
    }

    p_ppu->texture = SDL_CreateTexture(p_ppu->renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, GB_WIDTH, GB_HEIGHT);

    p_ppu->last_frame_time = 0;
    p_ppu->title_update_timer = 0;
    p_ppu->time_tot = 0;

    return p_ppu;
}

#define TITLE_LEN 7
#define TITLE_UPDATE_FREQ 60  // Once every 60 frame

void updateLCD(struct LCD* lcd){
    SDL_SetRenderDrawColor(lcd->renderer, 100, 100, 100, 255);
    SDL_RenderClear(lcd->renderer);
    SDL_RenderPresent(lcd->renderer);

    uint64_t now = curtime();
    uint64_t dt = now - lcd->last_frame_time;  // in nanoseconds
    lcd->time_tot += dt;
    lcd->last_frame_time = curtime();
    
    if(lcd->title_update_timer < 0){
        uint64_t avg = (double)lcd->time_tot / TITLE_UPDATE_FREQ;
        lcd->time_tot = 0;

        int fps = (double)(1e9) / avg;
        char title[TITLE_LEN] = "";
        snprintf(title, TITLE_LEN, "%02d FPS", fps);
        SDL_SetWindowTitle(lcd->window, title);

        lcd->title_update_timer = TITLE_UPDATE_FREQ;
    }
    --lcd->title_update_timer;
}

int LCDevent(struct LCD* lcd){
    SDL_Event ev;
    while(SDL_PollEvent(&ev)){
        if(ev.type == SDL_EVENT_QUIT){
            return LCD_QUIT;
        }
    }

    return 0;
}


void destroyLCD(struct LCD* lcd){
    SDL_DestroyWindow(lcd->window);
    SDL_DestroyRenderer(lcd->renderer);
}


void sleep(uint64_t ns){
    SDL_DelayNS(ns);
}

uint64_t curtime(){
    return SDL_GetTicksNS();
}
