#include "display_abstract.h"

#ifdef TFT_ST7789_T3
    #include <Adafruit_GFX.h>    // Core graphics library
    #include <ST7789_t3.h> // Hardware-specific library for ST7789 on Teensy
    #include "display_st7789_t3.h"
    //extern ST7789_t3 tft;
#endif
#ifdef TFT_SSOLED
    #include <ss_oled.h>
    #include "display_ss_oled.h"
#endif

#ifndef ST77XX_WHITE
    #define ST77XX_WHITE    0xFF
    #define ST77XX_BLACK    0x00
#endif

//extern DisplayTranslator tft;

#include <Encoder.h>
#include <Bounce2.h>

//#include "mymenu.h"

void setup_tft(void);
void tft_clear();
void tft_start();

void tft_update(int ticks);
void tft_print (const char *text);
//void tft_header(ST7789_t3 *tft, const char *text);

uint16_t rgb(uint8_t r, uint8_t g, uint8_t b);
uint16_t rgb(uint32_t rgb);

extern Encoder knob;
extern Bounce pushButtonA;
extern Bounce pushButtonB;
extern Bounce pushButtonC;
