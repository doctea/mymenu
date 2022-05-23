#ifndef DISPLAYABSTRACT__INCLUDED
#define DISPLAYABSTRACT__INCLUDED

#include <Arduino.h>

class DisplayTranslator {

    public:

    int GREEN = 0x7F;
    int WHITE = 0xFF;
    int BLACK = 0x00;
    int RED   = 0xAF;
    int BLUE  = 0xA7;

    DisplayTranslator() {};
    //~DisplayTranslator() {};

    virtual void setup() {};

    virtual void clear() {};

    virtual void setCursor(int x, int y) {};
    virtual void print(char *text) {};
    virtual int getCursorX() {};
    virtual int getCursorY() {};
    virtual void setTextColor(uint16_t fg,  uint16_t bg) {};
    virtual void drawLine(int x, int y, int w, int h, uint16_t color) {};
    virtual void fillRect(int x, int y, int w, int h, uint16_t color) {};
    virtual void setTextSize(int size) {};
    virtual void printf(const char *) {};
    virtual void printf(const char *pattern, const char *param1) {};
    virtual void printf(const char *pattern, const char *param1, const char *param2) {};
    virtual void printf(const char *pattern, int param1) {};
    virtual void printf(const char *pattern, int param1, int param2) {};
    virtual void printf(const char *pattern, int param1, int param2, int param3) {};
    virtual void printf(const char *pattern, int param1, int param2, int param3, float param4) {};
    virtual void println(const char *txt) {};

    virtual int width() {};
    virtual int height() {};

    virtual void updateDisplay() {};

    // by ktownsend from https://forums.adafruit.com/viewtopic.php?t=21536
    uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
        return ((r / 8) << 11) | ((g / 4) << 5) | (b / 8);
    }
    uint16_t rgb(uint32_t rgb) {
        uint8_t r = rgb>>16;
        uint8_t g = rgb>>8;
        uint8_t b = rgb & 0b11111111;
        return ((r / 8) << 11) | ((g / 4) << 5) | (b / 8);
    }

};


//DisplayTranslator::~DisplayTranslator() {}

#endif