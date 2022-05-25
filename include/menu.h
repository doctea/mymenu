#ifndef MENU_LIB_INCLUDED
#define MENU_LIB_INCLUDED

class Coord {
    public:
        int x, y;
        Coord(int in_x, int in_y) {
            x = in_x;
            y = in_y;
        }
};

#include "mymenu.h"
#include "menu_io.h"

//extern Menu menu;
//void setup_display();

#include <LinkedList.h>
#include "menuitems.h"

void setup_menu();

#if defined(__arm__) && defined(CORE_TEENSY)
    extern unsigned long _heap_start;
    extern unsigned long _heap_end;
    extern char *__brkval;
#endif


class Menu {
    int currently_selected  = -1;
    int currently_opened    = -1;
    LinkedList<MenuItem*> items = LinkedList<MenuItem*>();

    int panel_height[20];

    int last_knob_position;
    int button_count;

    void knob_turned(int knob_position) {
        Serial.printf("knob_turned %i\n", knob_position);
        //tft->setCursor(0,0);
        //tft->printf("knob %i", knob_position);
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
            Serial.printf("selected %i aka %s\n", currently_selected, items.get(currently_selected)->label);
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
            Serial.printf("selected %i aka %s\n", currently_selected, items.get(currently_selected)->label);
        }
        return true;
    }
    bool button_select() {
        Serial.printf("button_select() on item %i\n", currently_selected);
        if (currently_opened==-1) {
            Serial.printf("button_select with currently_opened -1 - selecting %i\n", currently_selected);
            currently_opened = currently_selected;
        } else {
            Serial.printf("button_select subselect on %i\n", currently_opened);
            if (items.get(currently_opened)->button_select()) 
                button_back();
        } 
        return true;
    }
    bool button_back() {
        Serial.println("button_back()");
        if (currently_opened!=-1 && !items.get(currently_opened)->button_back()) {
            Serial.printf("back with currently_opened %i and no subhandling, setting to -1\n", currently_opened);
            currently_selected = currently_opened;
            currently_opened = -1;
        } else if (currently_opened==-1) {
            Serial.printf("back pressed but already at top level with currently_opened %i"); //setting to -1\n", currently_opened);
        } else {
            Serial.printf("back with currently_opened %i, handled by selected\n"); //setting to -1\n", currently_opened);
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
        uint32_t message_colour = C_WHITE;
        DisplayTranslator *tft;

        Menu(DisplayTranslator *dt) {
            this->tft = dt;
        }

        void setup_display() {
            Serial.println("setup_display about to do tft->setup().."); Serial.flush();
            tft->setup();
            Serial.println("tft->setup() done!"); Serial.flush();
        }
        void start() {
            tft->start();
        }

        void add(MenuItem *m) {
            m->tft = this->tft;
            m->on_add();
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

        //#ifdef PPQN
        //#define LOOP_MARKERS
        //#define LOOP_LENGTH (PPQN * BEATS_PER_BAR * BARS_PER_PHRASE)
        virtual int draw_loop_markers(unsigned long ticks, unsigned long loop_length, int beats_per_bar = 4, int bars_per_phrase = 4) { //Coord pos) {
            //tft.setCursor(pos.x,pos.y);
            //int LOOP_LENGTH = PPQN * BEATS_PER_BAR * BARS_PER_PHRASE;
            int y = 0;
            //y+=2;
            float percent = float(ticks % loop_length) / (float)loop_length;
            //tft.drawFastHLine(0, tft.width(), 3, ST77XX_WHITE);
            //tft.drawFastHLine(0, tft.width() * percent, 2, ST77XX_RED);
            tft->fillRect(0, y, (percent*(float)tft->width()), 6, RED);

            for (int i = 0 ; i < tft->width() ; i+=(tft->width()/(beats_per_bar*bars_per_phrase))) {
                tft->drawLine(i, y, i, y+2, C_WHITE);
                //if (i%BEATS_PER_BAR==0)
                    //tft.drawLine(i, y, i, y+4, ST7735_CYAN);
            }

            for (int i = 0 ; i < tft->width() ; i+=(tft->width()/4)) {
                //tft.drawLine(i, y, i, y+4, ST7735_WHITE);
                tft->fillRect(i, y, 2, 5, C_WHITE);
            }

            //Serial.printf("percent %f, width %i\n", percent, tft->width());
            y += 6;
            return y;
        }
        //#endif

        // draw the menu display
        virtual int display();

        void update_inputs() {
            //static int button_count = 0;
            static int last_knob_read = 0, new_knob_read;
            //int new_knob_read;
            #ifdef ENCODER_KNOB_L
                new_knob_read = knob.read() / ENCODER_STEP_DIVISOR;///4;
                if (new_knob_read!=last_knob_read) {
                    Serial.printf("new_knob_read %i changed from %i\n", new_knob_read, last_knob_read);
                    if (ENCODER_STEP_DIVISOR>1)
                        last_knob_read = new_knob_read; ///4; 
                    else
                        last_knob_read = new_knob_read; // / ENCODER_STEP_DIVISOR; ///4; 
                    //if (last_knob_read<0) 
                    //    last_knob_read = MAX_KNOB;
                    knob_turned(last_knob_read);
                }
            #endif
            #ifdef PIN_BUTTON_A
                if (pushButtonA.update()) {
                    if (pushButtonA.fallingEdge()) {
                        button_count++;
                        button_select();
                    }
                }
            #endif
            #ifdef PIN_BUTTON_B
                if (pushButtonB.update()) {
                    if (pushButtonB.fallingEdge()) {
                        button_count++;
                        button_back();
                    }
                }
            #endif
            #ifdef PIN_BUTTON_C
                if (pushButtonC.update()) {
                    if (pushButtonC.fallingEdge()) {
                        button_count++;
                        button_right();
                    }
                }
            #endif
        }

        #if defined(__arm__) && defined(CORE_TEENSY)
            int freeRam() {
                return (char *)&_heap_end - __brkval;
            }

            void debug_free_ram() {
                //Serial.println(F("debug_free_ram() not implemented on Teensy"));
                Serial.printf("debug_free_ram: %i\n", freeRam());
            }
        #else
            int freeRam () {  
                extern int __heap_start, *__brkval;
                int v;
                return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
            }
            void debug_free_ram() {
                Serial.print(F("debug_free_ram: Free RAM is "));
                Serial.println(freeRam());
            }
        #endif

};

extern Menu *menu;


//void tft_print (char *text);
//void tft_clear();
/*void tft_print (char *text) {
    menu->tft->print(text);
}
void tft_clear() {
    menu->tft->clear();
}*/


/*void tft_print (char *text) {
    menu->tft->print(text);
}
void tft_clear() {
    menu->tft->clear();
}*/

#endif

