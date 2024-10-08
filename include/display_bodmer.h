#ifndef DISPLAY_BODMER__INCLUDED
#define DISPLAY_BODMER__INCLUDED

//#define ENABLE_ST77XX_FRAMEBUFFER

#ifdef TFT_BODMER

#include "display_abstract.h"

#include "debug.h"

#include "menu.h"

#ifdef BODMER_BUFFERED
    #include <Adafruit_GFX_Buffer.h>
    #include <Adafruit_GFX.h>
#endif
/*#include <SPI.h>
//#include "ST7789_t3.h"
#include <Adafruit_ST7789.h>
*/
#define USE_SPI_DMA
#include <TFT_eSPI.h>

#include "colours.h"

#include "tft.h"

#ifndef TFT_CS
    #define TFT_CS        D7
#endif
#ifndef TFT_RST
    #define TFT_RST        -1 // Or set to -1 and connect to Arduino RESET pin
#endif
#ifndef TFT_DC
    #define TFT_DC         D0
#endif

#ifndef SCREEN_WIDTH
    #define SCREEN_WIDTH TFT_WIDTH
    //#define SCREEN_WIDTH TFT_HEIGHT
#endif
#ifndef SCREEN_HEIGHT
    #define SCREEN_HEIGHT TFT_HEIGHT
    //#define SCREEN_HEIGHT TFT_WIDTH
#endif
#ifndef SCREEN_ROTATION
    #define SCREEN_ROTATION 0
#endif

class DisplayTranslator_Bodmer : public DisplayTranslator {
    public:

    #ifdef BODMER_BUFFERED
        TFT_eSPI    actual = TFT_eSPI();
        Adafruit_GFX_Buffer<TFT_eSPI> *tft = new Adafruit_GFX_Buffer<TFT_eSPI>(240, 135, actual);
    #else
        #ifdef BODMER_SPRITE
            //TFT_eSPI    real_actual_espi = TFT_eSPI(); //SCREEN_HEIGHT, SCREEN_WIDTH);
            TFT_eSPI *real_actual_espi = nullptr;
            //TFT_eSprite real_actual_sprite = TFT_eSprite(&real_actual_espi);
            //TFT_eSprite actual = TFT_eSprite(&real_actual_espi);
            TFT_eSprite *actual = nullptr;
            uint16_t* sprPtr = nullptr;
            //TFT_eSprite *tft = &actual;
            TFT_eSprite *tft = nullptr;//&actual;
        #else
            TFT_eSPI    actual = TFT_eSPI();
            TFT_eSPI *tft = &actual;
        #endif        
    #endif

    /*virtual const char *get_message_format() { return "[%-39.39s]"; }
    virtual const char *get_header_format() { return "%-41s"; }
    virtual const char *get_header_open_format() { return ">>>%-38s"; }
    virtual const char *get_header_selected_format() { return "%-41s"; }*/

    DisplayTranslator_Bodmer() {

    }

    // for this translator (Adafruit GFX + GFX_Buffer), we seem to need to initialise dynamically instead of statically
    virtual void init() override {
        this->setup();
    }
    virtual void setup() {
        Debug_println(F("DisplayTranslator_Bodmer setup()..")); Serial_flush();
        real_actual_espi = new TFT_eSPI();
        actual = new TFT_eSprite(real_actual_espi);
        tft = actual;
        tft->init(); //SCREEN_WIDTH, SCREEN_HEIGHT);           // Init ST7789 240x135

        #ifdef ARDUINO_ARCH_RP2040
            actual->setFreeFont();
        #endif

        #ifdef BODMER_BUFFERED
            actual.initDMA();
            tft->setRotation(SCREEN_ROTATION);
            actual.setRotation(SCREEN_ROTATION);
            actual.startWrite();
        #else
            #ifdef BODMER_SPRITE
                real_actual_espi->setRotation(SCREEN_ROTATION);
                
                #ifdef ARDUINO_ARCH_RP2040
                    real_actual_espi->initDMA();
                #endif
                real_actual_espi->setSwapBytes(false);
                if(SCREEN_ROTATION%2==1) {
                    sprPtr = (uint16_t*)actual->createSprite(SCREEN_HEIGHT, SCREEN_WIDTH);   // < - corrupts display in way that i did find mentioned in that thread !
                    real_actual_espi->fillRect(0, 0, SCREEN_HEIGHT, SCREEN_WIDTH, BLACK);
                } else {
                    sprPtr = (uint16_t*)actual->createSprite(SCREEN_WIDTH, SCREEN_HEIGHT);   // < - corrupts display in way that i did find mentioned in that thread !
                    real_actual_espi->fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, BLACK);
                }
                tft->fillSprite(BLACK);
                tft->setRotation(SCREEN_ROTATION);
                real_actual_espi->startWrite();
            #else
                //actual.initDMA();
                //real_actual_espi.setRotation(2); //SCREEN_ROTATION);
                //actual.setRotation(SCREEN_ROTATION);
            #endif
        #endif
        //tft->setRotation(1);
        tft->fillScreen(BLACK);
        this->setTextWrap(true);
        tft->setCursor(0,0);
        tft->println(F("DisplayTranslator init()!"));

        Debug_println(F("did init()")); Serial_flush();
        Debug_println(F("did fillscreen()")); Serial_flush();
        delay(500);
        // large block of text
        //testdrawtext("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Curabitur adipiscing ante sed nibh tincidunt feugiat. Maecenas enim massa, fringilla sed malesuada et, malesuada sit amet turpis. Sed porttitor neque ut ante pretium vitae malesuada nunc bibendum. Nullam aliquet ultrices massa eu hendrerit. Ut sed nisi lorem. In vestibulum purus a tortor imperdiet posuere. ", ST77XX_WHITE);
        //tft->useFrameBuffer(true);
    }


    // bodmer library doesn't expose the textwrap status, so remember it here instead
    bool enable_text_wrap = false;
    virtual void setTextWrap(bool enable_wrap) override {
        this->enable_text_wrap = enable_wrap;

        tft->setTextWrap(enable_wrap);
        real_actual_espi->setTextWrap(enable_wrap);
        actual->setTextWrap(enable_wrap);
    }
    virtual bool isTextWrap() override {
        return this->enable_text_wrap;
    }

    // this should work if this patch is ever accepted to the TFT_eSPI library: https://github.com/Bodmer/TFT_eSPI/pull/3380
    /*virtual bool setTextWrap(bool enable_wrap) override {
        tft->setTextWrap(enable_wrap);
        real_actual_espi->setTextWrap(enable_wrap);
        actual->setTextWrap(enable_wrap);
    }
    virtual bool isTextWrap() override {
        return tft->getTextWrap();
    }*/

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
    virtual void setTextColor(uint16_t fg, uint16_t bg = BLACK) override {
        //Serial.printf("setTextColor setting fg=%0x,\tbg=%0x\n", fg, bg);
        tft->setTextColor(fg, bg);
    }
    virtual void setTextSize(int size) override {
        size = constrain(size, 0, 2);
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
        // todo: sort this logic out properly
        #ifdef ILI9341_DRIVER
            if (SCREEN_ROTATION%2==0)
                return tft->width();
            else
                return tft->height();
        #else
            return tft->width();
        #endif
    }
    virtual int height() {
        // todo: sort this logic out properly
        #ifdef ILI9341_DRIVER
            if (SCREEN_ROTATION%2==0)
                return tft->height();
            else
                return tft->width();
        #else
            return tft->height();
        #endif
    }
    virtual int getSingleRowHeight() override {
        return 6; // or is it 8?
    }
    /*virtual int getRowHeight() override {
        //return (tft->getTextSize()+1) * 6;
        //return tft->getTextBounds()
        if (this->size==0) return 8;
        return (this->size+1) * 6;   // TODO: figure out how to do this properly
    }*/
    virtual int characterWidth() override { 
        //return (tft->getTextSize()) * 6;
        return 6;
    };

    virtual void clear(bool force = false) override {
        #if !defined(BDOMER_BUFFERED) && !defined(BODMER_SPRITE)
            if (force)
        #endif
        #ifdef BODMER_SPRITE
            tft->fillRect(0, 0, width(), height(), BLACK);
        #else
            tft->fillScreen(BLACK);
        #endif
        tft->setTextColor(C_WHITE, BLACK);
        //tft->fillRect(0, 0, tft->width(), tft->height(), BLACK);
    }

    virtual void start() override {
        DisplayTranslator::start();
        Debug_println("Display start.."); Serial_flush();
        //tft->updateScreenAsync(false);
    }

    virtual void updateDisplay() override {
        #ifdef BODMER_SPRITE
            //Serial.println("updateDisplay sprite mode");
            //spr.fillSprite(random(0,pow(2,16)));
            if (this->ready()) {
                int s_w, s_h;
                if (SCREEN_ROTATION%2==1) {
                    s_w = SCREEN_HEIGHT;
                    s_h = SCREEN_WIDTH;
                } else {
                    s_w = SCREEN_WIDTH;
                    s_h = SCREEN_HEIGHT;
                }

                #ifdef ARDUINO_ARCH_RP2040
                    real_actual_espi->pushImageDMA(0, 0, s_w, s_h, sprPtr);
                #else
                    real_actual_espi->pushImage(0, 0, s_w, s_h, sprPtr);
                #endif
            }
        #endif
        //tft->updateScreenAsync(false);
        //tft->display();
        //tft->push
    }

    #ifdef BODMER_SPRITE
        virtual bool ready() override {
            #ifdef ARDUINO_ARCH_RP2040
                return !real_actual_espi->dmaBusy();
            #else
                return true;
            #endif
        }
    #endif

    virtual void drawLine(int x0, int y0, int x1, int y1, uint16_t color) override {
        tft->drawLine(x0, y0, x1, y1, color);
    }
    virtual void drawFastVLine(int x, int y, int height, uint16_t colour) override {
        actual->drawFastVLine(x, y, height, colour);   
    }
    virtual void drawFastHLine(int x, int y, int width, uint16_t colour) override {
        actual->drawFastHLine(x, y, width, colour);    
    }

    virtual void fillRect(int x, int y, int w, int h, uint16_t color) override {
        tft->fillRect(x, y, w, h, color);
    }
    virtual void fillCircle(int x, int y, int radius, uint16_t colour) override {
        actual->fillCircle(x, y, radius, colour);
    }
    virtual void drawRect(int x, int y, int w, int h, uint16_t color) override {
        actual->drawRect(x, y, w, h, color);
    }

    /*virtual void drawRGBBitmap(int x, int y, GFXcanvas16 *c) {
        //tft->drawBitmap(c, x, y)
        tft->drawRGBBitmap(x, y, c->getBuffer(), c->width(), c->height());
    }*/
};

#endif
#endif
