#ifndef DISPLAY_ST7789_T3__INCLUDED
#define DISPLAY_ST7789_T3__INCLUDED

#include "display_abstract.h"

#include <Adafruit_GFX.h>
#include  <SPI.h>
#include "ST7789_t3.h"

#include "tft.h"

#define TFT_CS        10
#define TFT_RST        6 // Or set to -1 and connect to Arduino RESET pin
#define TFT_DC         9 

//Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
ST7789_t3 tft = ST7789_t3(TFT_CS, TFT_DC, TFT_RST);

class DisplayTranslator_STeensy :: public DisplayTranslator {
    ST7739_t3 *tft;
    public:

    int GREEN = ST77XX_GREEN;
    int WHITE = ST77XX_WHITE;
    int BLACK = ST77XX_BLACK;

    DisplayTranslator_STeensy(ST7739_t3 *tft) {
        this->tft = tft;
        this->setup();
    }

    virtual void setup() {
        tft->init(135, 240);           // Init ST7789 240x135
        tft->fillScreen(ST77XX_BLACK);
        delay(500);
        // large block of text
        tft->fillScreen(ST77XX_BLACK);
        //testdrawtext("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Curabitur adipiscing ante sed nibh tincidunt feugiat. Maecenas enim massa, fringilla sed malesuada et, malesuada sit amet turpis. Sed porttitor neque ut ante pretium vitae malesuada nunc bibendum. Nullam aliquet ultrices massa eu hendrerit. Ut sed nisi lorem. In vestibulum purus a tortor imperdiet posuere. ", ST77XX_WHITE);
        tft->useFrameBuffer(true);
    }

    virtual void setCursor(int x, int y) override {
        tft->setCursor(x,y);
    }
    virtual void print(char *text) override {
        tft->print(text);
    }
    virtual int getCursorX() override {
        return tft->getCursorX();
    }
    virtual int getCursorY() override {
        return tft->getCursorY();
    }
    virtual void setTextColor(uint16_t fg, uint16_t bg = ST77XX_BLACK) override {
        tft->setTextColor(fg, bg);
    }
    virtual void drawLine(int x, int y, int w, int h, uint16_t color) override {
        tft->drawLine(x, y, w, h, color);
    }
    virtual void fillRect(int x, int y, int w, int h, uint16_t color) override {
        tft->fillRect(x, y, w, h, color);
    }
    virtual void setTextSize(int size) {
        tft->setTextSize(size);
    }
    virtual void printf(const char *pattern) {
        tft->printf(pattern);
    }
    virtual void printf(const char *pattern, const char *param1) {
        tft->printf(pattern, param1);
    }
    virtual void printf(const char *pattern, const char *param1, const char *param2) {
        tft->printf(pattern, param1, param2);
    }
    virtual void printf(char *pattern, int param1, int param2) {
        tft->printf(pattern, param1);
    }
    virtual void printf(const char *pattern, int param1, int param2, int param3, float param4) {
        char tmp[255];
        tft->printf(pattern, param1, param2, param3, param4);
    }
    
    virtual void println(const char *txt) {
        tft->println(txt);
    }

    virtual void updateDisplay() {
        tft->updateScreenAsync(false);
    }

    virtual int width() {
        return tft->width();
    }
    virtual int height() {
        return tft->height();
    }
}


//#define MAX_KNOB 1024
/*Encoder knob(ENCODER_KNOB_L, ENCODER_KNOB_R);
Bounce pushButtonA = Bounce(PIN_BUTTON_A, 10); // 10ms debounce
Bounce pushButtonB = Bounce(PIN_BUTTON_B, 10); // 10ms debounce
Bounce pushButtonC = Bounce(PIN_BUTTON_C, 10); // 10ms debounce*/

//#include "menu.h"


/*void testdrawtext(char *text, uint16_t color) {
  tft->setCursor(0, 0);
  tft->setTextColor(color);
  tft->setTextWrap(true);
  tft->print(text);
}

// clear screen
void tft_clear() {
    tft->fillScreen(ST77XX_BLACK);
}

void tft_print (const char *text) {
    tft->print(text);
}

void tft_header(ST7789_t3 *tft, const char *text) {
    tft->setTextColor(ST77XX_WHITE,ST77XX_BLACK);
    tft->setTextSize(0);
    tft->println(text);
}*/


#endif