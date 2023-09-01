#ifndef MENU_LIB_INCLUDED
#define MENU_LIB_INCLUDED

#define MENU_MESSAGE_MAX (MENU_C_MAX*2)

#ifndef CORE_TEENSY
    // if no FLASHMEM then we're probably not running on Teensy platform, so define it empty
    #ifndef FLASHMEM
        #define FLASHMEM
    #endif
    //#define F(x) { x }
#endif

#include "debug.h"

#include "mymenu.h"
#include "menu_io.h"

#define MAX_MESSAGE_LOG 20

#include <LinkedList.h>
#include "menuitems.h"
//#include "menuitems_pinned.h"

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
    volatile int currently_selected = -1;
    int currently_opened = -1;
    int *panel_bottom = nullptr;
    int num_panels = 0;
    LinkedList<MenuItem*> *items = nullptr;
};

class Menu {
    int opened_page_index = -1;
    int selected_page_index = 0;
    page_t *selected_page = nullptr;
    LinkedList<page_t*> *pages = nullptr;

    PinnedPanelMenuItem *pinned_panel = nullptr;
    LinkedList<String> *messages_log = nullptr;

    int last_knob_position = -1;
    int button_count = 0;

    uint16_t screen_height_cutoff = 100;

    int8_t tab_textsize = 1;

    bool profile_enable = true;
    char profile_string[MENU_C_MAX] = "profiler output";

    public:
        bool debug = false;
        bool debug_times = false;

        bool auto_update = true;    // whether to send update to tft at end of every display() call, or to allow host app to decide

        bool recalculate_bottoms = false;

        void setDebugTimes(bool value) {
            this->debug_times = value;
        }
        bool isDebugTimes() {
            return this->debug_times;
        }
        void setProfileEnable(bool value) {
            this->profile_enable = value;
        }
        bool isProfileEnable() {
            return this->profile_enable;
        }

        void set_default_textsize(int textSize) {
            tft->set_default_textsize(textSize);
            this->recalculate_bottoms = true;
        }
        int get_default_textsize() {
            return tft->getTextSize();
        }

        enum {
            NORMAL = 0,
            DISPLAY_ONE = 1
        };

        int mode = NORMAL;

        char last_message[MENU_C_MAX] = ""; //...started up...";
        uint32_t message_colour = C_WHITE;
        DisplayTranslator *tft;

        bool is_opened() {
            return this->selected_page->currently_opened!=-1;
        }

        // input-handling stuff
        void knob_turned(int knob_position) {
            Debug_printf(F("knob_turned %i\n"), knob_position);
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
            Debug_println(F("knob_left()"));
            if (opened_page_index==-1) {
                selected_page_index--;
                if (selected_page_index<0)
                    selected_page_index = pages->size() - 1;
                select_page(selected_page_index);
            } else if (selected_page->currently_opened!=-1) { // && items->get(currently_opened)->knob_left()) {
                //Serial.printf(F("knob_left on currently_opened menuitem %i\n"), selected_page->currently_opened);
                selected_page->items->get(selected_page->currently_opened)->knob_left();
            } else {
                selected_page->currently_selected--;
                if (selected_page->currently_selected<0) 
                    selected_page->currently_selected = selected_page->items->size()-1;
                //Serial.printf(F("selected %i aka %s\n"), selected_page->currently_selected, selected_page->items->get(selected_page->currently_selected)->label);
                if (selected_page->currently_selected>=0 && selected_page->currently_selected < (int)selected_page->items->size() && !selected_page->items->get(selected_page->currently_selected)->is_selectable()) {
                    //Serial.println("?? extra knob_left because isn't selectable");
                    knob_left();
                }
            }
            /*if (debug) {
                char msg[tft->get_c_max()] = "";
                sprintf(msg, "knob_left to %i", selected_page->currently_selected);
                set_last_message(msg);
            }*/
            return true;
        }
        bool knob_right() {
            Debug_println(F("knob_right()"));
            if (opened_page_index==-1) {
                selected_page_index++;
                if (selected_page_index>=(int)pages->size())
                    selected_page_index = 0;
                select_page(selected_page_index);
            } else if (selected_page->currently_opened!=-1) { //&& items->get(currently_opened)->knob_right()) {
                //Serial.printf(F("knob_right on currently_opened menuitem %i\n"), selected_page->currently_opened);
                selected_page->items->get(selected_page->currently_opened)->knob_right();
            } else {
                selected_page->currently_selected++;
                if (selected_page->currently_selected >= (int)selected_page->items->size())
                    selected_page->currently_selected = 0;
                //Serial.printf(F("selected %i aka %s\n"), selected_page->currently_selected, selected_page->items->get(selected_page->currently_selected)->label);
                if (selected_page->currently_selected>=0 && selected_page->currently_selected < (int)selected_page->items->size() && !selected_page->items->get(selected_page->currently_selected)->is_selectable()) {
                    //Serial.println("?? extra knob_right because isn't selectable");
                    knob_right();
                }
            }
            /*if (debug) {
                char msg[tft->get_c_max()] = "";
                sprintf(msg, "knob_left to %i", selected_page->currently_selected);
                set_last_message(msg);
            }*/

            return true;
        }
        bool button_select() {
            Debug_printf(F("Menu#button_select() on item %i\n"), selected_page->currently_selected);
            if (opened_page_index==-1) {
                open_page(selected_page_index);
            } else if (selected_page->currently_opened==-1) {
                Debug_printf(F("button_select with currently_opened menuitem -1 - opening %i\n"), selected_page->currently_selected);
                if (selected_page->items->get(selected_page->currently_selected)->action_opened()) {
                    selected_page->currently_opened = selected_page->currently_selected;
                    return false;
                }
            } else {
                Debug_printf(F("Menu#button_select() subselecting already-opened %i (%s)\n"), selected_page->currently_opened, selected_page->items->get(selected_page->currently_opened)->label);
                if (selected_page->items->get(selected_page->currently_opened)->button_select()) 
                    button_back();
            } 
            return true;
        }
        bool button_select_released() {
            Debug_printf(F("Menu#button_select_released() on item %i\n"), selected_page->currently_selected);
            if (opened_page_index==-1) {
                // do nothing?
            } else if (selected_page->currently_opened==-1) {
                /*Serial.printf(F("button_select_released with currently_opened menuitem -1 - opening %i\n"), currently_selected);
                if (items->get(currently_selected)->action_opened()) {
                    currently_opened = currently_selected;
                    return false;
                }*/
            } else {
                Debug_printf(F("Menu#button_select_released() subselecting already-opened %i (%s)\n"), selected_page->currently_opened, selected_page->items->get(selected_page->currently_opened)->label);
                if (selected_page->items->get(selected_page->currently_opened)->button_select_released()) 
                    button_back();
            } 
            return true;
        }
        bool button_back() {
            Debug_println(F("button_back()"));
            if (opened_page_index==-1) {
                // do nothing?
            } else if (selected_page->currently_opened!=-1 && !selected_page->items->get(selected_page->currently_opened)->button_back()) {
                Debug_printf(F("back with currently_opened menuitem %i and no subhandling, setting to -1\n"), selected_page->currently_opened);
                selected_page->currently_selected = selected_page->currently_opened;
                selected_page->currently_opened = -1;
            } else if (selected_page->currently_opened==-1) {
                Debug_printf(F("back pressed but already at top level with currently_opened menuitem %i\n"), selected_page->currently_opened); //setting to -1\n", currently_opened);
                selected_page->currently_selected = -1;
                opened_page_index = -1;
            } else {
                Debug_printf(F("back with currently_opened menuitem %i, handled by selected\n"), selected_page->currently_opened); //setting to -1\n", currently_opened);
            }
            tft->clear(true);   // TOOD: don't rely on this
            return true;
        }
        bool button_right() {
            Debug_println(F("button_right()"));
            if (opened_page_index==-1) {
                open_page(selected_page_index);
            } else if (selected_page->currently_opened!=-1) {
                if (selected_page->items->get(selected_page->currently_opened)->button_right()) {
                    Debug_printf(F("right with currently_opened menuitem %i subhandled!\n"), selected_page->currently_opened);
                } else {
                    Debug_printf(F("right with currently_opened menuitem %i not subhandled!\n"), selected_page->currently_opened);
                }
            } else {
                Debug_printf(F("right with nothing currently_opened\n")); //setting to -1\n", currently_opened);
            }
            return true;
        }

        FLASHMEM int get_num_panels() {
            return this->selected_page->items->size();
        }


        Menu(DisplayTranslator *dt) {
            this->tft = dt;
            this->pages = new LinkedList<page_t*>();
            this->select_page(this->add_page("Main"));
            this->screen_height_cutoff = (int)(0.75f*(float)tft->height());
        }

        void set_screen_height_cutoff(int cutoff) {
            this->screen_height_cutoff = cutoff;
        }

        void set_screen_height_cutoff(float cutoff) {
            set_screen_height_cutoff((int)(cutoff * ((float)tft->height()-(3 * tft->getRowHeight()))));
        }

        //FLASHMEM 
        int add_page(const char *title, uint16_t colour = C_WHITE) {
            //Serial.printf("add_page(%s) has current size of %i\n", title, this->pages->size());
            return insert_page(title, this->pages->size()>0 ? this->pages->size() : 0, colour);
        }
        FLASHMEM
        int insert_page(const char *title, unsigned int position, uint16_t colour = C_WHITE) {
            //Serial.printf("insert_page() passed position %i\n", position);
            if (position > this->pages->size()) 
                position = this->pages->size();

            this->pages->add(position, new page_t {
                .title = (new String(title))->c_str(),
                .colour = colour
            });

            this->select_page(position);
            selected_page->items = new LinkedList<MenuItem*>();
            return position;
        }

        FLASHMEM void setup_display() {
            Debug_println(F("setup_display about to do tft->setup()..")); Serial_flush();
            tft->setup();
            Debug_println(F("tft->setup() done!")); Serial_flush();
        }
        #ifndef GDB_DEBUG
        FLASHMEM 
        #endif
        void start() {
            tft->start();
        }

        void select_next_page() {
            this->opened_page_index = -1;
            if (this->selected_page!=nullptr) 
                this->selected_page->currently_opened = -1;
            //Serial.printf("select_next_page currently on %i\n", this->selected_page_index);
            this->select_page(++this->selected_page_index);
        }
        void select_previous_page() {
            this->opened_page_index = -1;
            if (this->selected_page!=nullptr) 
                this->selected_page->currently_opened = -1;
            //Serial.printf("select_previous_page currently on %i\n", this->selected_page_index);
            this->select_page(this->selected_page_index - 1);
        }

        void select_page(unsigned int p) {
            this->selected_page_index = p;
            if (selected_page_index >= (int)pages->size())
                this->selected_page_index = 0;
            else if (selected_page_index < 0 )
                this->selected_page_index = pages->size() - 1;
            selected_page = pages->get(selected_page_index);
            //this->tft->clear();
            //if (pages->size()==1)
            //    this->open_page(0);
            //Serial.printf("Selected page %i\n", selected_page_index);
        }
        void open_page(unsigned int page_index) {
            page_index = constrain(page_index, (unsigned int)0, pages->size() - 1);
            opened_page_index = page_index;
            //Serial.printf("opening page %i, currently_selected is %i\n", page_index, selected_page->currently_selected);

            // select first selectable item 
            if (selected_page!=nullptr && selected_page->currently_selected==-1) {
                this->knob_right(); 
            }
            //Serial.printf("=> currently_selected is now %i\n", selected_page->currently_selected);
        }       

        // add a linkedlist of menuitems; delete the object when finished!
        FLASHMEM void add(LinkedList<MenuItem *> *items, uint16_t default_fg_colour = C_WHITE) {
            //Serial.println("starting add"); Serial.flush();
            if (items==nullptr) {
                Serial.println("WARNING: nullptr list passed to menu#add, skipping!");
                return;
            }
            //Serial.printf("passed items @%p\n", items);
            //Serial.printf("got items of size %i\n", items->size()); Serial.flush();
            for (unsigned int i = 0 ; i < items->size() ; i++) {
                if (items->get(i)==nullptr) {
                    //Serial.printf("skipping %i because its nullptr\n", i);
                    continue;
                }
                //items->get(i)->set_default_colours(default_fg_colour, BLACK);
                //Serial.printf("setting default_fg_colour %04X on %s\n", default_fg_colour, items->get(i)->label);
                //Serial.printf("adding item %i of %i..\n", i+1, items->size()); Serial.flush();
                this->add(items->get(i), default_fg_colour);
                //Serial.printf("added item %i of %i.\n", i+1, items->size()); Serial.flush();
            }
            //Serial.printf("deleting items?"); Serial.flush();
            items->clear(); 
            delete items;
            //Serial.printf("deleted items!"); Serial.flush();
        }
        /*#ifndef GDB_DEBUG
        FLASHMEM 
        #endif*/
        void add(MenuItem *m, uint16_t default_fg_colour = C_WHITE) {
            //Serial.printf("Menu page %i, adding item %i: %s\n", selected_page_index, selected_page->items->size(), m->label);
            if (m!=nullptr) {
                if (m->default_fg==C_WHITE)
                    m->set_default_colours(default_fg_colour, BLACK);
                m->tft = this->tft;
                m->on_add();
                selected_page->items->add(m);
            } else {
                //Serial.println(F("Passed nullptr to menu#add!"));
            }
        }
        FLASHMEM void add_pinned(PinnedPanelMenuItem *m) {
            if (m!=nullptr) {
                m->tft = this->tft;
                m->on_add();
            }
            pinned_panel = m;
        }

        FLASHMEM void set_messages_log(LinkedList<String> *messages_log) {
            this->messages_log = messages_log;
        }

        void add_message(const char *msg) {
            if (this->messages_log!=nullptr) {
                this->messages_log->add(String(msg));
                if (this->messages_log->size() > MAX_MESSAGE_LOG) {
                    messages_log->unlink(0);
                }
            }
        }        

        // set the colour of the message (ie red / green for error / success)
        void set_message_colour(uint32_t colour) {
            message_colour = colour;
        }
        // set the message to display at top of display
        void set_last_message(const char *msg) {
            strncpy(last_message, msg, MENU_C_MAX);
            this->add_message(msg);
        }

        int draw_message() {
            //tft.setCursor(0,0);
            // draw the last status message
            tft->setTextColor(message_colour,BLACK);
            tft->setTextSize(tft->default_textsize);
            tft->printf(tft->get_message_format(), last_message);
            tft->setCursor(0,tft->getCursorY()+3);  // workaround for tab positions?
            return tft->getCursorY();
        }

        //#ifdef PPQN
        //#define LOOP_MARKERS
        //#define LOOP_LENGTH (PPQN * BEATS_PER_BAR * BARS_PER_PHRASE)

        // draw the menu display
        int display();

        int display_pinned();
        
        void update_ticks(unsigned long ticks) {
            if (pinned_panel!=nullptr)
                pinned_panel->update_ticks(ticks);

            //Serial.printf("update_ticks %i...\n", ticks);
            const int pages_count = this->pages->size();
            for (int p = 0 ; p < pages_count ; p++) {
                //Serial.printf("\tupdate_ticks for page %i...\n", p);
                const unsigned int items_count = this->pages->get(p)->items->size();
                for (unsigned int i = 0 ; i < items_count ; i++) {
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
                #ifdef ENCODER_DURING_SETUP // if knob is a pointer to an Encoder; so that we can instantiate the knob at runtime on eg RP2040 earlephilhower so that interrupts dont get trashed https://github.com/PaulStoffregen/Encoder/pull/85
                    new_knob_read = knob->read() / ENCODER_STEP_DIVISOR;
                #else
                    new_knob_read = knob.read() / ENCODER_STEP_DIVISOR;
                #endif
                if (new_knob_read!=last_knob_read) {
                    Debug_printf("new_knob_read %i changed from %i\n", new_knob_read, last_knob_read);
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

        uint16_t get_next_colour() {
            static int index = 0;
            static const uint16_t colours[] = {
                tft->rgb(255,200,200),  // bright pastel colours
                tft->rgb(200,255,200),
                tft->rgb(200,200,255),
                tft->rgb(200,255,255),
                tft->rgb(255,200,255),
                tft->rgb(255,255,200),
                tft->rgb(255,100,100),
                tft->rgb(100,255,100),
                tft->rgb(100,100,255),
                tft->rgb(255,100,100),
                tft->rgb(255,100,255),
                tft->rgb(100,255,255),
                tft->rgb(255,255,100)
            };
            index%=(sizeof(colours)/sizeof(uint16_t));
            return colours[index++];
        }

    void updateDisplay() {
        tft->updateDisplay();
    }

};

extern Menu *menu;

#endif

