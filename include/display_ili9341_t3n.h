#pragma once
//#define ENABLE_ST77XX_FRAMEBUFFER

#ifdef TFT_ILI9341_T3N

#include "display_abstract.h"

#include "debug.h"

#include "menu.h"

//#include <Adafruit_GFX.h>
#include <SPI.h>
#include <ILI9341_t3n.h>
//#include <ILI9341_fonts.h>

//#include <fonts/FreeMono9pt7b.h>

#include "colours.h"

#include "tft.h"

#define SPI_SPEED 50000000

extern int current_fb;
extern bool done;
extern bool ready_to_change_fb;
extern ILI9341_t3n *ili_tft;
extern uint16_t *fb1[2];// = { nullptr, nullptr };
void frame_complete_callback();

class DisplayTranslator_ILI9341_T3N : public DisplayTranslator {
    public:
	//ILI9341_t3n(uint8_t _CS, uint8_t _DC, uint8_t _RST = 255, uint8_t _MOSI=11, uint8_t _SCLK=13, uint8_t _MISO=12);
    ILI9341_t3n actual = ILI9341_t3n(TFT_CS, TFT_DC, TFT_RST, TFT_MOSI, TFT_SCLK, TFT_MISO);
    ILI9341_t3n *tft = &actual;
  
    virtual int get_row_character_width() override { return (this->width() / ((this->default_textsize+1) * this->characterWidth())); }  // 

    DisplayTranslator_ILI9341_T3N() {
        //this->tft = &actual; //ST7789_t3(TFT_CS, TFT_DC, TFT_RST);
        //this->tft = new ILI9341_t3n(TFT_CS, TFT_DC, TFT_RST, 11, 13, 12);
        this->setup();
        ::ili_tft = this->tft; // set the global tft pointer to our instance's tft, so that the frame complete callback can access it
    }

    uint16_t *allocate_framebuffer() {
        int CBALLOC = TFT_WIDTH*TFT_HEIGHT*sizeof(uint16_t);
        uint16_t *_we_allocated_buffer = (uint16_t *)extmem_malloc(CBALLOC+32);
        if (_we_allocated_buffer == NULL)
            return 0;	// failed 
        _we_allocated_buffer = (uint16_t*) (((uintptr_t)_we_allocated_buffer + 32) & ~ ((uintptr_t) (31)));
        memset(_we_allocated_buffer, 0, CBALLOC);	    
        return _we_allocated_buffer;
    }


    virtual void setup() {
        Debug_println(F("ili9341 setup()..")); Serial_flush();
        if (Serial) { Serial.println(F("ili9341 setup()..")); Serial_flush(); }
        tft->begin(SPI_SPEED);
        tft->setRotation(SCREEN_ROTATION);
        #ifndef DONT_USE_EXTMEM_FRAMEBUFFER
            fb1[0] = allocate_framebuffer();
            fb1[1] = allocate_framebuffer();
            tft->setFrameBuffer(fb1[current_fb]);
        #endif
        tft->useFrameBuffer(true);

        tft->setFrameCompleteCB(frame_complete_callback);

        tft->setFrameRateControl(20);   // 20 to flicker less than 30!
        tft->initDMASettings();
        tft->updateChangedAreasOnly(true);
        //tft->endUpdateAsync
        this->setTextWrap(true);

        tft->setFontAdafruit();

        tft->fillScreen(BLACK);
        //tft->setTextWrap(false);
        tft->println(F("DisplayTranslator_ILI9341 setup()!"));
        Debug_println(F("ili9341 did init()")); Serial_flush();
        Debug_println(F("ili9341 did fillscreen()")); Serial_flush();
        //delay(500);
        // large block of text
        //testdrawtext("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Curabitur adipiscing ante sed nibh tincidunt feugiat. Maecenas enim massa, fringilla sed malesuada et, malesuada sit amet turpis. Sed porttitor neque ut ante pretium vitae malesuada nunc bibendum. Nullam aliquet ultrices massa eu hendrerit. Ut sed nisi lorem. In vestibulum purus a tortor imperdiet posuere. ", ST77XX_WHITE);
        //tft->useFrameBuffer(true);
        if (Serial) Serial.println("finishing setup()");
    }

    virtual void setTextWrap(bool enable_wrap) {
        this->tft->setTextWrap(enable_wrap);
    }
    virtual bool isTextWrap() {
        return this->tft->getTextWrap();
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
    virtual void setTextColor(uint16_t fg, uint16_t bg = BLACK) override {
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

    virtual int getSingleRowHeight() override {
        return 8;
    }
    /*virtual int getRowHeight() override {
        return (tft->getTextSize()+1) * 8;
    }*/
    virtual int characterWidth() override { 
        //return (tft->getTextSize()) * 6;
        return 6;
    };

    virtual void clear(bool force = false) {
        //if (force) {
            tft->fillScreen(BLACK);
            //tft->fillRect(0,0,tft->width(),tft->height(),BLACK);
            tft->setTextColor(C_WHITE);
        //}
        //tft->fillRect(0, 0, tft->width(), tft->height(), BLACK);
    }

    virtual void start() override {
        DisplayTranslator::start();
        Debug_println("Display start.."); Serial_flush();
        //tft->updateScreenAsync(false);
        tft->useFrameBuffer(true);
        Debug_println("did useframebuffer()"); Serial_flush();
    }
    
    virtual void updateDisplay() {
        if (Serial) Serial.println("updateDisplay..");
        //tft->updateScreenAsync(false);
        //tft->updateChangedAreasOnly(true);
        //if (/*ready() &&*/ !tft->asyncUpdateActive()) {
            if (done) {
                done = false;
                //tft->updateScreenAsync(false);
                //tft->updateScreen();
            }
            if (Serial) Serial.printf("updateDisplay.. current_fb=%i\n", current_fb); Serial_flush();
            
            // if (ready_to_change_fb) {
                //     Serial.printf("ready to change fb! current_fb=%i\n", current_fb); Serial_flush();
                //     //tft->setFrameBuffer(fb1[current_fb]);
                //     ready_to_change_fb = false;
                // }
                //this->framebuffer_ready = false;
                //tft->updateScreen();
                //}
                
                // effectively we know that drawing into current framebuffer has now finished; 
                // so we can switch the tft to the other framebuffer for drawing the next frame, while the dma is pushing the current framebuffer to the screen. 
                // this should allow us to draw the next frame in parallel with the dma pushing the current frame to the screen, 
                // and thus hopefully achieve higher framerates (since we can draw at our own pace, without needing to wait for the dma to finish pushing to the screen before we can start drawing the next frame).
                
        if (ready_to_change_fb) {
            tft->updateScreenAsync(true);
            current_fb = !current_fb;
            tft->setFrameBuffer(fb1[current_fb]);
            ready_to_change_fb = false;
        }
    }

    // ready to start rendering?
    virtual bool ready() override {
        //return true;
        //return done && !tft->asyncUpdateActive();
        //return !tft->asyncUpdateActive();
        //return this->framebuffer_ready;
        // bool ready = done; // && !tft->asyncUpdateActive();
        // if (ready_to_change_fb) {
        //     Serial.printf("ready to change fb! current_fb=%i\n", current_fb); Serial_flush();
        //     tft->setFrameBuffer(fb1[current_fb]);
        //     ready_to_change_fb = false;
        // }
        // return ready;
        // return ready;
        //return true;

        // we are ready to start rendering if the dma is not active, and either we are done with the previous frame, 
        // or we are ready to change the framebuffer (ie, the dma completed and called the callback to flip the fb)
        // bool ready = !tft->asyncUpdateActive() && (done || ready_to_change_fb);
        // if (ready_to_change_fb) {
        //     Serial.printf("ready to change fb! current_fb=%i\n", current_fb); Serial_flush();
        //     tft->setFrameBuffer(fb1[current_fb]);
        //     ready_to_change_fb = false;
        // }
        // return ready;
        
        // this is called to check whether we are ready to start rendering the next frame; 
        // we are ready to start rendering if the dma is not active (ie, it is not currently pushing a frame to the screen),
        // and either we are done with the previous frame (ie, the dma finished pushing the previous frame to the screen),
        // or we are ready to change the framebuffer (ie, the dma completed and called the callback to flip the fb, so we can start drawing the next frame in the other framebuffer).
        // bool ready = !tft->asyncUpdateActive() && (done || ready_to_change_fb);
        // if (ready_to_change_fb) {
        //     Serial.printf("ready to change fb! current_fb=%i\n", current_fb); Serial_flush();
        //     //tft->setFrameBuffer(fb1[current_fb]);
        //     ready_to_change_fb = false; 
        // }
        // return ready;
        Serial.printf("ready() called! current_fb=%i, done=%i, ready_to_change_fb=%i\n", current_fb, done, ready_to_change_fb); Serial_flush();
        return ready_to_change_fb;
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
