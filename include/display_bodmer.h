#pragma once

//#define ENABLE_ST77XX_FRAMEBUFFER

#ifdef TFT_BODMER

#include "tusb.h"

#include "display_abstract.h"

#include "debug.h"

#include "menu.h"

#ifdef BODMER_BUFFERED
    #include <Adafruit_GFX_Buffer.h>
    #include <Adafruit_GFX.h>
#endif
#define USE_SPI_DMA
#include <TFT_eSPI.h>

#if defined(DISPLAY_RGB332_FB_MODE) && !defined(BODMER_SPRITE)
    #error DISPLAY_RGB332_FB_MODE requires BODMER_SPRITE
#endif

#ifndef DISPLAY_RGB332_DMA_CHUNK_LINES
    #define DISPLAY_RGB332_DMA_CHUNK_LINES 16
#endif

#ifndef DISPLAY_RGB332_DMA_PINGPONG
    #define DISPLAY_RGB332_DMA_PINGPONG 1
#endif

#ifndef DISPLAY_RGB332_DIRTY_FLUSH
    #define DISPLAY_RGB332_DIRTY_FLUSH 1
#endif

// wrapper so that we can get the framebuffer pointer out of the TFT_eSprite, which is needed for pushing out the framebuffer data over serial
class TFT_eSprite_Wrapper : public TFT_eSprite {
    public:
    TFT_eSprite_Wrapper(TFT_eSPI *tft) : TFT_eSprite(tft) {}
    
    int16_t *get_img() {
        return (int16_t*)TFT_eSprite::_img;
    }
};
//using TFT_eSprite = TFT_eSprite_Wrapper;

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
            TFT_eSprite_Wrapper *actual = nullptr;
            uint16_t* sprPtr = nullptr;
            
            #if MENU_PERF_PARTIAL_UPDATES
            // Dirty Y-range for selective DMA push. Accumulated during a frame;
            // reset_dirty_region() clears it at frame start.
            int dirty_y_min = INT_MAX;
            int dirty_y_max = 0;
            #endif
            
            #ifdef DISPLAY_RGB332_FB_MODE
                uint8_t* sprPtr8 = nullptr;
                uint16_t* dmaStageA = nullptr;
                #if DISPLAY_RGB332_DMA_PINGPONG
                    uint16_t* dmaStageB = nullptr;
                #endif
                uint32_t dmaStageCapacityPixels = 0;
                uint16_t rgb332_to_565[256] = {0};
                bool rgb332LutReady = false;
                #if DISPLAY_RGB332_DIRTY_FLUSH
                    bool dirtyPending = true;
                    int16_t dirtyX0 = 0;
                    int16_t dirtyY0 = 0;
                    int16_t dirtyX1 = 0;
                    int16_t dirtyY1 = 0;
                #endif
            #endif
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

    #if defined(BODMER_SPRITE) && defined(DISPLAY_RGB332_FB_MODE)
    void initRGB332Lut() {
        if (rgb332LutReady) return;
        const uint8_t blue[] = {0, 11, 21, 31};
        for (uint16_t c = 0; c < 256; c++) {
            uint8_t msb = (c & 0x1C) >> 2 | (c & 0xC0) >> 3 | (c & 0xE0);
            uint8_t lsb = (c & 0x1C) << 3 | blue[c & 0x03];
            // pushImageDMA() uses byte-swap on RP2040 when _swapBytes is false,
            // so store pre-swapped words to keep colours correct on panel.
            rgb332_to_565[c] = (uint16_t(lsb) << 8) | msb;
        }
        rgb332LutReady = true;
    }

    bool ensureDMAStageBuffers(int width) {
        if (width <= 0) return false;
        uint32_t chunkLines = DISPLAY_RGB332_DMA_CHUNK_LINES;
        if (chunkLines == 0) chunkLines = 1;

        uint32_t neededPixels = uint32_t(width) * chunkLines;
        if (neededPixels <= dmaStageCapacityPixels && dmaStageA != nullptr) return true;

        if (dmaStageA != nullptr) { free(dmaStageA); dmaStageA = nullptr; }
        #if DISPLAY_RGB332_DMA_PINGPONG
            if (dmaStageB != nullptr) { free(dmaStageB); dmaStageB = nullptr; }
        #endif
        dmaStageCapacityPixels = 0;

        dmaStageA = (uint16_t*)malloc(sizeof(uint16_t) * neededPixels);
        #if DISPLAY_RGB332_DMA_PINGPONG
            dmaStageB = (uint16_t*)malloc(sizeof(uint16_t) * neededPixels);
        #endif

        #if DISPLAY_RGB332_DMA_PINGPONG
            if (dmaStageA == nullptr || dmaStageB == nullptr) {
        #else
            if (dmaStageA == nullptr) {
        #endif
            if (dmaStageA != nullptr) { free(dmaStageA); dmaStageA = nullptr; }
            #if DISPLAY_RGB332_DMA_PINGPONG
                if (dmaStageB != nullptr) { free(dmaStageB); dmaStageB = nullptr; }
            #endif
            return false;
        }

        dmaStageCapacityPixels = neededPixels;
        return true;
    }

    inline void convertRGB332ChunkTo565(const uint8_t* src, uint16_t* dst, uint32_t count) {
        while (count--) {
            *dst++ = rgb332_to_565[*src++];
        }
    }

    #if DISPLAY_RGB332_DIRTY_FLUSH
    inline void markDirtyRect(int x, int y, int w, int h) {
        if (w <= 0 || h <= 0) return;

        int maxW = width();
        int maxH = height();
        if (maxW <= 0 || maxH <= 0) return;

        int x0 = x;
        int y0 = y;
        int x1 = x + w - 1;
        int y1 = y + h - 1;

        if (x1 < 0 || y1 < 0 || x0 >= maxW || y0 >= maxH) return;

        if (x0 < 0) x0 = 0;
        if (y0 < 0) y0 = 0;
        if (x1 >= maxW) x1 = maxW - 1;
        if (y1 >= maxH) y1 = maxH - 1;

        if (!dirtyPending) {
            dirtyPending = true;
            dirtyX0 = x0;
            dirtyY0 = y0;
            dirtyX1 = x1;
            dirtyY1 = y1;
            return;
        }

        if (x0 < dirtyX0) dirtyX0 = x0;
        if (y0 < dirtyY0) dirtyY0 = y0;
        if (x1 > dirtyX1) dirtyX1 = x1;
        if (y1 > dirtyY1) dirtyY1 = y1;
    }

    inline void markDirtyFull() {
        int maxW = width();
        int maxH = height();
        if (maxW <= 0 || maxH <= 0) return;
        dirtyPending = true;
        dirtyX0 = 0;
        dirtyY0 = 0;
        dirtyX1 = maxW - 1;
        dirtyY1 = maxH - 1;
    }

    inline void clearDirty() {
        dirtyPending = false;
    }
    #endif
    #endif

    // for this translator (Adafruit GFX + GFX_Buffer), we seem to need to initialise dynamically instead of statically
    virtual void init() override {
        this->setup();
    }
    virtual void setup() {
        Debug_println(F("DisplayTranslator_Bodmer setup()..")); Serial_flush();

        real_actual_espi = new TFT_eSPI();
        actual = new TFT_eSprite_Wrapper(real_actual_espi);
        tft = actual;
        tft->init(); //SCREEN_WIDTH, SCREEN_HEIGHT);           // Init ST7789 240x135

        #if defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_ARCH_RP2350)
            // actual->setFreeFont(&TomThumb);
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

                #if defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_ARCH_RP2350)
                    real_actual_espi->initDMA();
                #endif
                real_actual_espi->setSwapBytes(false);
                #if defined(BODMER_SPRITE_8BIT) || defined(DISPLAY_RGB332_FB_MODE)
                    actual->setColorDepth(8);   // save RAM by using 8 bit color depth for the sprite?
                #endif
                #ifdef DISPLAY_RGB332_FB_MODE
                    initRGB332Lut();
                #endif
                void *spriteMem = nullptr;
                if(SCREEN_ROTATION%2==1) {
                    spriteMem = actual->createSprite(SCREEN_HEIGHT, SCREEN_WIDTH);   // < - corrupts display in way that i did find mentioned in that thread !
                    real_actual_espi->fillRect(0, 0, SCREEN_HEIGHT, SCREEN_WIDTH, BLACK);
                } else {
                    spriteMem = actual->createSprite(SCREEN_WIDTH, SCREEN_HEIGHT);   // < - corrupts display in way that i did find mentioned in that thread !
                    real_actual_espi->fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, BLACK);
                }
                #ifdef DISPLAY_RGB332_FB_MODE
                    sprPtr8 = (uint8_t*)spriteMem;
                    sprPtr = nullptr;
                #else
                    sprPtr = (uint16_t*)spriteMem;
                #endif
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

        #if defined(BODMER_SPRITE) && defined(DISPLAY_RGB332_FB_MODE) && DISPLAY_RGB332_DIRTY_FLUSH
            markDirtyFull();
        #endif

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
    virtual void println(const char *txt) override {
        tft->println(txt);
    }
    virtual void drawRoundRect(int x, int y, int w, int h, int radius, int color) override {
        tft->drawRoundRect(x, y, w, h, radius, color);
    }
    virtual void fillRoundRect(int x, int y, int w, int h, int radius, int color) override {
        tft->fillRoundRect(x, y, w, h, radius, color);
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
        #if defined(BODMER_SPRITE) && defined(DISPLAY_RGB332_FB_MODE) && DISPLAY_RGB332_DIRTY_FLUSH
            markDirtyFull();
        #endif
        //tft->fillRect(0, 0, tft->width(), tft->height(), BLACK);
    }

    // Dirty rectangle tracking methods for selective Y-range DMA push
    #if MENU_PERF_PARTIAL_UPDATES
    virtual void set_dirty_region(int y_min, int y_max) override {
        #ifdef BODMER_SPRITE
        if (y_min < dirty_y_min) dirty_y_min = y_min;
        if (y_max > dirty_y_max) dirty_y_max = y_max;
        #endif
    }
    
    virtual void reset_dirty_region() override {
        #ifdef BODMER_SPRITE
        dirty_y_min = INT_MAX;
        dirty_y_max = 0;
        #endif
    }
    
    virtual bool has_dirty_region() const override {
        #ifdef BODMER_SPRITE
        return dirty_y_max > dirty_y_min;
        #else
        return false;
        #endif
    }
    #endif // MENU_PERF_PARTIAL_UPDATES

    virtual void start() override {
        DisplayTranslator::start();
        Debug_println("Display start.."); Serial_flush();
        //tft->updateScreenAsync(false);
    }

    virtual void updateDisplay() override {
        #ifdef BODMER_SPRITE
        //spr.fillSprite(random(0,pow(2,16)));
            if (this->ready()) {
                // Serial.println("updateDisplay sprite mode");
                int s_w, s_h;
                if (SCREEN_ROTATION%2==1) {
                    s_w = SCREEN_HEIGHT;
                    s_h = SCREEN_WIDTH;
                } else {
                    s_w = SCREEN_WIDTH;
                    s_h = SCREEN_HEIGHT;
                }

                #ifdef DISPLAY_RGB332_FB_MODE
                    if (sprPtr8 == nullptr) return;

                    #if DISPLAY_RGB332_DIRTY_FLUSH
                        if (!dirtyPending) return;
                    #endif

                    int flushX = 0;
                    int flushY = 0;
                    int flushW = s_w;
                    int flushH = s_h;

                    #if DISPLAY_RGB332_DIRTY_FLUSH
                        flushX = dirtyX0;
                        flushY = dirtyY0;
                        flushW = dirtyX1 - dirtyX0 + 1;
                        flushH = dirtyY1 - dirtyY0 + 1;
                        if (flushW <= 0 || flushH <= 0) {
                            clearDirty();
                            return;
                        }
                    #endif

                    #if defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_ARCH_RP2350)
                        if (!ensureDMAStageBuffers(flushW)) return;

                        uint32_t y = (uint32_t)flushY;
                        const uint32_t endY = (uint32_t)(flushY + flushH);
                        const uint32_t chunkLines = DISPLAY_RGB332_DMA_CHUNK_LINES > 0 ? DISPLAY_RGB332_DMA_CHUNK_LINES : 1;

                        #if DISPLAY_RGB332_DMA_PINGPONG
                            bool useA = true;
                            bool dmaInFlight = false;
                            while (y < endY) {
                                uint32_t lines = endY - y;
                                if (lines > chunkLines) lines = chunkLines;

                                uint16_t *dst = useA ? dmaStageA : dmaStageB;
                                uint32_t row = 0;
                                while (row < lines) {
                                    const uint8_t *srcRow = sprPtr8 + ((y + row) * (uint32_t)s_w) + (uint32_t)flushX;
                                    uint16_t *dstRow = dst + (row * (uint32_t)flushW);
                                    convertRGB332ChunkTo565(srcRow, dstRow, (uint32_t)flushW);
                                    row++;
                                }

                                if (dmaInFlight) {
                                    while (real_actual_espi->dmaBusy()) {
                                    }
                                }

                                real_actual_espi->pushImageDMA(flushX, y, flushW, lines, dst);
                                dmaInFlight = true;
                                useA = !useA;
                                y += lines;
                            }

                            // Leave the final chunk in-flight to reduce frame push blocking.
                            // push_display() already gates next flush via ready() / dmaBusy().
                        #else
                            while (y < endY) {
                                uint32_t lines = endY - y;
                                if (lines > chunkLines) lines = chunkLines;

                                while (real_actual_espi->dmaBusy()) {
                                }

                                uint16_t *dst = dmaStageA;
                                uint32_t row = 0;
                                while (row < lines) {
                                    const uint8_t *srcRow = sprPtr8 + ((y + row) * (uint32_t)s_w) + (uint32_t)flushX;
                                    uint16_t *dstRow = dst + (row * (uint32_t)flushW);
                                    convertRGB332ChunkTo565(srcRow, dstRow, (uint32_t)flushW);
                                    row++;
                                }
                                real_actual_espi->pushImageDMA(flushX, y, flushW, lines, dst);

                                y += lines;
                            }
                        #endif

                    #else
                        if (!ensureDMAStageBuffers(flushW)) return;

                        uint32_t y = (uint32_t)flushY;
                        const uint32_t endY = (uint32_t)(flushY + flushH);
                        const uint32_t chunkLines = DISPLAY_RGB332_DMA_CHUNK_LINES > 0 ? DISPLAY_RGB332_DMA_CHUNK_LINES : 1;

                        while (y < endY) {
                            uint32_t lines = endY - y;
                            if (lines > chunkLines) lines = chunkLines;

                            uint16_t *dst = dmaStageA;
                            uint32_t row = 0;
                            while (row < lines) {
                                const uint8_t *srcRow = sprPtr8 + ((y + row) * (uint32_t)s_w) + (uint32_t)flushX;
                                uint16_t *dstRow = dst + (row * (uint32_t)flushW);
                                convertRGB332ChunkTo565(srcRow, dstRow, (uint32_t)flushW);
                                row++;
                            }
                            real_actual_espi->pushImage(flushX, y, flushW, lines, dst);

                            y += lines;
                        }
                    #endif

                    #if DISPLAY_RGB332_DIRTY_FLUSH
                        clearDirty();
                    #endif
                #else
                    #if defined(USE_DMA) || defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_ARCH_RP2350)
                        #if MENU_PERF_PARTIAL_UPDATES
                        // Selective Y-range DMA push — only transfer rows that were marked dirty.
                        if (has_dirty_region()) {
                            int push_y_min = constrain(dirty_y_min, 0, s_h - 1);
                            int push_y_max = constrain(dirty_y_max, push_y_min + 1, s_h);
                            int push_height = push_y_max - push_y_min;
                            if (push_height > 0) {
                                uint16_t *push_ptr = sprPtr + (push_y_min * s_w);
                                real_actual_espi->pushImageDMA(0, push_y_min, s_w, push_height, push_ptr);
                            }
                        } else {
                            real_actual_espi->pushImageDMA(0, 0, s_w, s_h, sprPtr);
                        }
                        #else
                        // Full-frame push (MENU_PERF_PARTIAL_UPDATES disabled).
                        real_actual_espi->pushImageDMA(0, 0, s_w, s_h, sprPtr);
                        #endif // MENU_PERF_PARTIAL_UPDATES
                    #else
                        // real_actual_espi->pushImage(0, 0, s_w, s_h, sprPtr);
                    #endif
                #endif
            }
        #endif
        //tft->updateScreenAsync(false);
        //tft->display();
        //tft->push
    }

    #ifdef BODMER_SPRITE
        virtual bool ready() override {
            #if defined(USE_DMA) || defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_ARCH_RP2350)
                return !real_actual_espi->dmaBusy();
            #else
                return true;
            #endif
        }
    #endif

    virtual void drawLine(int x0, int y0, int x1, int y1, uint16_t color) override {
        tft->drawLine(x0, y0, x1, y1, color);
        #if defined(BODMER_SPRITE) && defined(DISPLAY_RGB332_FB_MODE) && DISPLAY_RGB332_DIRTY_FLUSH
            int minX = x0 < x1 ? x0 : x1;
            int maxX = x0 > x1 ? x0 : x1;
            int minY = y0 < y1 ? y0 : y1;
            int maxY = y0 > y1 ? y0 : y1;
            markDirtyRect(minX, minY, (maxX - minX) + 1, (maxY - minY) + 1);
        #endif
    }
    virtual void drawFastVLine(int x, int y, int height, uint16_t colour) override {
        actual->drawFastVLine(x, y, height, colour);
        #if defined(BODMER_SPRITE) && defined(DISPLAY_RGB332_FB_MODE) && DISPLAY_RGB332_DIRTY_FLUSH
            markDirtyRect(x, y, 1, height);
        #endif
    }
    virtual void drawFastHLine(int x, int y, int width, uint16_t colour) override {
        actual->drawFastHLine(x, y, width, colour);
        #if defined(BODMER_SPRITE) && defined(DISPLAY_RGB332_FB_MODE) && DISPLAY_RGB332_DIRTY_FLUSH
            markDirtyRect(x, y, width, 1);
        #endif
    }

    virtual void fillRect(int x, int y, int w, int h, uint16_t color) override {
        tft->fillRect(x, y, w, h, color);
        #if defined(BODMER_SPRITE) && defined(DISPLAY_RGB332_FB_MODE) && DISPLAY_RGB332_DIRTY_FLUSH
            markDirtyRect(x, y, w, h);
        #endif
    }
    virtual void fillCircle(int x, int y, int radius, uint16_t colour) override {
        actual->fillCircle(x, y, radius, colour);
        #if defined(BODMER_SPRITE) && defined(DISPLAY_RGB332_FB_MODE) && DISPLAY_RGB332_DIRTY_FLUSH
            int d = radius * 2 + 1;
            markDirtyRect(x - radius, y - radius, d, d);
        #endif
    }
    virtual void drawRect(int x, int y, int w, int h, uint16_t color) override {
        actual->drawRect(x, y, w, h, color);
        #if defined(BODMER_SPRITE) && defined(DISPLAY_RGB332_FB_MODE) && DISPLAY_RGB332_DIRTY_FLUSH
            markDirtyRect(x, y, w, h);
        #endif
    }

    /*virtual void drawRGBBitmap(int x, int y, GFXcanvas16 *c) {
        //tft->drawBitmap(c, x, y)
        tft->drawRGBBitmap(x, y, c->getBuffer(), c->width(), c->height());
    }*/

    // RLE-encoded framebuffer send (count+value pairs) for faster transmission
    void sendRawFrame() {
#ifdef DISPLAY_RGB332_FB_MODE
        Serial.print("==START-FRAME==");

        uint16_t w = width();
        uint16_t h = height();
        uint32_t total = (uint32_t)w * (uint32_t)h;
        uint8_t encoding = 0;
        uint32_t size = total * 2;

        Serial.write((uint8_t*)&w, 2);
        Serial.write((uint8_t*)&h, 2);
        Serial.write((uint8_t*)&encoding, 1);
        Serial.write((uint8_t*)&size, 4);

        if (!ensureDMAStageBuffers(w) || sprPtr8 == nullptr) {
            Serial.print("==END-FRAME==");
            return;
        }

        uint32_t y = 0;
        const uint32_t chunkLines = DISPLAY_RGB332_DMA_CHUNK_LINES > 0 ? DISPLAY_RGB332_DMA_CHUNK_LINES : 1;
        while (y < (uint32_t)h) {
            uint32_t lines = (uint32_t)h - y;
            if (lines > chunkLines) lines = chunkLines;

            const uint32_t pixels = lines * (uint32_t)w;
            const uint8_t *src = sprPtr8 + (y * (uint32_t)w);
            convertRGB332ChunkTo565(src, dmaStageA, pixels);
            Serial.write((uint8_t*)dmaStageA, pixels * sizeof(uint16_t));
            y += lines;
        }

        Serial.print("==END-FRAME==");
        return;
    #else

        // Send (optionally RLE-encoded) 16-bit framebuffer to host. Protocol:
        // [==START-FRAME==][w:2][h:2][encoding:1][size:4][payload:size bytes][==END-FRAME==]
        // encoding: 0 = raw (uncompressed 16-bit little-endian words),
        //           1 = RLE (pairs of [count:uint16][value:uint16])
        // Payload for RLE: repeated [count:2][value:2] pairs.
        Serial.print("==START-FRAME==");
        uint16_t w = width();
        uint16_t h = height();
        uint32_t total = (uint32_t)w * (uint32_t)h;

        // pointer to pixel data (16-bit words)
        uint16_t *pixels = (uint16_t*)actual->getPointer();

        // First pass: compute encoded payload size for RLE without allocating
        uint32_t encoded_bytes = 0;
        uint32_t i = 0;
        while (i < total) {
            uint16_t val = pixels[i];
            uint32_t run = 1;
            while (i + run < total && pixels[i + run] == val && run < 0xFFFF) {
                run++;
            }
            encoded_bytes += 2 + 2; // count (2 bytes) + value (2 bytes)
            i += run;
        }

        uint8_t encoding = 1; // assume RLE
        uint32_t size = encoded_bytes;
        uint32_t raw_bytes = total * 2;
        // If RLE doesn't help, send raw instead
        if (encoded_bytes >= raw_bytes) {
            encoding = 0;
            size = raw_bytes;
        }

        Serial.write((uint8_t*)&w, 2);
        Serial.write((uint8_t*)&h, 2);
        Serial.write((uint8_t*)&encoding, 1);
        Serial.write((uint8_t*)&size, 4);

        if (encoding == 0) {
            // send raw bytes (little-endian 16-bit words)
            uint8_t *ptr = (uint8_t*)actual->getPointer();
            uint32_t remaining = size;
            while (remaining > 0) {
                uint32_t chunk = remaining;
                Serial.write(ptr, chunk);
                ptr += chunk;
                remaining -= chunk;
            }
        } else {
            // Second pass: stream encoded runs
            i = 0;
            while (i < total) {
                uint16_t val = pixels[i];
                uint32_t run = 1;
                while (i + run < total && pixels[i + run] == val && run < 0xFFFF) {
                    run++;
                }
                uint16_t run16 = (uint16_t)run;
                Serial.write((uint8_t*)&run16, 2);
                Serial.write((uint8_t*)&val, 2);
                i += run;
            }
        }

        // end marker
        Serial.print("==END-FRAME==");
        //Serial.flush();
#endif
    }

    virtual void push_framebuffer_serial() override {
        // get the framebuffer data from the actual_espi and push it out over serial
        if (Serial) this->sendRawFrame();
    }
    virtual const char* viewer_pixel_format() override { return "BGR565"; }
};

#endif