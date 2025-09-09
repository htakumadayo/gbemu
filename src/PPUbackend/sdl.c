#include <SDL3/SDL.h>
#include <stdlib.h>
#include "ppu.h"
#include "common.h"
#include "registers.h"


const uint32_t PALETTE_COLOR[] = {
    0xFFFFFFFF,
    0xFFAAAAAA,
    0xFF444444,
    0xFF000000
};

struct LCD{
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture* texture;

    uint64_t last_frame_time;
    int title_update_timer;
    uint64_t time_tot;

    void* pixels;
    int pitch;

    uint32_t dot;
};


struct LCD* initLCD(){
    struct LCD* p_ppu = malloc(sizeof(struct LCD));

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return NULL;
    }

    if (!SDL_CreateWindowAndRenderer("GB EMULATOR", GB_WIDTH, GB_HEIGHT, SDL_WINDOW_RESIZABLE, &p_ppu->window, &p_ppu->renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return NULL;
    }

    p_ppu->texture = SDL_CreateTexture(p_ppu->renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, GB_WIDTH, GB_HEIGHT);
    SDL_SetTextureScaleMode(p_ppu->texture, SDL_SCALEMODE_NEAREST);

    p_ppu->last_frame_time = 0;
    p_ppu->title_update_timer = 0;
    p_ppu->time_tot = 0;

    return p_ppu;
}

void beginFrame(struct LCD* lcd){
    if(SDL_LockTexture(lcd->texture, NULL, &lcd->pixels, &lcd->pitch) == 0){
        ERR("FAILED TO LOCK TEXTURE");
    }

    lcd->dot = 0;
    
    // Texture locked.
}

// TODO: Implement STAT

void process4Dots(struct LCD* lcd, uint8_t* memory){
    const uint8_t LCDC = memory[ADDR_LCDC];
    uint16_t mode = lcd->dot % DOTS_PER_SCANLINE;
    uint16_t line = lcd->dot / DOTS_PER_SCANLINE;
    memory[ADDR_LY] = line;
    const uint16_t
        mode2 = 80,
        mode3 = 172,
        mode0 = 204;

    // Vblank if line >= 144
    if(line >= 144){
        lcd->dot += 4;
        return;
    }

    if(mode < mode2){   // Mode 2: OAM scan
        // pass
    }
    else if(mode < mode2 + mode3){  // Mode 3: Drawing pixels (Penalties ignored)
        uint16_t scrY = line;
        uint16_t scrX = mode - mode2;

        const uint8_t SCX = memory[ADDR_SCX];
        const uint8_t SCY = memory[ADDR_SCY];
        const uint8_t palette = memory[ADDR_BGP];
        uint16_t bgY = (scrY + SCY) % 256;

        uint16_t bg_tiles_offset = (BIT_N(LCDC, 3) ? 0x9C00 : 0x9800);  // 0: 9800-9BFF, 1: 9C00-9FFF
        int tile_area_mode = BIT_N(LCDC, 4);

        for(uint8_t i=0; i<4; ++i){
            // Draw background
            uint16_t bgX = (scrX + SCX) % 256;

            uint16_t tile_id = (bgX / TILE_DIM) + 32 * (bgY / TILE_DIM);
            uint8_t tile_addr = memory[tile_id + bg_tiles_offset];
            uint16_t tile_addr_on_ram;
            if(tile_area_mode){  // $8000
                tile_addr_on_ram = 0x8000 + 16*tile_addr;  // 16 bytes per tile
            }
            else{  // $ 9000 + signed
                tile_addr_on_ram = 0x9000 + *((int8_t*)&tile_addr) * 16;
            }
            
            uint16_t pixelX = 7 - (bgX & 0x07); // In fact, bit 7 is the leftmost pixel
            uint16_t pixelY = bgY & 0x07;
            uint16_t ram_line_byte_addr = tile_addr_on_ram + pixelY*2;
        

            uint8_t color_id = BIT_N(memory[ram_line_byte_addr], pixelX) + 0x02 * BIT_N(memory[ram_line_byte_addr + 1], pixelX);
            uint32_t color = PALETTE_COLOR[(palette >> (2*color_id)) & 0x03];
            uint32_t* pixel_ptr = (uint32_t*)((uint8_t*)lcd->pixels + scrY*lcd->pitch) + scrX;
            *pixel_ptr = color;

            ++scrX;
        }
        

    }
    else if(mode < mode2 + mode3 + mode0){  // Mode 0: Hblank
        // pass
    }

    lcd->dot += 4;
}

#define TITLE_LEN 7
#define TITLE_UPDATE_FREQ 60  // Once every 60 frame

void updateLCD(struct LCD* lcd){
    SDL_SetRenderDrawColor(lcd->renderer, 100, 100, 100, 255);
    SDL_RenderClear(lcd->renderer);   // Is this really needed?

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


    // Unlock texture and update screen
    SDL_UnlockTexture(lcd->texture);
    SDL_RenderTexture(lcd->renderer, lcd->texture, NULL, NULL);
    SDL_RenderPresent(lcd->renderer);
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
