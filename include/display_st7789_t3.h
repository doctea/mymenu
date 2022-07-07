#ifndef DISPLAY_ST7789_T3__INCLUDED
#define DISPLAY_ST7789_T3__INCLUDED

//#define ENABLE_ST77XX_FRAMEBUFFER

#ifdef TFT_ST7789_T3

#include "display_abstract.h"

#include "menu.h"

#include <Adafruit_GFX.h>
#include <SPI.h>
#include "ST7789_t3.h"

#include "colours.h"

#include "tft.h"

#define TFT_CS        10
#define TFT_RST        6 // Or set to -1 and connect to Arduino RESET pin
#define TFT_DC         9 

//Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

/*#define C_WHITE ST77XX_WHITE
#define BLACK   ST77XX_BLACK
#define RED     ST77XX_RED
#define GREEN   ST77XX_GREEN
#define BLUE    ST77XX_BLUE
#define YELLOW  ST77XX_YELLOW*/

class DisplayTranslator_STeensy : public DisplayTranslator {
    public:
    ST7789_t3 actual = ST7789_t3(TFT_CS, TFT_DC, TFT_RST);
    ST7789_t3 *tft;
   
    virtual const char *get_message_format() { return "[%-20s]"; }
    virtual const char *get_header_format() { return "%-22s"; }
    virtual const char *get_header_open_format() { return ">>>%-19s"; }
    virtual const char *get_header_selected_format() { return "%-22s"; }

    DisplayTranslator_STeensy() {
        this->tft = &actual; //ST7789_t3(TFT_CS, TFT_DC, TFT_RST);
        this->setup();
    }

    virtual void setup() {
        Serial.println("steensy setup().."); Serial.flush();
        tft->init(135, 240);           // Init ST7789 240x135
        tft->fillScreen(ST77XX_BLACK);
        tft->println("DisplayTranslator init()!");
        Serial.println("did init()"); Serial.flush();
        Serial.println("did fillscreen()"); Serial.flush();
        delay(500);
        // large block of text
        //testdrawtext("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Curabitur adipiscing ante sed nibh tincidunt feugiat. Maecenas enim massa, fringilla sed malesuada et, malesuada sit amet turpis. Sed porttitor neque ut ante pretium vitae malesuada nunc bibendum. Nullam aliquet ultrices massa eu hendrerit. Ut sed nisi lorem. In vestibulum purus a tortor imperdiet posuere. ", ST77XX_WHITE);
        //tft->useFrameBuffer(true);
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
    virtual void setTextSize(int size) override {
        tft->setTextSize(size);
    }
    virtual void printf(const char *pattern) override {
        tft->printf(pattern);
    }
    virtual void printf(const char *pattern, char *param1) override {
        tft->printf(pattern, param1);
    }
    virtual void printf(const char *pattern, char *param1, char *param2) override {
        tft->printf(pattern, param1, param2);
    }
    virtual void printf(const char *pattern, int param1) override {
        tft->printf(pattern, param1);
    }
    virtual void printf(const char *pattern, int param1, int param2) override {
        tft->printf(pattern, param1, param2);
    }
    virtual void printf(const char *pattern, int param1, int param2, int param3) override {
        tft->printf(pattern, param1, param2, param3);
    }
    virtual void printf(const char *pattern, int param1, int param2, int param3, float param4) override {
        tft->printf(pattern, param1, param2, param3, param4);
    }  
    virtual void println(const char *txt) override {
        tft->println(txt);
    }
    virtual void drawRoundRect(int x, int y, int w, int h, int radius, int color) override {
        tft->drawRoundRect(x, y, w, h, radius, color);
    }
    virtual void fillRoundRect(int x, int y, int w, int h, int radius, int color) override {
        tft->fillRoundRect(x, y, w, h, radius, color);
    }
    virtual void printf(const char *pattern, int param1, char* param2) override {
        tft->printf(pattern, param1, param2);
    }
    virtual void printf(const char *pattern, int param1, const uint8_t* param2) override {
        tft->printf(pattern, param1, param2);
    }
    virtual void printf(const char *pattern, char *param1, int param2, int param3) {
        tft->printf(pattern, param1, param2, param3);
    };

    virtual int width() {
        return tft->width();
    }
    virtual int height() {
        return tft->height();
    }

    virtual int getRowHeight() override {
        return (tft->getTextSize()+1) * 8;
    }

    virtual void clear(bool force = false) {
        tft->fillScreen(BLACK);
        //tft->fillRect(0, 0, tft->width(), tft->height(), BLACK);
    }

    virtual void start() {
        Serial.println("Display start.."); Serial.flush();
        //tft->updateScreenAsync(false);
        tft->useFrameBuffer(true);
        Serial.println("did useframebuffer()"); Serial.flush();
    }

    virtual void updateDisplay() {
        //Serial.println("updateDisplay..");
        tft->updateScreenAsync(false);
    }
};


//void tft_print(const char *text);

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
#endif