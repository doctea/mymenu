#ifndef DISPLAYABSTRACT__INCLUDED
#define DISPLAYABSTRACT__INCLUDED

#include <Arduino.h>

#define C_WHITE     0xFF
#define BLACK       0x00
#define RED         0xFA
#define GREEN       0xAF
#define BLUE        0x00
#define YELLOW      0xF0

class DisplayTranslator {

    public:

    // seems like some tft devices need to be inited dynamically instead of statically, so allow for that
    virtual void init() {}; 

    virtual const char *get_message_format() { return "[%-20.20s]"; }
    virtual const char *get_header_format() { return "%-22s"; }
    virtual const char *get_header_open_format() { return ">>>%-19s"; }
    virtual const char *get_header_selected_format() { return "%-22s"; }
    virtual uint8_t get_c_max() {
        return 20;
    }

    /*const char *MESSAGE_FORMAT          = "[%-20s]";
    const char *HEADER_FORMAT           = "%-22s";
    const char *HEADER_OPEN_FORMAT      = ">>>%-19s";
    const char *HEADER_SELECTED_FORMAT  = "%-22s";*/

    DisplayTranslator() {};
    //~DisplayTranslator() {};

    virtual void setup() {};
    virtual void start() {};
    virtual void clear(bool force = false) {};

    virtual bool ready() { return true; };

    virtual void setCursor(int x, int y) {};
    virtual void print(const char *text) {};
    virtual int getCursorX() { return 0; };
    virtual int getCursorY() { return 0; };
    virtual void setTextColor(uint16_t fg, uint16_t bg) {};
    virtual void setTextSize(unsigned int size) {};
    virtual void printf(const char *pattern);
    virtual void printf(const char *pattern, char *param1);
    virtual void printf(const char *pattern, char *param1, char *param2);
    virtual void printf(const char *pattern, char *param1, char *param2, char *param3);
    virtual void printf(const char *pattern, char *param1, int param2, int param3);
    virtual void printf(const char *pattern, int param1, char* param2);
    virtual void printf(const char *pattern, int param1);
    virtual void printf(const char *pattern, int param1, int param2);
    virtual void printf(const char *pattern, int param1, int param2, int param3);
    virtual void printf(const char *pattern, int param1, int param2, int param3, float param4);
    virtual void printf(const char *pattern, int param1, const uint8_t *param2);
    virtual void printf(const char *pattern, char param1, int param2, char *param3);
    virtual void println();
    virtual void println(const char *txt);
    virtual void printc(char c);


    virtual int width() { return 128; };
    virtual int height() { return 64; };

    virtual int getRowHeight() { return 1; };
    virtual int characterWidth() { return 1; };

    virtual void updateDisplay() {};

    // by ktownsend from https://forums.adafruit.com/viewtopic.php?t=21536
    uint16_t rgb(uint8_t r, uint8_t g, uint8_t b);
    uint16_t rgb(uint32_t rgb);
    // take a 565 16-bit colour value and return a dimmed version of it
    uint16_t halfbright_565(uint16_t colour);
    virtual bool will_x_rows_fit_to_height(int rows, int height = -1);

    // shapes + lines
    virtual void drawLine(int x0, int y0, int x1, int y1, uint16_t color) {};
    virtual void drawFastVLine(int x, int y, int height, uint16_t colour) = 0;
    virtual void drawFastHLine(int x, int y, int width, uint16_t colour) = 0;
    virtual void fillRect(int x, int y, int w, int h, uint16_t color) {};
    virtual void drawRect(int x, int y, int w, int h, uint16_t color) {};
    virtual void drawRoundRect(int x, int y, int w, int h, int radius, int color) { Serial.println(F("TODO: unimplemented drawRoundRect())")); };
    virtual void fillRoundRect(int x, int y, int w, int h, int radius, int color) { Serial.println(F("TODO: unimplemented fillRoundRect())")); };
    virtual void fillCircle(int x, int y, int radius, uint16_t colour) = 0;

};


//DisplayTranslator::~DisplayTranslator() {}

#endif
