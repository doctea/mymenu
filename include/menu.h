#ifndef MENU__INCLUDED
#define MENU__INCLUDED

#include "mymenu.h"

//extern Menu menu;
//void setup_display();

#include <LinkedList.h>
#include "menuitems.h"

class Menu {
    int currently_selected  = -1;
    int currently_opened    = -1;
    LinkedList<MenuItem*> items = LinkedList<MenuItem*>();

    int panel_height[20];

    int last_knob_position;
    int button_count;

    void knob_turned(int knob_position) {
        if (knob_position < last_knob_position) {
            //set_bpm(bpm_current-1);
            knob_left();
        } else if (knob_position > last_knob_position) {
            //set_bpm(bpm_current+1);
            knob_right();
        }
        last_knob_position = knob_position;
        // do some action when knob is turned
    }
    bool knob_left() {
        Serial.println("knob_left()");
        if (currently_opened!=-1) { // && items.get(currently_opened)->knob_left()) {
            Serial.printf("knob_left on currently_opened %i\n", currently_opened);
            items.get(currently_opened)->knob_left();
        } else {
            currently_selected--;
            if (currently_selected<0) 
                currently_selected = items.size()-1;
        }
        return true;
    }
    bool knob_right() {
        Serial.println("knob_right()");
        if (currently_opened!=-1) { //&& items.get(currently_opened)->knob_right()) {
            Serial.printf("knob_right on currently_opened %i\n", currently_opened);
            items.get(currently_opened)->knob_right();
        } else {
            currently_selected++;
            if (currently_selected >= items.size())
                currently_selected = 0;
        }
        return true;
    }
    bool button_select() {
        Serial.println("button_select()");
        if (currently_opened==-1) {
            Serial.printf("button_select with currently_opened -1 - selecting %i\n", currently_selected);
            currently_opened = currently_selected;
        } else {
            Serial.printf("button_select subselect on %i\n", currently_opened);
            items.get(currently_opened)->button_select();
        } 
        return true;
    }
    bool button_back() {
        Serial.println("button_back()");
        if (currently_opened!=-1 && !items.get(currently_opened)->button_back()) {
            Serial.printf("back with currently_opened %i and no subhandling, setting to -1\n", currently_opened);
            currently_selected = currently_opened;
            currently_opened = -1;
        } else {
            Serial.printf("back with currently_opened %i, handled by selected"); //setting to -1\n", currently_opened);
        }
        return true;
    }
    bool button_right() {
        Serial.println("button_right()");
        if (currently_opened!=-1) {
            if (items.get(currently_opened)->button_right()) {
                Serial.printf("right with currently_opened %i subhandled!\n", currently_opened);
            } else {
                Serial.printf("right with currently_opened %i not subhandled!\n", currently_opened);
            }
        } else {
            Serial.printf("right with nothing currently_opened\n"); //setting to -1\n", currently_opened);
        }
        return true;
    }

    public:
        char last_message[20] = "...started up...";
        uint32_t message_colour = ST77XX_WHITE;
        DisplayTranslator *tft;

        Menu(DisplayTranslator *dt) {
            this->tft = dt;
        }

        void setup_display() {
            tft->setup();
        }

        void add(MenuItem *m) {
            m->tft = this->tft;
            items.add(m);
        }

        // set the colour of the message (ie red / green for error / success)
        void set_message_colour(uint32_t colour) {
            message_colour = colour;
        }
        // set the message to display at top of display
        void set_last_message(const char *msg) {
            strcpy(last_message, msg);
        }

        virtual int draw_message() {
            //tft.setCursor(0,0);
            // draw the last status message
            tft->setTextColor(message_colour,ST77XX_BLACK);
            tft->setTextSize(0);
            tft->printf("[%-20s]",last_message);
            return tft->getCursorY();
        }

        #ifdef PPQN
        #define LOOP_MARKERS
        virtual int draw_loop_markers() { //Coord pos) {
            //tft.setCursor(pos.x,pos.y);
            int LOOP_LENGTH = PPQN * BEATS_PER_BAR * BARS_PER_PHRASE;
            int y = 0;
            //y+=2;
            float percent = float(ticks % LOOP_LENGTH) / (float)LOOP_LENGTH;
            //tft.drawFastHLine(0, tft.width(), 3, ST77XX_WHITE);
            //tft.drawFastHLine(0, tft.width() * percent, 2, ST77XX_RED);
            tft->fillRect(0, y, (percent*(float)tft->width()), 6, ST77XX_RED);

            for (int i = 0 ; i < tft->width() ; i+=(tft->width()/(BEATS_PER_BAR*BARS_PER_PHRASE))) {
                tft->drawLine(i, y, i, y+2, ST7735_WHITE);
                //if (i%BEATS_PER_BAR==0)
                    //tft.drawLine(i, y, i, y+4, ST7735_CYAN);
            }

            for (int i = 0 ; i < tft->width() ; i+=(tft->width()/4)) {
                //tft.drawLine(i, y, i, y+4, ST7735_WHITE);
                tft->fillRect(i, y, 2, 5, ST7735_WHITE);
            }

            //Serial.printf("percent %f, width %i\n", percent, tft->width());
            y += 6;
            return y;
        }
        #endif

        // draw the menu display
        virtual int display();
        void update_inputs() {
            //static int button_count = 0;
            static int last_knob_read = 0, new_knob_read;
            //int new_knob_read;
            new_knob_read = knob.read();///4;
            if (new_knob_read!=last_knob_read) {
                last_knob_read = new_knob_read/4;
                //if (last_knob_read<0) 
                //    last_knob_read = MAX_KNOB;
                knob_turned(last_knob_read);
            }
            if (pushButtonA.update()) {
                if (pushButtonA.fallingEdge()) {
                    button_count++;
                    button_select();
                }
            }
            if (pushButtonB.update()) {
                if (pushButtonB.fallingEdge()) {
                    button_count++;
                    button_back();
                }
            }
            if (pushButtonC.update()) {
                if (pushButtonC.fallingEdge()) {
                    button_count++;
                    button_right();
                }
            }
        }


        int freeRam() {
            Serial.println("TODO: implement free RAM report");
            return 1337;
        }

};

extern Menu menu;

#endif

