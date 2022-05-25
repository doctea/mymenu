#ifndef DISPLAYABSTRACT__INCLUDED
#define DISPLAYABSTRACT__INCLUDED

#include <Arduino.h>


#define C_WHITE   0xFFFF
#define BLACK   0x0000
#define RED     0xFA00
#define GREEN   0xAFF0
#define BLUE    0x00AF
#define YELLOW  0xF00F

class DisplayTranslator {

    public:

    DisplayTranslator() {};
    //~DisplayTranslator() {};

    virtual void setup() {};
    virtual void start() {};
    virtual void clear() {};

    virtual void setCursor(int x, int y) {};
    virtual void print(char *text) {};
    virtual int getCursorX() {};
    virtual int getCursorY() {};
    virtual void setTextColor(uint16_t fg,  uint16_t bg) {};
    virtual void drawLine(int x, int y, int w, int h, uint16_t color) {};
    virtual void fillRect(int x, int y, int w, int h, uint16_t color) {};
    virtual void setTextSize(int size) {};
    virtual void printf(const char *pattern) {};
    virtual void printf(const char *pattern, char *param1) {};
    virtual void printf(const char *pattern, char *param1, char *param2) {};
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