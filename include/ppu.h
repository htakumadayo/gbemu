#ifndef EMU_COMMON_H
#define EMU_FOMMON_H 1


#define GB_WIDTH 160
#define GB_HEIGHT 144


#define LCD_QUIT -1;


#define DOTS_PER_SCANLINE 456
#define TILE_DIM 8

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