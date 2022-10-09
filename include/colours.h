//#ifndef COLOURS__INCLUDED
//#define COLOURS__INCLUDED

#undef MENU_C_MAX
#define MENU_C_MAX 20

#if defined(TFT_SSOLED) or defined(TFT_ST7789_T3)
    #ifdef C_WHITE
        #undef C_WHITE
    #endif
    #ifdef BLACK
        #undef BLACK
    #endif
    #ifdef RED
        #undef RED
    #endif
    #ifdef GREEN
        #undef GREEN
    #endif
    #ifdef BLUE
        #undef BLUE
    #endif
    #ifdef YELLOW
        #undef YELLOW
    #endif
#endif

#ifdef TFT_SSOLED
    #define C_WHITE     0xFF
    #define BLACK       0x00
    #define RED         0xFA
    #define GREEN       0xAF
    #define BLUE        0x00
    #define YELLOW      0xF0
    
    #undef MENU_C_MAX
    #define MENU_C_MAX  20
#endif

#ifdef TFT_ST7789_T3
    #include "Adafruit_GFX.h"
    #include "ST7789_t3.h"

    #define C_WHITE ST77XX_WHITE
    #define BLACK   ST77XX_BLACK
    #define RED     ST77XX_RED
    #define GREEN   ST77XX_GREEN
    #define BLUE    ST77XX_CYAN
    #define YELLOW  ST77XX_YELLOW
    #define ORANGE  ST77XX_ORANGE
    #define PURPLE  ST77XX_MAGENTA

    #undef MENU_C_MAX
    #ifdef TFT_ST7789_T3_BIG
        #define MENU_C_MAX  40
    #else
        #define MENU_C_MAX  20
    #endif
#endif

//#endif