#ifndef EMU_COMMON_H
#define EMU_FOMMON_H 1


#define GB_WIDTH 160
#define GB_HEIGHT 144


#define LCD_QUIT -1;


#define DOTS_PER_SCANLINE 456
#define TILE_DIM_POW 3  // 8 pixel = 2^3 pixel
#define TILE_DIM (1 << TILE_DIM_POW)

#define START_OAM 0xFE00
#define END_OAM 0xFE9F
#define OBJECT_LENGTH 4
#define MAX_OBJ_PER_LINE 10

#define OAM_YPOS 0
#define OAM_XPOS 1
#define OAM_TILEIDX 2
#define OAM_ATTR 3

#define OAM_ATTR_PRIORITY 7
#define OAM_ATTR_YFLIP    6
#define OAM_ATTR_XFLIP    5
#define OAM_ATTR_PALETTE  4
#define OAM_ATTR_BANK     3   // Only CGB so should not be used

#define LCDC_PPU_ENABLE 7
#define LCDC_WINDOW_TILEMAP 6
#define LCDC_WINDOW_ENABLE 5
#define LCDC_BGWINDOW_TILES 4
#define LCDC_BG_TILEMAP 3
#define LCDC_OBJ_SIZE 2
#define LCDC_OBJ_ENABLE 1
#define LCDC_BGWINDOW_ENABLE 0



extern const uint32_t PALETTE_COLOR[]; 

struct LCD;

struct LCD* initLCD();
void beginFrame(struct LCD* lcd);
void process4Dots(struct LCD* lcd, uint8_t* memory);
void updateLCD(struct LCD* lcd);
int LCDevent(struct LCD* lcd);
void destroyLCD(struct LCD* lcd);

// Pauses the program
void sleep(uint64_t ns);
// Returns the current time in nanoseconds
uint64_t curtime();


#endif