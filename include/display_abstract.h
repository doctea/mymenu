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

    virtual void setCursor(int x, int y) {};
    virtual void print(const char *text) {};
    virtual int getCursorX() { return 0; };
    virtual int getCursorY() { return 0; };
    virtual void setTextColor(uint16_t fg, uint16_t bg) {};
    virtual void drawLine(int x0, int y0, int x1, int y1, uint16_t color) {};
    virtual void fillRect(int x, int y, int w, int h, uint16_t color) {};
    virtual void setTextSize(unsigned int size) {};
    virtual void printf(const char *pattern) { Serial.println(F("WARNING: printf(char*pattern) not overridden!")); };
    virtual void printf(const char *pattern, char *param1)  { Serial.println(F("WARNING: printf(char*pattern, char *param1) not overridden!")); };
    virtual void printf(const char *pattern, char *param1, char *param2) { Serial.println(F("WARNING: printf(char*pattern, char *param1, char *param2) not overridden!")); };
    virtual void printf(const char *pattern, char *param1, char *param2, char *param3) { Serial.println(F("WARNING: printf(char*pattern, char *param1, char *param2, char *param3) not overridden!")); };
    virtual void printf(const char *pattern, char *param1, int param2, int param3) { Serial.println(F("WARNING: printf(char*pattern, char *param1, char *param2, char *param3) not overridden!")); };
    virtual void printf(const char *pattern, int param1, char* param2) { Serial.println(F("WARNING: printf(char *pattern, int param1, char* param2) not overridden!")); }
    virtual void printf(const char *pattern, int param1) { Serial.println(F("WARNING: printf(char *pattern, int param1) not overridden!")); }
    virtual void printf(const char *pattern, int param1, int param2) { Serial.println(F("WARNING: printf(char *pattern, int param1, int param2) not overridden!")); } 
    virtual void printf(const char *pattern, int param1, int param2, int param3) { Serial.println(F("WARNING: printf(char *pattern, int param1, int param2, int param3) not overridden!")); } 
    virtual void printf(const char *pattern, int param1, int param2, int param3, float param4) { Serial.println(F("WARNING: printf(char *pattern, int param1, int param2, int param3, float param4) not overridden!")); } 
    virtual void printf(const char *pattern, int param1, const uint8_t *param2) { Serial.println(F("WARNING: printf(char *pattern, int param1, const uint8_t *param2) not overridden!")); }
    virtual void printf(const char *pattern, char param1, int param2, char *param3) { Serial.println(F("WARNING: printf(char *pattern, int param1, const uint8_t *param2) not overridden!")); }
    virtual void println() { Serial.println(F("WARNING: println(void) not overridden!")); };
    virtual void println(const char *txt) { Serial.println(F("WARNING: println(const char *) not overridden!"));};
    virtual void printc(char c) { Serial.println(F("WARNING: printc(char) not overridden!")); }

    virtual void drawRoundRect(int x, int y, int w, int h, int radius, int color) { Serial.println(F("TODO: unimplemented drawRoundRect())")); };
    virtual void fillRoundRect(int x, int y, int w, int h, int radius, int color) { Serial.println(F("TODO: unimplemented fillRoundRect())")); };

    virtual int width() { return 128; };
    virtual int height() { return 64; };

    virtual int getRowHeight() { return 1; };
    virtual int characterWidth() { return 1; };

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

    virtual bool will_x_rows_fit_to_height(int rows, int height = -1) {
        if (height==-1) height = this->height();
        int rowHeight = this->getRowHeight();
        
        int position = getCursorY() + (rows*rowHeight);
        bool result = position <= height;
        /*if (result) 
            Serial.printf("will_x_rows_fit_to_height: current Y is %i, rows %i, rowHeight %i, (so Y+rows*rowHeight=%i) -> will fit in height %i?\n", 
                getCursorY(), rows, rowHeight, position, height);
        else
            Serial.printf("will_x_rows_fit_to_height: current Y is %i, rows %i, rowHeight %i, (so Y+rows*rowHeight=%i) -> WON'T fit in height %i?\n", 
                getCursorY(), rows, rowHeight, position, height);*/
        return result;
    } 
};


//DisplayTranslator::~DisplayTranslator() {}

#endif