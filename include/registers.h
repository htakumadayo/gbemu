#ifndef EMU_REGISTERS
#define EMU_REGISTERS

// INTERRUPT ADDR
#define ADDR_IE 0xFFFF
#define ADDR_IF 0xFF0F

#define ADDR_SB 0xFF01
#define ADDR_SC 0xFF02

// Timer
#define ADDR_DIV  0xFF04
#define ADDR_TIMA 0xFF05
#define ADDR_TMA  0xFF06
#define ADDR_TAC  0xFF07

// PPU
#define ADDR_LCDC 0xFF40
#define ADDR_LY 0xFF44 // Read-only
#define ADDR_LYC  0xFF45 // LY compare
#define ADDR_STAT 0xFF41
#define ADDR_SCY  0xFF42
#define ADDR_SCX  0xFF43
#define ADDR_WY   0xFF4A
#define ADDR_WX   0xFF4B
#define ADDR_DMA  0xFF46
#define ADDR_BGP  0xFF47
#define ADDR_OBP0 0xFF48
#define ADDR_OBP1 0xFF49

// Joypad input
#define ADDR_JOYP 0xFF00




#endif