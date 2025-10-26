#include <SDL3/SDL.h>
#include <stdlib.h>
#include "SM83.h"
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

    uint8_t oam_scan_done;
    uint16_t render_obj_list[MAX_OBJ_PER_LINE];
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

// Return real on-screen coordinates of the object
#define OBJX(memory, obj_address) ( ((int16_t)(memory[obj_address + 1])) - 8 )
#define OBJY(memory, obj_address) ( ((int16_t)(memory[obj_address])) - 16    )

uint8_t getPixelColorIDX(uint8_t* memory, uint16_t raw_tile_idx, uint8_t mode8000, uint8_t on_tile_x, uint8_t on_tile_y){
    uint16_t tile_addr_on_ram;
    if(mode8000){  // $8000 + unsigned
        tile_addr_on_ram = 0x8000 + 16*raw_tile_idx;  // 16 bytes per tile
    }
    else{  // $ 9000 + signed
        tile_addr_on_ram = 0x9000 + *((int8_t*)&raw_tile_idx) * 16;
    }

    // Memory address of the pixel line. The 2 factor accounts the way pixels are stored
    uint16_t ram_line_byte_addr = tile_addr_on_ram + on_tile_y*2;

    // Fetch color id (which will be converted to color using palette)
    uint8_t color_id = BIT_N(memory[ram_line_byte_addr], on_tile_x) + 0x02*BIT_N(memory[ram_line_byte_addr + 1], on_tile_x);
    return color_id;
}

uint8_t getColorFromPalette(uint8_t palette, uint8_t color_id){
    // 2 bits per color, mask out unneeded parts
    return (palette >> (2*color_id)) & 0x03;
}

// Return the color ID of the pixel. This is not color index used for palettes. 
// If BG pixel should be drawn instead, returns 255.
uint8_t getObjPixel(struct LCD* lcd, uint8_t* memory, uint8_t scrX, uint8_t scrY, uint8_t double_height_mode){
    // BG over OBJ (Bit 7) -> Implement later
    // Ignore attributes except palette (Bit 4) and xflip (Bit 5) for now.

    // Priority of OPAQUE pixels:
    // 1. X coordinate: object with smaller x coordinate wins
    // 2. Object with earlier appearence in OAM wins
    uint8_t i = 0;
    const int16_t default_minx_value = 9999;
    int16_t cur_min_x = default_minx_value;  // Random big value
    uint8_t out = 0;  // Output Color ID
    uint8_t palettes[2] = {memory[ADDR_OBP0], memory[ADDR_OBP1]};

    while(i < MAX_OBJ_PER_LINE && lcd->render_obj_list[i] != 0){
        uint16_t obj_addr = lcd->render_obj_list[i];
        int16_t obj_x = OBJX(memory, obj_addr);
        int16_t obj_y = OBJY(memory, obj_addr);
        uint8_t tile_idx = memory[obj_addr + OAM_TILEIDX];
        uint8_t attributes = memory[obj_addr + OAM_ATTR];

        uint8_t palette_id = BIT_N(attributes, OAM_ATTR_PALETTE);
        int second_tile = (scrY - obj_y) / TILE_DIM; // 0: first tile, 1: second tile
        int on_tile_x = scrX - obj_x;
        int on_tile_y = (scrY - obj_y) % TILE_DIM;

        // Skip if pixel not contained in the object
        if(0 <= on_tile_x && on_tile_x < TILE_DIM){
            if(!BIT_N(attributes, OAM_ATTR_XFLIP)){
                on_tile_x = (TILE_DIM - 1) - on_tile_x;
            }

            // Fetch palette color id
            uint16_t raw_tile_idx = (double_height_mode ? ((tile_idx & 0xFE) + second_tile) : tile_idx);
            uint8_t color_id = getPixelColorIDX(memory, raw_tile_idx, 1, on_tile_x, on_tile_y);
    
            if(obj_x < cur_min_x && color_id != 0){
                out = getColorFromPalette(palettes[palette_id], color_id);
                cur_min_x = obj_x;
            } 
        }
        
        ++i;
    }


    if(cur_min_x < default_minx_value){
        return out;
    }
    else{
        return 255;  // See function description
    }
}

#define MODE0 0
#define MODE1 1
#define MODE2 2
#define MODE3 3
#define UPDATE_STAT_MODE(memory, mode_n) {memory[ADDR_STAT] = (0xFC&memory[ADDR_STAT]) + (mode_n);}

#define STAT_LYC 6
#define STAT_MODE2 5
#define STAT_MODE1 4
#define STAT_MODE0 3
#define STAT_LYCeqLY 2

void process4Dots(struct LCD* lcd, uint8_t* memory){
    const uint8_t LCDC = memory[ADDR_LCDC];
    uint16_t mode = lcd->dot % DOTS_PER_SCANLINE;
    uint16_t line = lcd->dot / DOTS_PER_SCANLINE;
    memory[ADDR_LY] = line;

    uint8_t stat = memory[ADDR_STAT];
    uint8_t LYC_condition = (line == memory[ADDR_LYC]);
    if(LYC_condition){
        DIRECT_SET(memory[ADDR_STAT], STAT_LYCeqLY);
        if(BIT_N(stat, STAT_LYC)){
            setInterruptFlag(memory, INT_STAT);
        }
    }
    else{
        DIRECT_RESET(memory[ADDR_STAT], STAT_LYCeqLY);
    }
    

    const uint16_t
        mode2 = 80,
        mode3 = 172,
        mode0 = 204;

    
    // Mode 1: Vblank if line >= 144
    if(line >= 144){
        UPDATE_STAT_MODE(memory, MODE1);
        // Request vblank
        if(line == 144){
            setInterruptFlag(memory, INT_VBLANK);

            if(BIT_N(stat, STAT_MODE1)){
                setInterruptFlag(memory, INT_STAT);
            }
        }

        uint8_t* DMA_source_addr = &memory[ADDR_DMA];
        if(*DMA_source_addr != 0xFF){
            memcpy(memory + 0xFE00, memory + ((uint16_t)*DMA_source_addr << 8), 0xA0);
            *DMA_source_addr = 0xFF;
        }
        lcd->dot += 4;
        return;
    }
    
    // Object dimension
    uint8_t double_height = BIT_N(LCDC, LCDC_OBJ_SIZE);  // 0: 8x8, 1: 8x16

    if(mode < mode2 && !lcd->oam_scan_done){   // Mode 2: OAM scan
        UPDATE_STAT_MODE(memory, MODE2);
        if(BIT_N(stat, STAT_MODE2)){  // Note: we can use lcd->oam_scan_done to detect whether it just entered mode2 or not
            setInterruptFlag(memory, INT_STAT);
        }
        // Scan what objects will be drawn on this line (up to 10, sequentially from $FE00 to $FE9F)
        // Even offscreen (in terms of X) will be counted
        uint16_t check_addr = START_OAM;
        uint8_t obj_count = 0;
        uint8_t object_height = TILE_DIM*(1+double_height);
        while(check_addr <= END_OAM && obj_count < MAX_OBJ_PER_LINE){
            int16_t Y_pos = OBJY(memory, check_addr);  // c.f. defn of Y
            if(Y_pos <= line && line < (Y_pos + object_height)){
                lcd->render_obj_list[obj_count] = check_addr;
                ++obj_count;
            }

            check_addr += OBJECT_LENGTH;
        }

        // Mark the end if there are less than 10 objects to draw on that line.
        if(obj_count < MAX_OBJ_PER_LINE){
            lcd->render_obj_list[obj_count] = 0;  
        }
        lcd->oam_scan_done = 1;

        // BG over OBJ behaviour: PPU decides what OBJECT-pixel to draw THEN consider the flag. 
        // This means lower priority object's "BG over OBJ" flag is masked by higher priority object's.
    }
    else if(mode < mode2 && lcd->oam_scan_done){
        UPDATE_STAT_MODE(memory, MODE2);
    }
    else if(mode < mode2 + mode3){  // Mode 3: Drawing pixels (Penalties ignored)
        UPDATE_STAT_MODE(memory, MODE3);
        
        // Current drawing pixel
        uint16_t scrY = line;
        uint16_t scrX = mode - mode2;

        // Register values
        const uint8_t SCX = memory[ADDR_SCX];
        const uint8_t SCY = memory[ADDR_SCY];
        const uint8_t palette = memory[ADDR_BGP];
        const uint8_t WX = memory[ADDR_WX];
        const uint8_t WY = memory[ADDR_WY];
        const uint8_t window_enabled = BIT_N(LCDC, 5);

        // bgY: the pixel y-position on the tilemap
        //uint16_t bgY = (scrY + SCY) % 256;

        // Offset specifying which tilemap is used for the background. 0: 9800-9BFF, 1: 9C00-9FFF
        #define OFFSET(BIT) (BIT_N(LCDC, (BIT)) ? 0x9C00 : 0x9800)
        uint16_t bg_tiles_offset = OFFSET(LCDC_BG_TILEMAP);
        uint16_t window_tiles_offset = OFFSET(LCDC_WINDOW_TILEMAP);

        // Addressing mode for BG and window tiles. 0: $8800/signed address, 1: $8000/unsigned
        int tile_addr_mode = BIT_N(LCDC, LCDC_BGWINDOW_TILES);

        // Window top-left corner coordinates on screen
        int16_t window_posx = (int16_t)WX - 7;
        int16_t window_posy = WY;
        uint32_t* pixel_base_ptr = (uint32_t*)((uint8_t*)lcd->pixels + scrY*lcd->pitch);

        for(uint8_t _=0; _<4; ++_){
            // Draw object
            // Search what object to draw

            // Draw background/window
            // If background is covered by window
            uint8_t color_id;
            uint8_t obj_pixel = getObjPixel(lcd, memory, scrX, scrY, double_height);
            if(obj_pixel != 255){
                color_id = obj_pixel;
            }
            else{
                uint8_t draw_window = (window_enabled && scrX >= window_posx && scrY >= window_posy);
                uint16_t tilemap_x, tilemap_y;  // Pixel position on tilemap
                if(draw_window){
                    tilemap_x = (scrX - window_posx) % 256;
                    tilemap_y = (scrY - window_posy) % 256;
                }
                else{
                    tilemap_x = (scrX + SCX) % 256;
                    tilemap_y = (scrY + SCY) % 256;
                }

                // Pixel coordinates on tile
                uint16_t pixelX = 7 - (tilemap_x & 0x07); // In fact, bit 7 is the leftmost pixel
                uint16_t pixelY = tilemap_y & 0x07;

                // Tile id that the pixel belongs to
                uint16_t tile_id = (tilemap_x >> TILE_DIM_POW) + 32 * (tilemap_y >> TILE_DIM_POW);

                // Below: Fetch address of the tile
                uint16_t tiles_offset = (draw_window ? window_tiles_offset : bg_tiles_offset);
                uint8_t tile_addr = memory[tile_id + tiles_offset]; // "Raw" address given by the tilemap. Still need to interpret it according to addressing mode
                
                color_id = getColorFromPalette(palette, getPixelColorIDX(memory, tile_addr, tile_addr_mode, pixelX, pixelY));
            }
            // 2 bits per color, mask out unneeded parts
            uint32_t color = PALETTE_COLOR[color_id];
            // Get pixel pointer of the actual display (ChatGPT)
            uint32_t* pixel_ptr = pixel_base_ptr + scrX;
            *pixel_ptr = color;

            ++scrX;
        }
    }
    else if(mode < mode2 + mode3 + mode0){  // Mode 0: Hblank
        UPDATE_STAT_MODE(memory, MODE0);
        if(mode == mode2 + mode3 && BIT_N(stat, STAT_MODE0)){  // i.e. it just entered mode0
            setInterruptFlag(memory, INT_STAT);
        }
        if(lcd->oam_scan_done){
            lcd->oam_scan_done = 0;
        }
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
