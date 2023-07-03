#ifndef DISPLAY_ILI9341_TGX__INCLUDED
#define DISPLAY_ILI9341_TGX__INCLUDED

#ifdef TFT_ILI9341_TGX

#include "display_abstract.h"

#include <ILI9341Wrapper.h>

#include "debug.h"

#include "menu.h"

//#include <Adafruit_GFX.h>
#include <SPI.h>


// the screen driver library : https://github.com/vindar/ILI9341_T4
#include <ILI9341_T4.h> 

// the tgx library 
#include <tgx.h> 
//#include <font_tgx_OpenSans_Bold.h>
#include <font_tgx_Arial.h>

#include "colours.h"

#include "tft.h"

#define PIN_BACKLIGHT 255   // optional, set this only if the screen LED pin is connected directly to the Teensy.
#define PIN_TOUCH_IRQ 255   // optional. set this only if the touchscreen is connected on the same SPI bus
#define PIN_TOUCH_CS  255   // optional. set this only if the touchscreen is connected on the same spi bus

#define SPI_SPEED       40000000

using namespace tgx;

//ILI9341_T4::ILI9341Driver actual(PIN_CS, PIN_DC, PIN_SCLK, PIN_MOSI, PIN_MISO, PIN_RST, PIN_TOUCH_CS, PIN_TOUCH_IRQ);
ILI9341Wrapper actual(PIN_CS, PIN_DC, PIN_SCLK, PIN_MOSI, PIN_MISO, PIN_RST, PIN_TOUCH_CS, PIN_TOUCH_IRQ);

// 2 x 10K diff buffers (used by tft) for differential updates (in DMAMEM)
DMAMEM ILI9341_T4::DiffBuffStatic<6000> diff1;
DMAMEM ILI9341_T4::DiffBuffStatic<6000> diff2;

// internal framebuffer (150K in DMAMEM) used by the ILI9431_T4 library for double buffering.
DMAMEM uint16_t internal_fb[TFT_WIDTH * TFT_HEIGHT];

 // main screen framebuffer (150K in DTCM for fastest access)
DMAMEM uint16_t framebuffer[TFT_WIDTH * TFT_HEIGHT];
Image<RGB565> img(framebuffer, 320, 240);

class DisplayTranslator_ILI9341_TGX : public DisplayTranslator {
    public:
    //ILI9341_T4::ILI9341Driver *tft = &actual;
 
    // image that encapsulates fb.
    Image<RGB565> *tft = &img;

    iVec2_s16 cursor;

    virtual const char *get_message_format() { return "[%-38.38s]"; }
    virtual const char *get_header_format() { return "%-40s"; }
    virtual const char *get_header_open_format() { return ">>>%-37s"; }
    virtual const char *get_header_selected_format() { return "%-40s"; }
    virtual uint8_t get_c_max() {
        return MENU_C_MAX;
    }

    DisplayTranslator_ILI9341_TGX() {
        //this->tft = &actual; //ST7789_t3(TFT_CS, TFT_DC, TFT_RST);
        //this->tft = new ILI9341_t3n(TFT_CS, TFT_DC, TFT_RST, 11, 13, 12);
        this->setup();
    }

    virtual void setup() {
        Debug_println(F("ili9341_tgx setup()..")); Serial_flush();
        Serial.println(F("ili9341_tgx setup()..")); Serial_flush();
        while (!actual.begin(SPI_SPEED));

        actual.setRotation(SCREEN_ROTATION);
        actual.setCanvas(internal_fb, TFT_WIDTH, TFT_HEIGHT);
        actual.setDiffBuffers(&diff1, &diff2);
        actual.setDiffGap(4);
        actual.setRefreshRate(60); // was 140
        //actual.setVsyncSpacing(2);

        //tft->setFont(Arial_18);
        /*while (true) {
            //tft->println(F("DisplayTranslator_ILI9341 setup()!"));
            tft->fillRect(0, 0, tft->width(), tft->height(), random(65535));
            tft->updateScreen();
            //delay(100);
            //Serial.println("did thing");
        }*/

        //tft->init(240, 320);           // Init ST7789 240x135
        tft->fillScreen((uint16_t)BLACK);
        //tft->setTextWrap(false);
        //tft->println(F("DisplayTranslator_ILI9341_tgx setup()!"));
        iVec2 pos = { 0, 0 };
        tft->drawText("DisplayTranslator_ILI9341_tgx setup()!", pos, (uint16_t)C_WHITE, font_tgx_Arial_8, 1);
        Debug_println(F("ili9341_tgx did init()")); Serial_flush();
        Debug_println(F("ili9341_tgx did fillscreen()")); Serial_flush();
        //delay(500);
        // large block of text
        //testdrawtext("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Curabitur adipiscing ante sed nibh tincidunt feugiat. Maecenas enim massa, fringilla sed malesuada et, malesuada sit amet turpis. Sed porttitor neque ut ante pretium vitae malesuada nunc bibendum. Nullam aliquet ultrices massa eu hendrerit. Ut sed nisi lorem. In vestibulum purus a tortor imperdiet posuere. ", ST77XX_WHITE);
        //tft->useFrameBuffer(true);
        Serial.println("finishing setup()");
    }
    
    virtual void setCursor(int x, int y) override {
        //tft->setCursor(x,y);
    }
    virtual int getCursorX() override {
        //return tft->getCursorX();
        return 0;
    }
    virtual int getCursorY() override {
        return 0; //return tft->getCursorY();
    }
    /*
    virtual void print(const char *text) override {
        tft->print(text);
    }
    virtual void setTextColor(uint16_t fg, uint16_t bg = BLACK) override {
        //Serial.printf("setTextColor setting fg=%0x,\tbg=%0x\n", fg, bg);
        tft->setTextColor(fg, bg);
    }
    virtual void setTextSize(unsigned int size) override {
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
    }*/

    virtual int width() {
        return tft->width();
    }
    virtual int height() {
        return tft->height();
    }

    virtual int getRowHeight() override {
        //return (tft->getTextSize()+1) * 6;
        return 6;
    }
    virtual int characterWidth() override { 
        //return (tft->getTextSize()) * 6;
        return 6;
    };

    virtual void clear(bool force = false) {
        /*tft->fillScreen(BLACK);
        tft->setTextColor(C_WHITE);*/

        //tft->fillRect(0, 0, tft->width(), tft->height(), BLACK);
    }

    virtual void start() {
        Debug_println("Display start.."); Serial_flush();
        //tft->updateScreenAsync(false);
        //tft->useFrameBuffer(true);
        Debug_println("did useframebuffer()"); Serial_flush();
    }

    virtual void updateDisplay() {
        //Serial.println("updateDisplay..");
        //tft->updateScreenAsync(false);
        actual.update(framebuffer);
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
        actual.drawFilledCircle<false,true>(x, y, radius, colour, colour);
    }
    virtual void drawRect(int x, int y, int w, int h, uint16_t color) override {
        tft->drawRect(x, y, w, h, color);
    }
    /*virtual void drawRGBBitmap(int x, int y, GFXcanvas16 *c) {
        //tft->drawBitmap(c, x, y)
        tft->drawRGBBitmap(x, y, c->getBuffer(), c->width(), c->height());
    }*/
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