#if (defined __GNUC__) && (__GNUC__ >= 5) && (__GNUC_MINOR__ >= 4) && (__GNUC_PATCHLEVEL__ > 1)
    #pragma GCC diagnostic ignored "-Wpragmas"
    #pragma GCC diagnostic ignored "-Wformat-truncation"
    #pragma GCC diagnostic ignored "-Wstringop-truncation"
#endif

#pragma once

#include <Arduino.h>
#include <stdarg.h>

#define C_WHITE     0xFF
#define BLACK       0x00
#define RED         0xFA
#define GREEN       0xAF
#define BLUE        0x00
#define YELLOW      0xF0

const int message_max = 20;

#define tft_print_column(format, ...) { int __x = tft->getCursorX(); tft->printf(format, __VA_ARGS__); tft->setCursor(__x, tft->getCursorY()); }
class DisplayTranslator {
    public:

    unsigned int default_textsize = 0;
    unsigned int maximum_textsize = 3;
    unsigned int size = default_textsize;

    // seems like some tft devices need to be inited dynamically instead of statically, so allow for that
    virtual void init() {}; 

    int row_character_width = 22;
    char message_format[message_max] = "[%-20.20s]";
    char header_format[message_max] = "%-22s";
    char header_open_format[message_max] = ">>>%-19s";
    char header_selected_format[message_max] = "%-22s";
    // recreate string formats based on current textsize
    virtual void setup_formats() {
        this->row_character_width = this->get_row_character_width();
        const uint8_t message_width = (row_character_width > 2) ? (uint8_t)(row_character_width - 2) : 1;
        snprintf(message_format,         message_max, "[%%-%i.%is]", message_width, message_width);
        snprintf(header_format,          message_max, "%%-%is",      (uint8_t)row_character_width);
        snprintf(header_open_format,     message_max, ">>>%%-%is",   (uint8_t)row_character_width-3);
        snprintf(header_selected_format, message_max, "%%-%is",      (uint8_t)row_character_width);
    }
    virtual int get_row_character_width() { return (this->width() / ((this->default_textsize+1) * this->characterWidth())) + 1; }
    virtual const char *get_message_format()         { return message_format; }
    virtual const char *get_header_format()          { return header_format; }
    virtual const char *get_header_open_format()     { return header_open_format; }
    virtual const char *get_header_selected_format() { return header_selected_format; }

    virtual void setTextWrap(bool enable_wrap) = 0;
    virtual bool isTextWrap() = 0;

    // set the base textsize to use (and recreate the string formats based on that size)
    virtual void set_default_textsize(unsigned int textsize) {
        this->default_textsize = textsize;
        this->setup_formats();
    }
    virtual unsigned int get_default_textsize() {
        return this->default_textsize;
    }
    virtual void set_maximum_textsize(unsigned int textsize) {
        this->maximum_textsize = textsize;
        this->setup_formats();
    }

    virtual unsigned int get_textsize_for_width(const char *text, unsigned int pixel_width) {
        //((int)strlen(text_to_render)*tft->currentCharacterWidth() < tft->width()/2);
        //return constrain(pixel_width / (characterWidth() * strlen(text)), default_textsize, 3);
        return constrain(
            pixel_width / (characterWidth() * strlen(text)), 
            (unsigned int)0, 
            default_textsize+maximum_textsize
        );
    }

    virtual uint8_t get_c_max() {
        //return 20;
        return row_character_width;
    }

    /*const char *MESSAGE_FORMAT          = "[%-20s]";
    const char *HEADER_FORMAT           = "%-22s";
    const char *HEADER_OPEN_FORMAT      = ">>>%-19s";
    const char *HEADER_SELECTED_FORMAT  = "%-22s";*/

    DisplayTranslator() {};
    //~DisplayTranslator() {};

    virtual void setup() {};
    virtual void start() {
        this->set_default_textsize(this->default_textsize);
        this->set_maximum_textsize((this->default_textsize+1) * 2);
    };
    virtual void clear(bool force = false) {};

    virtual bool ready() { return true; };

    virtual void setCursor(int x, int y) {};
    virtual void print(const char *text) {};
    virtual int getCursorX() { return 0; };
    virtual int getCursorY() { return 0; };
    virtual void setTextColor(uint16_t fg, uint16_t bg) {};
    virtual void setTextSize(int size) {};
    virtual int getTextSize() { return size; };
    virtual void printf(const char *format, ...) __attribute__((format(printf, 2, 3)));
    virtual void println();
    virtual void println(const char *txt);
    virtual void printc(char c);


    virtual int width() { return 128; };
    virtual int height() { return 64; };

    virtual int getSingleRowHeight() {
        return 1;
    }
    virtual int getRowHeight() { return getSingleRowHeight() * (1+this->getTextSize()); }
    virtual int characterWidth() { return 1; };
    virtual int currentCharacterWidth() {
        return characterWidth() * (this->size>0?this->size:1);
    }

    virtual void updateDisplay() {};

    // ---------------------------------------------------------------------------
    // Partial-update dirty region tracking.
    // Guarded by MENU_PERF_PARTIAL_UPDATES.
    //
    // Contract for implementors:
    //   set_dirty_region(y_min, y_max) — expand the tracked dirty Y-range to include [y_min, y_max).
    //     Must be idempotent/accumulative (union, not replace).
    //   reset_dirty_region()           — clear the tracked range, called at the start of each frame.
    //   has_dirty_region()             — returns true if any region has been marked dirty this frame.
    //
    // Future extension: mark_dirty_rect(x, y, w, h) for sub-row granularity.
    // Recommended implementation strategy: tile-grid bitmask (e.g. 16×16 tiles).
    #if MENU_PERF_PARTIAL_UPDATES
        virtual void set_dirty_region(int y_min, int y_max) {};
        virtual void reset_dirty_region() {};
        virtual bool has_dirty_region() const { return false; };
    #endif

    // by ktownsend from https://forums.adafruit.com/viewtopic.php?t=21536
    // get a 565-format colour for an rgb value
    uint16_t rgb(uint8_t r, uint8_t g, uint8_t b);
    uint16_t rgb(uint32_t rgb);
    // take a 565 16-bit colour value and return a dimmed version of it
    uint16_t halfbright_565(uint16_t colour);
    // dim_level should be 1-3, 1 = brightest, 3 = maximum dimming
    uint16_t dim_565(uint16_t colour, int8_t dim_level);

    virtual bool will_x_rows_fit_to_height(int rows, int height = -1);

    // shapes + lines
    virtual void drawLine(int x0, int y0, int x1, int y1, uint16_t color) = 0;
    virtual void drawFastVLine(int x, int y, int height, uint16_t colour) = 0;
    virtual void drawFastHLine(int x, int y, int width, uint16_t colour) = 0;
    virtual void fillRect(int x, int y, int w, int h, uint16_t color) = 0;
    virtual void drawRect(int x, int y, int w, int h, uint16_t color) = 0;
    virtual void drawRoundRect(int x, int y, int w, int h, int radius, int color) = 0; //{ Serial.println(F("TODO: unimplemented drawRoundRect())")); };
    virtual void fillRoundRect(int x, int y, int w, int h, int radius, int color) = 0; //{ Serial.println(F("TODO: unimplemented fillRoundRect())")); };
    virtual void fillCircle(int x, int y, int radius, uint16_t colour) = 0;

    virtual void push_framebuffer_serial() {};
    // Pixel colour order of the framebuffer (used by the remote viewer for correct colour decode).
    // Override in display subclasses if the framebuffer uses BGR565 order (e.g. TFT_eSPI/Bodmer).
    virtual const char* viewer_pixel_format() { return "RGB565"; };

};

