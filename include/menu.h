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

#include "debug.h"

#include "mymenu.h"
#include "menu_io.h"

/*#ifndef MENU_MAX_PANELS
    #define MENU_MAX_PANELS 50
#endif*/

//extern Menu menu;
//void setup_display();

#include <LinkedList.h>
#include "menuitems.h"

FLASHMEM void setup_menu();

#if defined(__arm__) && defined(CORE_TEENSY)
    extern unsigned long _heap_start;
    extern unsigned long _heap_end;
    extern char *__brkval;
#endif

#define MAX_PAGE_TITLE 10
struct page_t {
    const char *title = "Default"; //[MAX_PAGE_TITLE];
    uint16_t colour = C_WHITE;
    int currently_selected = -1;
    int currently_opened = -1;
    int *panel_bottom = nullptr;
    int num_panels = 0;
    LinkedList<MenuItem*> *items = nullptr;
};

class Menu {
    //int currently_selected  = -1;
    //int currently_opened    = -1;

    //LinkedList<MenuItem*> *items = nullptr; //LinkedList<MenuItem*>();

    int opened_page_index = -1;
    int selected_page_index = 0;
    page_t *selected_page = nullptr;
    LinkedList<page_t*> *pages = nullptr;

    PinnedPanelMenuItem *pinned_panel = nullptr;

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

        bool is_opened() {
            return this->selected_page->currently_opened!=-1;
        }

        void open_page(int page_index) {
            opened_page_index = page_index;
            if (selected_page->currently_selected==-1)
                selected_page->currently_selected = 0;
        }
        
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
            if (opened_page_index==-1) {
                selected_page_index--;
                if (selected_page_index<0)
                    selected_page_index = pages->size() - 1;
                select_page(selected_page_index);
            } else if (selected_page->currently_opened!=-1) { // && items->get(currently_opened)->knob_left()) {
                Serial.printf(F("knob_left on currently_opened menuitem %i\n"), selected_page->currently_opened);
                selected_page->items->get(selected_page->currently_opened)->knob_left();
            } else {
                selected_page->currently_selected--;
                if (selected_page->currently_selected<0) 
                    selected_page->currently_selected = selected_page->items->size()-1;
                Serial.printf(F("selected %i aka %s\n"), selected_page->currently_selected, selected_page->items->get(selected_page->currently_selected)->label);
                if (selected_page->currently_selected>=0 && selected_page->currently_selected < selected_page->items->size() && !selected_page->items->get(selected_page->currently_selected)->is_selectable()) {
                    //Serial.println("?? extra knob_left because isn't selectable");
                    knob_left();
                }
            }
            if (debug) {
                char msg[tft->get_c_max()] = "";
                sprintf(msg, "knob_left to %i", selected_page->currently_selected);
                set_last_message(msg);
            }
            return true;
        }
        bool knob_right() {
            Serial.println(F("knob_right()"));
            if (opened_page_index==-1) {
                selected_page_index++;
                if (selected_page_index>=pages->size())
                    selected_page_index = 0;
                select_page(selected_page_index);
            } else if (selected_page->currently_opened!=-1) { //&& items->get(currently_opened)->knob_right()) {
                Serial.printf(F("knob_right on currently_opened menuitem %i\n"), selected_page->currently_opened);
                selected_page->items->get(selected_page->currently_opened)->knob_right();
            } else {
                selected_page->currently_selected++;
                if (selected_page->currently_selected >= selected_page->items->size())
                    selected_page->currently_selected = 0;
                Serial.printf(F("selected %i aka %s\n"), selected_page->currently_selected, selected_page->items->get(selected_page->currently_selected)->label);
                if (selected_page->currently_selected>=0 && selected_page->currently_selected < selected_page->items->size() && !selected_page->items->get(selected_page->currently_selected)->is_selectable()) {
                    //Serial.println("?? extra knob_right because isn't selectable");
                    knob_right();
                }
            }
            if (debug) {
                char msg[tft->get_c_max()] = "";
                sprintf(msg, "knob_left to %i", selected_page->currently_selected);
                set_last_message(msg);
            }

            return true;
        }
        bool button_select() {
            Serial.printf(F("Menu#button_select() on item %i\n"), selected_page->currently_selected);
            if (opened_page_index==-1) {
                open_page(selected_page_index);
            } else if (selected_page->currently_opened==-1) {
                Serial.printf(F("button_select with currently_opened menuitem -1 - opening %i\n"), selected_page->currently_selected);
                if (selected_page->items->get(selected_page->currently_selected)->action_opened()) {
                    selected_page->currently_opened = selected_page->currently_selected;
                    return false;
                }
            } else {
                Serial.printf(F("Menu#button_select() subselecting already-opened %i (%s)\n"), selected_page->currently_opened, selected_page->items->get(selected_page->currently_opened)->label);
                if (selected_page->items->get(selected_page->currently_opened)->button_select()) 
                    button_back();
            } 
            return true;
        }
        bool button_select_released() {
            Serial.printf(F("Menu#button_select_released() on item %i\n"), selected_page->currently_selected);
            if (opened_page_index==-1) {
                // do nothing?
            } else if (selected_page->currently_opened==-1) {
                /*Serial.printf(F("button_select_released with currently_opened menuitem -1 - opening %i\n"), currently_selected);
                if (items->get(currently_selected)->action_opened()) {
                    currently_opened = currently_selected;
                    return false;
                }*/
            } else {
                Serial.printf(F("Menu#button_select_released() subselecting already-opened %i (%s)\n"), selected_page->currently_opened, selected_page->items->get(selected_page->currently_opened)->label);
                if (selected_page->items->get(selected_page->currently_opened)->button_select_released()) 
                    button_back();
            } 
            return true;
        }
        bool button_back() {
            Serial.println(F("button_back()"));
            if (opened_page_index==-1) {
                // do nothing?
            } else if (selected_page->currently_opened!=-1 && !selected_page->items->get(selected_page->currently_opened)->button_back()) {
                Serial.printf(F("back with currently_opened menuitem %i and no subhandling, setting to -1\n"), selected_page->currently_opened);
                selected_page->currently_selected = selected_page->currently_opened;
                selected_page->currently_opened = -1;
            } else if (selected_page->currently_opened==-1) {
                Serial.printf(F("back pressed but already at top level with currently_opened menuitem %i\n"), selected_page->currently_opened); //setting to -1\n", currently_opened);
                selected_page->currently_selected = -1;
                opened_page_index = -1;
            } else {
                Serial.printf(F("back with currently_opened menuitem %i, handled by selected\n"), selected_page->currently_opened); //setting to -1\n", currently_opened);
            }
            tft->clear(true);   // TOOD: don't rely on this
            return true;
        }
        bool button_right() {
            Serial.println(F("button_right()"));
            if (opened_page_index==-1) {
                open_page(selected_page_index);
            } else if (selected_page->currently_opened!=-1) {
                if (selected_page->items->get(selected_page->currently_opened)->button_right()) {
                    Serial.printf(F("right with currently_opened menuitem %i subhandled!\n"), selected_page->currently_opened);
                } else {
                    Serial.printf(F("right with currently_opened menuitem %i not subhandled!\n"), selected_page->currently_opened);
                }
            } else {
                Serial.printf(F("right with nothing currently_opened\n")); //setting to -1\n", currently_opened);
            }
            return true;
        }

        FLASHMEM int get_num_panels() {
            return this->selected_page->items->size();
        }

        char last_message[MENU_C_MAX] = ""; //...started up...";
        uint32_t message_colour = C_WHITE;
        DisplayTranslator *tft;

        Menu(DisplayTranslator *dt) {
            this->tft = dt;
            //this->items = new LinkedList<MenuItem*>();
            this->pages = new LinkedList<page_t*>();
            this->select_page(this->add_page("Main"));
        }

        //FLASHMEM 
        int add_page(const char *title, uint16_t colour = C_WHITE) {
            this->pages->add(new page_t {
                .title = (new String(title))->c_str(),
                .colour = colour
            });
            int index = this->pages->size()-1;
            this->select_page(index);
            selected_page->items = new LinkedList<MenuItem*>();
            return index;
        }

        FLASHMEM void setup_display() {
            Serial.println(F("setup_display about to do tft->setup()..")); Serial_flush();
            tft->setup();
            Serial.println(F("tft->setup() done!")); Serial_flush();
        }
        #ifndef GDB_DEBUG
        FLASHMEM 
        #endif
        void start() {
            tft->start();
        }

        void select_page(int p) {
            this->selected_page_index = p;
            if (selected_page_index >= pages->size())
                this->selected_page_index = 0;
            selected_page = pages->get(selected_page_index);
        }

        FLASHMEM void add(LinkedList<MenuItem *> *items, int16_t default_fg_colour = C_WHITE) {
            for (int i = 0 ; i < items->size() ; i++) {
                items->get(i)->set_default_colours(default_fg_colour, BLACK);
                //Serial.printf("setting default_fg_colour %04X on %s\n", default_fg_colour, items->get(i)->label);
                this->add(items->get(i));
            }
        }
        /*#ifndef GDB_DEBUG
        FLASHMEM 
        #endif*/
        void add(MenuItem *m) {
            Serial.printf("Menu page %i, adding item %i: %s\n", selected_page_index, selected_page->items->size(), m->label);
            if (m!=nullptr) {
                m->tft = this->tft;
                m->on_add();
                selected_page->items->add(m);
            } else {
                Serial.println(F("Passed nullptr to menu#add!"));
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

            //Serial.printf("update_ticks %i...\n", ticks);
            const int pages_count = this->pages->size();
            for (int p = 0 ; p < pages_count ; p++) {
                //Serial.printf("\tupdate_ticks for page %i...\n", p);
                const int items_count = this->pages->get(p)->items->size();
                for (int i = 0 ; i < items_count ; i++) {
                    //Serial.printf("\t\tupdate_ticks for item %i...\n", i);
                    this->pages->get(p)->items->get(i)->update_ticks(ticks);
                }
            }
            //Serial.printf("updated_ticks %i\n", ticks);
        }

        void update_inputs() {
            //static int button_count = 0;
            //int new_knob_read;
            #ifdef ENCODER_KNOB_L
                static int last_knob_read = 0, new_knob_read;
                new_knob_read = knob.read() / ENCODER_STEP_DIVISOR;///4;
                if (new_knob_read!=last_knob_read) {
                    //Serial.printf("new_knob_read %i changed from %i\n", new_knob_read, last_knob_read);
                    /*if (ENCODER_STEP_DIVISOR>1)
                        last_knob_read = new_knob_read; ///4; 
                    else
                        last_knob_read = new_knob_read; // / ENCODER_STEP_DIVISOR; ///4; */
                    last_knob_read = new_knob_read;
                    knob_turned(last_knob_read);
                }
            #endif
            #ifdef PIN_BUTTON_A
                if (pushButtonA.update()) {
                    if (pushButtonA.fell()) {
                        button_count++;
                        button_select();
                    } else if (pushButtonA.rose()) {
                        button_select_released();
                    }
                }
            #endif
            #ifdef PIN_BUTTON_B
                if (pushButtonB.update()) {
                    if (pushButtonB.fell()) {
                        button_count++;
                        button_back();
                    } /*else if (pushButtonB.fell()) {
                        button_back_released();
                    }*/
                }
            #endif
            #ifdef PIN_BUTTON_C
                if (pushButtonC.update()) {
                    if (pushButtonC.fell()) {
                        button_count++;
                        button_right();
                    } /*else if (pushButtonC.fell()) {
                        button_right_released();
                    }*/
                }
            #endif
        }

        #if defined(__arm__) && defined(CORE_TEENSY)
            int freeRam() {
                return (char *)&_heap_end - __brkval;
            }

            void debug_free_ram() {
                //Serial.println(F("debug_free_ram() not implemented on Teensy"));
                Serial.printf(F("debug_free_ram: %i\n"), freeRam());
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

        uint16_t get_next_colour() {
            static int index = 0;
            static const uint16_t colours[] = {
                /*tft->rgb(50,0,0),     // dark colours
                tft->rgb(0,50,0),
                tft->rgb(0,0,50),
                tft->rgb(0,50,50),
                tft->rgb(50,0,50),
                tft->rgb(50,50,0)*/
                tft->rgb(255,200,200),  // bright pastel colours
                tft->rgb(200,255,200),
                tft->rgb(200,200,255),
                tft->rgb(200,255,255),
                tft->rgb(255,200,255),
                tft->rgb(255,255,200)
            };
            index%=(sizeof(colours)/sizeof(uint16_t));
            return colours[index++];
        }

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

