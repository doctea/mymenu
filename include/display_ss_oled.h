#ifndef DISPLAY_SS_OLED__INCLUDED
#define DISPLAY_SS_OLED__INCLUDED

#ifdef TFT_SSOLED

#include "display_abstract.h"

#include "ss_oled.h"

extern SSOLED ssoled;

#ifndef OLED_SDA_PIN
#define OLED_SDA_PIN -1
#endif
#ifndef OLED_SCL_PIN
#define OLED_SCL_PIN -1
#endif
// Set this to -1 to disable or the GPIO pin number connected to the reset
// line of your display if it requires an external reset
#ifndef OLED_RESET_PIN
#define OLED_RESET_PIN -1
#endif
#ifndef OLED_ADDR
// let ss_oled figure out the display address
#define OLED_ADDR 0x3C
#endif

// don't rotate the display
#define FLIP180 0
// don't invert the display
#define INVERT 0
// Bit-Bang the I2C bus
#define USE_HW_I2C 1

// Change this if you're using different OLED displays
#define MY_OLED OLED_128x64

// The SSOLED structure. Each structure is about 56 bytes
// There is no limit to the number of simultaneous displays which can be controlled by ss_oled 

class DisplayTranslator_SS_OLED : public DisplayTranslator {
    public:
    SSOLED ssoled;
    SSOLED *tft;
    uint8_t ucBackBuffer[1024];

    virtual const char *get_message_format() { return "[%-19s]"; }
    virtual const char *get_header_format() { return "%-19s"; }
    virtual const char *get_header_open_format() { return ">>>%-18s"; }
    virtual const char *get_header_selected_format() { return "%-19s"; }

    DisplayTranslator_SS_OLED() {
        this->tft = &ssoled;
        this->tft = tft;
        this->setup();
    }

    virtual void setup() override {
        int rc;
        char *msgs[] = {(char *)"SSD1306 @ 0x3C", (char *)"SSD1306 @ 0x3D",(char *)"SH1106 @ 0x3C",(char *)"SH1106 @ 0x3D"};

        rc = oledInit(tft, MY_OLED, OLED_ADDR, FLIP180, INVERT, USE_HW_I2C, OLED_SDA_PIN, OLED_SCL_PIN, OLED_RESET_PIN, 800000L); // use standard I2C bus at 400Khz
        if (rc != OLED_NOT_FOUND) {
            oledFill(tft, 0, 1);
            oledWriteString(tft, 0,0,0,msgs[rc], FONT_NORMAL, 0, 1);
            delay(2000);
        } else {
            while (1) {};
        }
        oledSetBackBuffer(tft, ucBackBuffer);
        oledSetTextWrap(&ssoled, true);
    }

    int x,y;
    int textSize = 1;
    bool bInvert = false;
    virtual void setCursor(int x, int y) override {
        this->x = x;
        this->y = y;
        oledSetCursor(tft, x, y);
    }
    virtual void print(char *text) override {
        oledWriteString(tft, 0, x, y, text, textSize, bInvert, 1);
        x = tft->iCursorX;
        y = tft->iCursorY;
    }
    virtual int getCursorX() override {
        return tft->iCursorX;
    }
    virtual int getCursorY() override {
        return tft->iCursorY;
    }
    virtual void setTextColor(uint16_t fg, uint16_t bg) override {
        //tft.setTextColor(fg, bg);
        // TODO: monochrome display, do nothing?  maybe invert?
        //Serial.printf("setTextColor(%02x, %02x) vs black:%02x and white:%02x\n", fg, bg, BLACK, C_WHITE);
        if (fg==BLACK) { //} && bg==C_WHITE) {
            //Serial.println("!!!!!!!!!!!!!!!!!!!!!!!!INVERTING!!!!!!!!!!!!!!!!!!!!!!!!");
            //Serial.printf("black-on-white: setTextColor(%02x, %02x) vs %02x and %02x\n", fg, bg, BLACK, C_WHITE);
            bInvert = true;
        } else if (/*fg==C_WHITE &&*/ bg==BLACK) {
            //Serial.printf("white-on-black: setTextColor(%02x, %02x) vs %02x and %02x\n", fg, bg, this->BLACK, this->WHITE);
            bInvert = false;
        } else
            bInvert = false; 
    }
    virtual void drawLine(int x1, int y1, int x2, int y2, uint16_t color) override {
        //tft.drawLine(x, y, w, h, color);
        oledDrawLine(tft, x1, y1, x2, y2, 1);
    }
    virtual void fillRect(int x1, int y1, int w, int h, uint16_t color) override {
        //tft.fillRect(x, y, w, h, color);
        int ucColor = 0xFF;
        oledRectangle(tft, x1, y1, w-x1, h-y1, ucColor, true);
    }
    virtual void setTextSize(int size) override {
        textSize = size;
    }
    virtual void printf(const char *text) override {
        //char tmp[255];
        //sprintf(tmp, pattern);
        //tft.printf(pattern, param1);
        //char *tmp = pattern;
        //Serial.printf("ss_oled->printf(\"%s\")\n", pattern);
        char newtext[30] = "                       ";
        char tmp[100] = "                                 ";
        sprintf(newtext, "%s", text);

        // todo: fix newline character display issue...
        bool newline = newtext[strlen(newtext)-1]=='\n';
        if (newline) newtext[strlen(newtext)-1] = '\0';
        
        sprintf(tmp,"%-21s", newtext);   // limit to width and ensure space at end of string is blanked out?
        oledWriteString(&ssoled, 0, tft->iCursorX, tft->iCursorY, tmp, textSize, bInvert, 1);
        if (newline && tft->iCursorX>0) { //}) { //tmp[strlen(tmp)-1] == '\n') { //} || tft->iCursorX>ssoled.oled_x) {
        //if (tmp[strlen(tmp)-1] == '\n') { //} || tft->iCursorX>ssoled.oled_x) {
            //Serial.println("moving line due to a \\n");
            setCursor(0,tft->iCursorY+(1+textSize));
        }
    }
    virtual void printf(const char *pattern, char *param1) override {
        char tmp[255];
        sprintf(tmp, pattern, param1);
        //tft.printf(pattern, param1);
        printf(tmp);
        //oledWriteString(&ssoled, 0, tft->iCursorX, tft->iCursorY, tmp, textSize, bInvert, 1);
    }
    virtual void printf(const char *pattern, char *param1, char *param2) override {
        char tmp[255];
        sprintf(tmp, pattern, param1, param2);
        //tft.printf(pattern, param1);
        printf(tmp);
        //oledWriteString(&ssoled, 0, tft->iCursorX, tft->iCursorY, tmp, textSize, bInvert, 1);
    }
    virtual void printf(const char *pattern, int param1) override {
        char tmp[255];
        sprintf(tmp, pattern, param1);
        printf(tmp);
        //oledWriteString(&ssoled, 0, tft->iCursorX, tft->iCursorY, tmp, textSize, bInvert, 1);
    }
    virtual void printf(const char *pattern, int param1, int param2) override {
        char tmp[255];
        sprintf(tmp, pattern, param1, param2);
        printf(tmp);
        //oledWriteString(&ssoled, 0, tft->iCursorX, tft->iCursorY, tmp, textSize, bInvert, 1);
    }
    virtual void printf(const char *pattern, int param1, int param2, int param3) override {
        char tmp[255];
        sprintf(tmp, pattern, param1, param2, param3);
        printf(tmp);
        //oledWriteString(&ssoled, 0, tft->iCursorX, tft->iCursorY, tmp, textSize, bInvert, 1);
    }
    virtual void printf(const char *pattern, int param1, int param2, int param3, float param4) override {
        char tmp[255];
        sprintf(tmp, pattern, param1, param2, param3, param4);
        printf(tmp);
        //oledWriteString(&ssoled, 0, tft->iCursorX, tft->iCursorY, tmp, textSize, bInvert, 1);
    }
    virtual void printf(const char *pattern, int param1, char* param2) override {
        char tmp[255];
        sprintf(tmp, pattern, param1);
        printf(tmp);
    }
    virtual void printf(const char *pattern, int param1, const uint8_t *param2) override {
        char tmp[255];
        sprintf(tmp, pattern, param1);
        printf(tmp);
    }
    virtual void printf(const char *pattern, char *param1, int param2, int param3) {
        char tmp[255];
        sprintf(tmp, pattern, param1, param2, param3);
        printf(tmp);
    }


    virtual void println(const char *txt) override {
        printf(txt);
        //oledWriteString(&ssoled, 0, tft->iCursorX, tft->iCursorY, (char*)txt, textSize, bInvert, 1);
        if (txt[strlen(txt)-1]!='\n')
            oledSetCursor(tft, 0, tft->iCursorY+(1+textSize));
    }

    virtual int width() override {
        return 128; //tft->width();
    }
    virtual int height() override {
        return 7; //tft->height(); // 64?
    }

    virtual int getRowHeight() override {
        return this->textSize+1;
    }

    virtual void clear(bool force) override {
        //Serial.println("clear..");
        /*oledFill(tft, 0, 0);
        oledRectangle(tft, 0, 0, tft->oled_x, tft->oled_y, 0, 1);*/
        memset(&ucBackBuffer, (uint8_t)0, 1024);
        if (force)
            oledFill(tft, 0, true);
        //tft->fillScreen(tft->BLACK);
    }


    virtual void updateDisplay() override {
        //tft->updateScreenAsync(false);
        //menu->display();
    }

};

/*void tft_print(char *text, int x, int y) {
    //oledWriteString(&ssoled, x, y, 0, text, FONT_NORMAL, 0, 1);
    oledFill(&ssoled, 0, 1);
    oledWriteString(&ssoled, 0, x, y, text, FONT_NORMAL, random(1), 1);
}
*/

/*void tft_print (char *text) {
    tft->print(text);
}*/


#endif
#endif