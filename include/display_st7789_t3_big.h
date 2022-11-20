#ifndef DISPLAY_ST7789_T3_BIG__INCLUDED
#define DISPLAY_ST7789_T3_BIG__INCLUDED

#ifdef TFT_ST7789_T3

#include "display_st7789_t3.h"

#define TFT_CS        10
#undef TFT_RST
#define TFT_RST        -1 //6 // Or set to -1 and connect to Arduino RESET pin
#define TFT_DC         9 

class DisplayTranslator_STeensy_Big : public DisplayTranslator_STeensy {
    public:
    ST7789_t3 actual = ST7789_t3(TFT_CS, TFT_DC, TFT_RST);

    virtual const char *get_message_format() { return "[%-40s]"; }
    virtual const char *get_header_format() { return "%-40s"; }
    virtual const char *get_header_open_format() { return ">>>%-37s"; }
    virtual const char *get_header_selected_format() { return "%-40s"; }
    virtual byte get_c_max() {
        return MENU_C_MAX;
    }

    DisplayTranslator_STeensy_Big () {
        this->tft = &this->actual; //ST7789_t3(TFT_CS, TFT_DC, TFT_RST);
        this->setup();
    };

    virtual void setup() {
        Serial.println(F("steensy_big setup()..")); Serial_flush();
        tft->init(240, 320);           // Init ST7789 240x135
        tft->setRotation(2);
        tft->fillScreen(ST77XX_BLACK);
        tft->println("DisplayTranslator_STeensy_Big init()!");
        Serial.println(F("did init()")); Serial_flush();
        Serial.println(F("did fillscreen()")); Serial_flush();
        delay(500);
        // large block of text
        //testdrawtext("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Curabitur adipiscing ante sed nibh tincidunt feugiat. Maecenas enim massa, fringilla sed malesuada et, malesuada sit amet turpis. Sed porttitor neque ut ante pretium vitae malesuada nunc bibendum. Nullam aliquet ultrices massa eu hendrerit. Ut sed nisi lorem. In vestibulum purus a tortor imperdiet posuere. ", ST77XX_WHITE);
        //tft->useFrameBuffer(true);
    }
};
#endif

#endif