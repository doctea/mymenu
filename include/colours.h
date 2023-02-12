//#ifndef COLOURS__INCLUDED
//#define COLOURS__INCLUDED

#undef MENU_C_MAX
#define MENU_C_MAX 20

#if defined(TFT_SSOLED) or defined(TFT_ST7789_T3) or defined(TFT_ST7789) or defined(TFT_BODMER)
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
    #ifdef GREY
        #undef GREY
    #endif
#endif

#ifdef TFT_SSOLED
    #define C_WHITE     0xFF
    #define BLACK       0x00
    #define RED         0xFA
    #define GREEN       0xAF
    #define BLUE        0x00
    #define YELLOW      0xF0
    #define GREY        0x80
    
    #undef MENU_C_MAX
    #define MENU_C_MAX  20
    #define MENU_MESSAGE_MAX MENU_C_MAX
#endif

#if defined(TFT_ST7789_T3) || defined(TFT_ST7789)
    #include <Adafruit_GFX.h>
    //#include "ST7789_t3.h"
    #if defined(TFT_ST7789)
        #include <Adafruit_ST77xx.h>
    #endif

    #define C_WHITE ST77XX_WHITE
    #define BLACK   ST77XX_BLACK
    #define RED     ST77XX_RED
    #define GREEN   ST77XX_GREEN
    #define BLUE    ST77XX_CYAN
    #define YELLOW  ST77XX_YELLOW
    #define ORANGE  ST77XX_ORANGE
    #define PURPLE  ST77XX_MAGENTA
    #define GREY    (C_WHITE / 2)

    #undef MENU_C_MAX
    #ifdef TFT_ST7789_T3_BIG
        #define MENU_C_MAX  40
    #else
        #define MENU_C_MAX  20
    #endif
    #define MENU_MESSAGE_MAX (MENU_C_MAX*2)
#endif

#if defined(TFT_BODMER)
    //#ifndef _TFT_eSPIH_
        #include <TFT_eSPI.h>
    //#endif
    
    #define C_WHITE TFT_WHITE
    #define BLACK   TFT_BLACK
    #define RED     TFT_RED
    #define GREEN   TFT_GREEN
    #define BLUE    TFT_CYAN
    #define YELLOW  TFT_YELLOW
    #define ORANGE  TFT_ORANGE
    #define PURPLE  TFT_MAGENTA
    #define GREY    (C_WHITE / 2)

    #undef MENU_C_MAX
    #ifdef TFT_ST7789_T3_BIG
        #define MENU_C_MAX  40
    #else
        #define MENU_C_MAX  20
    #endif
    #define MENU_MESSAGE_MAX (MENU_C_MAX*2)
#endif

//#endif