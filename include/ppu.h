#ifndef EMU_COMMON_H
#define EMU_FOMMON_H 1


#define GB_WIDTH 160
#define GB_HEIGHT 144


#define LCD_QUIT -1;

struct LCD;

struct LCD* initLCD();
void updateLCD(struct LCD* lcd);
int LCDevent(struct LCD* lcd);
void destroyLCD(struct LCD* lcd);

// Pauses the program
void sleep(uint64_t ns);
// Returns the current time in nanoseconds
uint64_t curtime();


#endif