#ifndef DISPLAY_ST7789_T3__INCLUDED
#define DISPLAY_ST7789_T3__INCLUDED

//#define ENABLE_ST77XX_FRAMEBUFFER

#ifdef TFT_ST7789_T3

#include "display_abstract.h"

#include "debug.h"

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
   
    /*virtual const char *get_message_format() { return "[%-20.20s]"; }
    virtual const char *get_header_format() { return "%-22s"; }
    virtual const char *get_header_open_format() { return ">>>%-19s"; }
    virtual const char *get_header_selected_format() { return "%-22s"; }*/

    DisplayTranslator_STeensy() {
        this->tft = &actual; //ST7789_t3(TFT_CS, TFT_DC, TFT_RST);
        this->setup();
    }

    virtual void setTextWrap(bool enable_wrap) {
        this->tft->setTextWrap(enable_wrap);
    }
    virtual bool isTextWrap() {
        return this->tft->getTextWrap();
    }

    virtual void setup() {
        Debug_println(F("steensy setup()..")); Serial_flush();
        tft->init(135, 240);           // Init ST7789 240x135
        tft->fillScreen(ST77XX_BLACK);
        //tft->setTextWrap(false);
        tft->println(F("DisplayTranslator init()!"));
        Debug_println(F("did init()")); Serial_flush();
        Debug_println(F("did fillscreen()")); Serial_flush();
        delay(500);
        // large block of text
        //testdrawtext("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Curabitur adipiscing ante sed nibh tincidunt feugiat. Maecenas enim massa, fringilla sed malesuada et, malesuada sit amet turpis. Sed porttitor neque ut ante pretium vitae malesuada nunc bibendum. Nullam aliquet ultrices massa eu hendrerit. Ut sed nisi lorem. In vestibulum purus a tortor imperdiet posuere. ", ST77XX_WHITE);
        //tft->useFrameBuffer(true);
    }


    virtual void setCursor(int x, int y) override {
        tft->setCursor(x,y);
    }
    virtual void print(const char *text) override {
        tft->print(text);
    }
    virtual int getCursorX() override {
        return tft->getCursorX();
    }
    virtual int getCursorY() override {
        return tft->getCursorY();
    }
    virtual void setTextColor(uint16_t fg, uint16_t bg = ST77XX_BLACK) override {
        //Serial.printf("setTextColor setting fg=%0x,\tbg=%0x\n", fg, bg);
        tft->setTextColor(fg, bg);
    }
    virtual void setTextSize(int size) override {
        size += this->default_textsize;
        this->size = size;
        tft->setTextSize(this->size);
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
    virtual void printf(const char *pattern, char *param1, char *param2, char *param3) override {
        tft->printf(pattern, param1, param2, param3);
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
    }
    virtual void printf(const char *pattern, char param1, int param2, char *param3) {
        tft->printf(pattern, param1, param2, param3);
    }
    virtual void println() {
        tft->println();
    }
    virtual void printc(char c) override {
        tft->print(c);
    }

    virtual int width() {
        return tft->width();
    }
    virtual int height() {
        return tft->height();
    }

    /*virtual int getRowHeight() override {
        return (tft->getTextSize()+1) * 6;
    }*/
    virtual int characterWidth() override { 
        //return (tft->getTextSize()) * 6;
        return 6;
    };

    virtual void clear(bool force = false) override {
        tft->fillScreen(BLACK);
        tft->setTextColor(C_WHITE);
        //tft->fillRect(0, 0, tft->width(), tft->height(), BLACK);
    }

    virtual void start() override {
        DisplayTranslator::start();
        Debug_println("Display start.."); Serial_flush();
        //tft->updateScreenAsync(false);
        tft->useFrameBuffer(true);
        Debug_println("did useframebuffer()"); Serial_flush();
    }

    virtual void updateDisplay() override {
        //Serial.println("updateDisplay..");
        tft->updateScreenAsync(false);
    }

    virtual void drawLine(int x0, int y0, int x1, int y1, uint16_t color) override {
        tft->drawLine(x0, y0, x1, y1, color);
    }
    virtual void drawFastVLine(int x, int y, int height, uint16_t colour) override {
        tft->drawFastVLine(x, y, height, colour);   
    }
    virtual void drawFastHLine(int x, int y, int width, uint16_t colour) override {
        tft->drawFastHLine(x, y, width, colour);    
    }

    virtual void fillRect(int x, int y, int w, int h, uint16_t color) override {
        tft->fillRect(x, y, w, h, color);
    }
    virtual void fillCircle(int x, int y, int radius, uint16_t colour) override {
        tft->fillCircle(x, y, radius, colour);
    }
    virtual void drawRect(int x, int y, int w, int h, uint16_t color) override {
        tft->drawRect(x, y, w, h, color);
    }
    /*virtual void drawRGBBitmap(int x, int y, GFXcanvas16 *c) {
        //tft->drawBitmap(c, x, y)
        tft->drawRGBBitmap(x, y, c->getBuffer(), c->width(), c->height());
    }*/
};


#endif
#endif