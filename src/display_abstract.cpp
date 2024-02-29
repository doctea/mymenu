#include "display_abstract.h"

void DisplayTranslator::printf(const char *pattern) { Serial.println(F("WARNING: DisplayTranslator::printf(char*pattern) not overridden!")); };
void DisplayTranslator::printf(const char *pattern, char *param1)  { Serial.println(F("WARNING: DisplayTranslator::printf(char*pattern, char *param1) not overridden!")); };
void DisplayTranslator::printf(const char *pattern, char *param1, char *param2) { Serial.println(F("WARNING: DisplayTranslator::printf(char*pattern, char *param1, char *param2) not overridden!")); };
void DisplayTranslator::printf(const char *pattern, char *param1, char *param2, char *param3) { Serial.println(F("WARNING: DisplayTranslator::printf(char*pattern, char *param1, char *param2, char *param3) not overridden!")); };
void DisplayTranslator::printf(const char *pattern, char *param1, int param2, int param3) { Serial.println(F("WARNING: DisplayTranslator::printf(char*pattern, char *param1, char *param2, char *param3) not overridden!")); };
void DisplayTranslator::printf(const char *pattern, int param1, char* param2) { Serial.println(F("WARNING: DisplayTranslator::printf(char *pattern, int param1, char* param2) not overridden!")); }
void DisplayTranslator::printf(const char *pattern, int param1) { Serial.println(F("WARNING: DisplayTranslator::printf(char *pattern, int param1) not overridden!")); }
void DisplayTranslator::printf(const char *pattern, int param1, int param2) { Serial.println(F("WARNING: DisplayTranslator::printf(char *pattern, int param1, int param2) not overridden!")); } 
void DisplayTranslator::printf(const char *pattern, int param1, int param2, int param3) { Serial.println(F("WARNING: DisplayTranslator::printf(char *pattern, int param1, int param2, int param3) not overridden!")); } 
void DisplayTranslator::printf(const char *pattern, int param1, int param2, int param3, float param4) { Serial.println(F("WARNING: DisplayTranslator::printf(char *pattern, int param1, int param2, int param3, float param4) not overridden!")); } 
void DisplayTranslator::printf(const char *pattern, int param1, const uint8_t *param2) { Serial.println(F("WARNING: DisplayTranslator::printf(char *pattern, int param1, const uint8_t *param2) not overridden!")); }
void DisplayTranslator::printf(const char *pattern, char param1, int param2, char *param3) { Serial.println(F("WARNING: DisplayTranslator::printf(char *pattern, int param1, const uint8_t *param2) not overridden!")); }
void DisplayTranslator::println() { Serial.println(F("WARNING: DisplayTranslator::println(void) not overridden!")); };
void DisplayTranslator::println(const char *txt) { Serial.println(F("WARNING: DisplayTranslator::println(const char *) not overridden!"));};
void DisplayTranslator::printc(char c) { Serial.println(F("WARNING: DisplayTranslator::printc(char) not overridden!")); }

// by ktownsend from https://forums.adafruit.com/viewtopic.php?t=21536
uint16_t DisplayTranslator::rgb(uint8_t r, uint8_t g, uint8_t b) {
    return ((r / 8) << 11) | ((g / 4) << 5) | (b / 8);
}
uint16_t DisplayTranslator::rgb(uint32_t rgb) {
    uint8_t r = rgb>>16;
    uint8_t g = rgb>>8;
    uint8_t b = rgb & 0b11111111;
    return ((r / 8) << 11) | ((g / 4) << 5) | (b / 8);
}

// take a 565 16-bit colour value and return a dimmed version of it
uint16_t DisplayTranslator::halfbright_565(uint16_t colour) {
    unsigned char red = (colour & 0xf800) >> 11;
    unsigned char green = (colour & 0x07e0) >> 5;
    unsigned char blue = colour & 0x001f;
    //Serial.printf("halfbright took colour %04x and split into red=%i, green=%i, blue=%i ", colour, red, green, blue);
    red = red >> 1;
    green = green >> 1;
    blue = green >> 1;
    uint16_t ret = rgb(red<<3,green<<2,blue<<3);
    //Serial.printf("and returning %04x\n", ret);
    return ret;
}

uint16_t DisplayTranslator::dim_565(uint16_t colour, int8_t dim_level) {
    unsigned char red = (colour & 0xf800) >> 11;
    unsigned char green = (colour & 0x07e0) >> 5;
    unsigned char blue = colour & 0x001f;
    //Serial.printf("halfbright took colour %04x and split into red=%i, green=%i, blue=%i ", colour, red, green, blue);
    red = red >> min(3, dim_level);
    green = green >> min(2, dim_level);
    blue = green >> min(3, dim_level);
    uint16_t ret = rgb(red<<3,green<<2,blue<<3);
    //Serial.printf("and returning %04x\n", ret);
    return ret;
}

bool DisplayTranslator::will_x_rows_fit_to_height(int rows, int height) {
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