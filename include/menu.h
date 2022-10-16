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

#ifndef MENU_MAX_PANELS
    #define MENU_MAX_PANELS 50
#endif

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
    LinkedList<MenuItem*> *items = nullptr; //LinkedList<MenuItem*>();

    PinnedPanelMenuItem *pinned_panel = nullptr;

    int panel_height[MENU_MAX_PANELS];/* = { 0, 0, 0, 0, 0, 
                             0, 0, 0, 0, 0, 
                             0, 0, 0, 0, 0,
                             0, 0, 0, 0, 0 };*/

    int last_knob_position = -1;
    int button_count = 0;

    public:
        bool debug = false;
        bool debug_times = false;

        void setDebugTimes(bool value) {
            this->debug_times = value;
        }
        bool isDebugTimes() {
            return this->debug_times;
        }

        enum {
            NORMAL = 0,
            DISPLAY_ONE = 1
        };

        int mode = NORMAL;
        
        // input-handling stuff
        void knob_turned(int knob_position) {
            Serial.printf(F("knob_turned %i\n"), knob_position);
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
            Serial.println(F("knob_left()"));
            if (currently_opened!=-1) { // && items->get(currently_opened)->knob_left()) {
                Serial.printf(F("knob_left on currently_opened menuitem %i\n"), currently_opened);
                items->get(currently_opened)->knob_left();
            } else {
                currently_selected--;
                if (currently_selected<0) 
                    currently_selected = items->size()-1;
                Serial.printf(F("selected %i aka %s\n"), currently_selected, items->get(currently_selected)->label);
                if (currently_selected>=0 && currently_selected < items->size() && !items->get(currently_selected)->is_selectable()) {
                    //Serial.println("?? extra knob_left because isn't selectable");
                    knob_left();
                }
            }
            if (debug) {
                char msg[tft->get_c_max()] = "";
                sprintf(msg, "knob_left to %i", currently_selected);
                set_last_message(msg);
            }
            return true;
        }
        bool knob_right() {
            Serial.println(F("knob_right()"));
            if (currently_opened!=-1) { //&& items->get(currently_opened)->knob_right()) {
                Serial.printf(F("knob_right on currently_opened menuitem %i\n"), currently_opened);
                items->get(currently_opened)->knob_right();
            } else {
                currently_selected++;
                if (currently_selected >= items->size())
                    currently_selected = 0;
                Serial.printf(F("selected %i aka %s\n"), currently_selected, items->get(currently_selected)->label);
                if (currently_selected>=0 && currently_selected < items->size() && !items->get(currently_selected)->is_selectable()) {
                    //Serial.println("?? extra knob_right because isn't selectable");
                    knob_right();
                }
            }
            if (debug) {
                char msg[tft->get_c_max()] = "";
                sprintf(msg, "knob_left to %i", currently_selected);
                set_last_message(msg);
            }

            return true;
        }
        bool button_select() {
            Serial.printf(F("Menu#button_select() on item %i\n"), currently_selected);
            if (currently_opened==-1) {
                Serial.printf(F("button_select with currently_opened menuitem -1 - opening %i\n"), currently_selected);
                if (items->get(currently_selected)->action_opened()) {
                    currently_opened = currently_selected;
                    return false;
                }
            } else {
                Serial.printf(F("Menu#button_select() subselecting already-opened %i (%s)\n"), currently_opened, items->get(currently_opened)->label);
                if (items->get(currently_opened)->button_select()) 
                    button_back();
            } 
            return true;
        }
        bool button_back() {
            Serial.println(F("button_back()"));
            if (currently_opened!=-1 && !items->get(currently_opened)->button_back()) {
                Serial.printf(F("back with currently_opened menuitem %i and no subhandling, setting to -1\n"), currently_opened);
                currently_selected = currently_opened;
                currently_opened = -1;
            } else if (currently_opened==-1) {
                Serial.printf("back pressed but already at top level with currently_opened menuitem %i\n", currently_opened); //setting to -1\n", currently_opened);
                currently_selected = 0;
            } else {
                Serial.printf("back with currently_opened menuitem %i, handled by selected\n", currently_opened); //setting to -1\n", currently_opened);
            }
            tft->clear(true);   // TOOD: don't rely on this
            return true;
        }
        bool button_right() {
            Serial.println("button_right()");
            if (currently_opened!=-1) {
                if (items->get(currently_opened)->button_right()) {
                    Serial.printf("right with currently_opened menuitem %i subhandled!\n", currently_opened);
                } else {
                    Serial.printf("right with currently_opened menuitem %i not subhandled!\n", currently_opened);
                }
            } else {
                Serial.printf("right with nothing currently_opened\n"); //setting to -1\n", currently_opened);
            }
            return true;
        }

        char last_message[MENU_C_MAX] = "...started up...";
        uint32_t message_colour = C_WHITE;
        DisplayTranslator *tft;

        Menu(DisplayTranslator *dt) {
            this->tft = dt;
            this->items = new LinkedList<MenuItem*>();
        }

        FLASHMEM void setup_display() {
            Serial.println("setup_display about to do tft->setup().."); Serial.flush();
            tft->setup();
            Serial.println("tft->setup() done!"); Serial.flush();
        }
        FLASHMEM void start() {
            tft->start();
        }

        FLASHMEM void add(LinkedList<MenuItem *> *items) {
            for (int i = 0 ; i < items->size() ; i++) {
                this->add(items->get(i));
            }
        }
        FLASHMEM void add(MenuItem *m) {
            if (m!=nullptr) {
                m->tft = this->tft;
                m->on_add();
                items->add(m);
            } else {
                Serial.println("Passed nullptr to menu#add!");
            }
        }
        FLASHMEM void add_pinned(PinnedPanelMenuItem *m) {
            if (m!=nullptr) {
                m->tft = this->tft;
                m->on_add();
            }
            pinned_panel = m;
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
            tft->printf((char*)tft->get_message_format(), last_message);
            return tft->getCursorY();
        }

        //#ifdef PPQN
        //#define LOOP_MARKERS
        //#define LOOP_LENGTH (PPQN * BEATS_PER_BAR * BARS_PER_PHRASE)

        // draw the menu display
        virtual int display();
        
        void update_ticks(unsigned long ticks) {
            if (pinned_panel!=nullptr)
                pinned_panel->update_ticks(ticks);

            for (int i = 0 ; i < this->items->size() ; i++) {
                this->items->get(i)->update_ticks(ticks);
            }
        }

        void update_inputs() {
            //static int button_count = 0;
            //int new_knob_read;
            #ifdef ENCODER_KNOB_L
                static int last_knob_read = 0, new_knob_read;
                new_knob_read = knob.read() / ENCODER_STEP_DIVISOR;///4;
                if (new_knob_read!=last_knob_read) {
                    //Serial.printf("new_knob_read %i changed from %i\n", new_knob_read, last_knob_read);
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
                /*uint32_t stackTop;
                uint32_t heapTop;

                // current position of the stack.
                stackTop = (uint32_t) &stackTop;

                // current position of heap.
                void* hTop = malloc(1);
                heapTop = (uint32_t) hTop;
                free(hTop);

                // The difference is (approximately) the free, available ram.
                return stackTop - heapTop;*/
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

