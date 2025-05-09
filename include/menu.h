#if (defined __GNUC__) && (__GNUC__ >= 5) && (__GNUC_MINOR__ >= 3) && (__GNUC_PATCHLEVEL__ >= 1)
    #pragma GCC diagnostic ignored "-Wpragmas"
    #pragma GCC diagnostic ignored "-Wformat-truncation"
    #pragma GCC diagnostic ignored "-Wstringop-truncation"
#endif

#ifndef MENU_LIB_INCLUDED
#define MENU_LIB_INCLUDED

#define LONGPRESS_MILLIS 250    // how long is considered to be a 'long press' of button

#define MENU_MESSAGE_MAX (MENU_C_MAX*2)

#define MAX_MESSAGE_LOG 20

#ifndef CORE_TEENSY
    // if no FLASHMEM then we're probably not running on Teensy platform, so define it empty
    #ifndef FLASHMEM
        #define FLASHMEM
    #endif

    //#define F(x) { x }
#endif

#ifndef CALLOC_FUNC
    #define CALLOC_FUNC calloc
#endif
#ifndef MALLOC_FUNC
    #define MALLOC_FUNC malloc
#endif

#include "debug.h"

#include "mymenu.h"
#include "menu_io.h"

#include <LinkedList.h>
#include "menuitems.h"
//#include "menuitems_pinned.h"

//FLASHMEM // causes a section type conflict with 'void Menu::add(LinkedList<MenuItem*>*, uint16_t)'
//void setup_menu(bool button_pressed_state = HIGH);

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
    bool scrollable = true;
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

    bool profile_enable = false;
    char profile_string[MENU_C_MAX] = "profiler output";

    bool button_mode_rise_on_click = true;

    public:
        bool debug = false;
        bool debug_times = false;

        bool auto_update = true;    // whether to send update to tft at end of every display() call, or to allow host app to decide

        bool recalculate_bottoms = false;

        Menu(DisplayTranslator *dt, bool button_mode_rise_on_click = false) {
            this->tft = dt;
            this->pages = new LinkedList<page_t*>();
            this->select_page(this->add_page("Main"));
            this->screen_height_cutoff = (int)(0.75f*(float)tft->height());

            //this->button_mode_rise_on_click = button_mode_rise_on_click;
            #ifdef PIN_BUTTON_A
                pushButtonA.setPressedState(button_mode_rise_on_click);
            #endif
            #ifdef PIN_BUTTON_B
                pushButtonB.setPressedState(button_mode_rise_on_click);
            #endif
            #ifdef PIN_BUTTON_C
                pushButtonC.setPressedState(button_mode_rise_on_click);
            #endif
        }

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

        bool hide_pinned = false;
        int expand_pinned_level() {
            if (hide_pinned)
                return 0;
            if (!this->get_selected_page()->scrollable)
                return 1;
            return 2;
        }
        void set_hide_pinned(bool value) {
            this->hide_pinned = value;
        }
        bool get_hide_pinned() {
            return this->hide_pinned;
        }

        enum {
            NORMAL = 0,
            DISPLAY_ONE = 1
        };

        int mode = NORMAL;

        char last_message[MENU_C_MAX] = ""; //...started up...";
        uint32_t message_colour = C_WHITE;
        DisplayTranslator *tft;

        bool is_item_opened() {
            return this->selected_page->currently_opened!=-1;
        }
        bool is_page_opened() {
            return this->opened_page_index!=-1;
        }

        // input-handling stuff
        void knob_turned(int knob_position) {
            Debug_printf("knob_turned %i\n", knob_position);
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
            Debug_println("knob_left()");
            if (!is_page_opened()) {
                selected_page_index--;
                if (selected_page_index<0)
                    selected_page_index = pages->size() - 1;
                select_page(selected_page_index);
            } else if (is_item_opened()) { // && items->get(currently_opened)->knob_left()) {
                //Serial.printf(F("knob_left on currently_opened menuitem %i\n"), selected_page->currently_opened);
                selected_page->items->get(selected_page->currently_opened)->knob_left();
            } else {
                select_previous_selectable_item();
            }
            /*if (debug) {
                char msg[tft->get_c_max()] = "";
                sprintf(msg, "knob_left to %i", selected_page->currently_selected);
                set_last_message(msg);
            }*/
            return true;
        }
        bool knob_right() {
            Debug_println("knob_right()");
            if (!is_page_opened()) {
                selected_page_index++;
                if (selected_page_index>=(int)pages->size())
                    selected_page_index = 0;
                select_page(selected_page_index);
            } else if (is_item_opened()) { //&& items->get(currently_opened)->knob_right()) {
                //Serial.printf(F("knob_right on currently_opened menuitem %i\n"), selected_page->currently_opened);
                selected_page->items->get(selected_page->currently_opened)->knob_right();
            } else {
                select_next_selectable_item();
            }
            /*if (debug) {
                char msg[tft->get_c_max()] = "";
                sprintf(msg, "knob_left to %i", selected_page->currently_selected);
                set_last_message(msg);
            }*/
            return true;
        }

        void select_first_selectable_item() {
            this->selected_page->currently_selected = -1;
            //this->select_next_selectable_item();
            const unsigned int size = this->selected_page->items->size();
            for (unsigned int i = 0 ; i < size ; i++) {
                if (this->selected_page->items->get(i)->is_selectable()) {
                    //if (Serial) Serial.printf("select_first_selectable_item on page %s found a selectable item at %i!\n", selected_page->title, i);
                    selected_page->currently_selected = i;
                    if (this->selected_page->items->size()==1) {
                        //if (Serial) Serial.printf("found only one item, so opening it too!\n", selected_page->title, i);
                        //selected_page->currently_opened = selected_page->currently_selected;
                        if (selected_page->items->get(i)->is_openable()) {
                            button_select(); button_select_released();
                        }
                    }
                    return;
                }
            }
            //if (Serial) Serial.println("select_first_selectable_item didn't find anything to select!");
        }
        void select_next_selectable_item() {
            int found = this->find_next_selectable_item();
            //Serial.printf("select_next_selectable_item found %i\n", found); Serial_flush();
            this->selected_page->currently_selected = found;
        }
        int find_next_selectable_item() {
            int current = this->selected_page->currently_selected;
            const int size = this->selected_page->items->size();
            //Serial.printf("find_next_selectable_item on page %i, starting with current=%i and size=%i..\n", this->opened_page_index, current, size);
            for (int c = (current+1)%size ; c < size+current ; c++) {
                //Serial.printf("find_next_selectable_item checking item at index %i..\n", c%size);
                if (this->selected_page->items->get(c%size)->is_selectable())
                    return c%size;
            }

            // nothing found selectable - but if currently selected is selectable then just use that
            if (current>=0 && this->selected_page->items->get(current)->is_selectable())
                return current;

            //Serial.println("nothing selectable found, returning -1!");
            return -1;
        }
        void select_previous_selectable_item() {
            int found = this->find_previous_selectable_item();
            this->selected_page->currently_selected = found;
        }
        int find_previous_selectable_item() {
            int current = this->selected_page->currently_selected;
            const int size = this->selected_page->items->size();
            int c = current;

            do {
                c--;
                if (c<0)
                    c = size-1;
                if (this->selected_page->items->get(c%size)->is_selectable()) {
                    //Serial.printf("found at %i\n", c);
                    return c;
                }
            } while (c != current);

            // nothing found selectable - but if currently selected is selectable then just use that
            if (current>=0 && this->selected_page->items->get(current)->is_selectable())
                return current;

            //Serial.println("not found, returning -1");
            return -1;
        }

        bool button_select() {
            Debug_printf("Menu#button_select() on item %i\n", selected_page->currently_selected);
            if (!is_page_opened()) {
                open_page(selected_page_index);
            } else if (!is_item_opened()) {
                Debug_printf("button_select with currently_opened menuitem -1 - opening %i\n", selected_page->currently_selected);
                if (selected_page->items->get(selected_page->currently_selected)->action_opened()) {
                    selected_page->currently_opened = selected_page->currently_selected;
                    return false;
                }
            } else {
                Debug_printf("Menu#button_select() subselecting already-opened %i (%s)\n", selected_page->currently_opened, selected_page->items->get(selected_page->currently_opened)->label);
                if (selected_page->items->get(selected_page->currently_opened)->button_select()) 
                    button_back();
            } 
            return true;
        }
        bool button_select_released() {
            Debug_printf("Menu#button_select_released() on item %i\n", selected_page->currently_selected);
            if (!is_page_opened()) {
                // do nothing?
            } else if (!is_item_opened()) {
                /*Serial.printf(F("button_select_released with currently_opened menuitem -1 - opening %i\n"), currently_selected);
                if (items->get(currently_selected)->action_opened()) {
                    currently_opened = currently_selected;
                    return false;
                }*/
            } else {
                Debug_printf("Menu#button_select_released() subselecting already-opened %i (%s)\n", selected_page->currently_opened, selected_page->items->get(selected_page->currently_opened)->label);
                if (selected_page->items->get(selected_page->currently_opened)->button_select_released()) 
                    button_back();
            } 
            return true;
        }

        bool back_held = false; // variable for tracking whether a longpress is currently being processed, so that we don't continue to call longpress unnecessarily
        bool button_back() {
            Debug_println(F("button_back()"));
            back_held = false;
            if (!is_page_opened()) {
                // already at top level - do nothing?
            } else if (is_item_opened() && !selected_page->items->get(selected_page->currently_opened)->button_back()) {
                // an item is opened, and it responded false to button_back()
                Debug_printf("back with currently_opened menuitem %i and no subhandling, setting to -1\n", selected_page->currently_opened); 
                selected_page->currently_opened = -1;
                if (selected_page->items->size()==1) {
                    // if there is only one item on this page, close the page too
                    // todo: make this understand if there is only one SELECTABLE item on the page
                    opened_page_index = -1;
                }
            } else if (!is_item_opened()) {
                // no item is opened
                Debug_printf("back pressed but already at top level with currently_opened menuitem %i\n", selected_page->currently_opened); //setting to -1\n", currently_opened);
                selected_page->currently_selected = -1;
                opened_page_index = -1;
            } else {
                Debug_printf("back with currently_opened menuitem %i, handled by selected\n", selected_page->currently_opened); //setting to -1\n", currently_opened);
            }
            tft->clear(true);   // TOOD: don't rely on this
            return true;
        }
        bool button_back_longpress() {
            if (!back_held) {
                back_held = true;
                select_page_quickjump();
            } 
            return false;
        }
        bool button_right() {
            Debug_println("button_right()");
            if (!is_page_opened()) {
                open_page(selected_page_index);
            } else if (is_item_opened()) {
                if (selected_page->items->get(selected_page->currently_opened)->button_right()) {
                    Debug_printf("right with currently_opened menuitem %i subhandled!\n", selected_page->currently_opened);
                } else {
                    Debug_printf("right with currently_opened menuitem %i not subhandled!\n", selected_page->currently_opened);
                }
            } else {
                Debug_printf("right with nothing currently_opened\n"); //setting to -1\n", currently_opened);
            }
            return true;
        }

        FLASHMEM int get_num_panels() {
            return this->selected_page->items->size();
        }

        FLASHMEM
        void set_screen_height_cutoff(int cutoff) {
            this->screen_height_cutoff = cutoff;
        }

        FLASHMEM
        void set_screen_height_cutoff(float cutoff) {
            set_screen_height_cutoff((int)(cutoff * ((float)tft->height()-(3 * tft->getRowHeight()))));
        }

        // FLASHMEM // void setup_menu() causes a section type conflict with int Menu::add_page(const char*, uint16_t)
        int add_page(const char *title, uint16_t colour = C_WHITE, bool scrollable = true) {
            //Serial.printf("add_page(%s) has current size of %i\n", title, this->pages->size());
            return insert_page(title, this->pages->size()>0 ? this->pages->size() : 0, colour, scrollable);
        }
        //FLASHMEM //causes a section type conflict with 'virtual void DeviceBehaviour_Keystep::setup_callbacks()'
        int insert_page(const char *title, unsigned int position, uint16_t colour = C_WHITE, bool scrollable = true) {
            //Serial.printf("insert_page() passed position %i\n", position);
            if (position > this->pages->size()) 
                position = this->pages->size();

            page_t *p = new page_t;
            p->title = (new String(title))->c_str();
            p->colour = colour;
            p->scrollable = scrollable;

            this->pages->add(position, p);

            this->select_page(position);
            selected_page->items = new LinkedList<MenuItem*>();
            return position;
        }

        FLASHMEM void setup_display() {
            Debug_println("setup_display about to do tft->setup().."); Serial_flush();
            tft->setup();
            Debug_println("tft->setup() done!"); Serial_flush();
        }
        #ifndef GDB_DEBUG
        FLASHMEM 
        #endif
        void start() {
            tft->start();
        }

        static const int_least8_t NUM_QUICK_PAGE_HISTORY = 10;
        uint_least8_t quick_page_history_head = 0;
        uint_least8_t quick_page_history_total = 0;
        int_least8_t quick_page_index = 0;
        int_least8_t quick_pages[NUM_QUICK_PAGE_HISTORY] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
        // add the page index to the 'visited history'
        void remember_opened_page(int8_t page_index = -1) {
            if (page_index==-1)
                page_index = selected_page_index;
            // don't store qjump page itself..
            if (page_index==quick_page_index) 
                return;

            // don't add this item if it already exists
            for (int i = 0 ; i < NUM_QUICK_PAGE_HISTORY ; i++) {    
                if (this->quick_pages[i]==page_index)
                    return;
            }

            // add the page to the rolling history
            this->quick_pages[this->quick_page_history_head] = page_index;

            // wrap history where necessary
            quick_page_history_head++;
            if (quick_page_history_head >= NUM_QUICK_PAGE_HISTORY)
                quick_page_history_head = 0;

            if (quick_page_history_total < NUM_QUICK_PAGE_HISTORY) 
                quick_page_history_total++;
        }
        page_t *get_quick_page(unsigned int page_index) {
            if (quick_pages[page_index]==-1)
                return nullptr;
            return pages->get(quick_pages[page_index]);
        }
        void setup_quickjump();
        void select_page_quickjump() {
            if (quick_page_index>=0) {
                selected_page_index = opened_page_index = -1;
                this->open_page(this->quick_page_index);
            }
        }

        // get the index of a page based on title string; returns -1 if not found
        int get_page_index_for_name(const char *title) {
            for (unsigned int i = 0 ; i < pages->size() ; i++) 
                if (strcmp(pages->get(i)->title, title)==0)
                    return i;
            return -1;
        }

        page_t *get_selected_page() {
            return this->selected_page;
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
            // unselect current first
            if (this->selected_page!=nullptr)
                this->selected_page->currently_selected = this->selected_page->currently_opened = -1;

            this->opened_page_index = -1;
            this->selected_page_index = p;
            if (selected_page_index >= (int)pages->size())
                this->selected_page_index = 0;
            else if (selected_page_index < 0 )
                this->selected_page_index = pages->size() - 1;

            selected_page = pages->get(selected_page_index);
            //Serial.printf("Selected page %i\n", selected_page_index);
        }
        void select_page_for_name(const char *name) {
            select_page(get_page_index_for_name(name));
        }
        void open_page(unsigned int page_index) {
            //Serial.printf("open_page %i\n", page_index);
            page_index = constrain(page_index, (unsigned int)0, pages->size() - 1);

            select_page(page_index);

            //Serial.printf("opening page %i, currently_selected is %i\n", page_index, selected_page->currently_selected);

            // select first selectable item 
            if (selected_page!=nullptr) {
                opened_page_index = page_index;

                select_first_selectable_item();
                if (selected_page->currently_selected==-1) {
                    // close the page, as couldn't find a selectable item on it!
                    this->opened_page_index = -1;
                }
            }

            //if (is_page_opened())
            this->remember_opened_page(page_index);

            //Serial.printf("=> currently_selected is now %i\n", selected_page->currently_selected);
        }       

        // add a linkedlist of menuitems; delete the LinkedList object when finished!
        FLASHMEM 
        void add(LinkedList<MenuItem *> *items, uint16_t default_fg_colour = C_WHITE) {
            //Serial.println("starting add"); Serial.flush();
            if (items==nullptr) {
                if (Serial) Serial.println("WARNING: nullptr list passed to menu#add, skipping!");
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
        // FLASHMEM // void setup_menu() causes a section type conflict with void Menu::add(MenuItem*, uint16_t)
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
            if (pinned_panel==nullptr) {
                // first pinned panel, just add it 
                if (m!=nullptr) {
                    m->tft = this->tft;
                    m->on_add();
                }
                pinned_panel = m;
            } else {
                // second pinned panel, jump through some hoops to add both
                DoublePinnedPanelMenuItem *dpp = new DoublePinnedPanelMenuItem(pinned_panel, m);
                if (m!=nullptr) {
                    m->tft = this->tft;
                    m->on_add();
                }
                this->pinned_panel = dpp;
            }
        }

        // set the LinkedList where to store the messages
        FLASHMEM void set_messages_log(LinkedList<String> *messages_log) {
            this->messages_log = messages_log;
        }

        // add message to the message history
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

        // render the last message to screen in the correct colours etc
        int draw_message() {
            //tft.setCursor(0,0);
            // draw the last status message
            tft->setTextColor(message_colour,BLACK);
            tft->setTextSize(tft->default_textsize);
            tft->printf(tft->get_message_format(), last_message);
            tft->setCursor(0,tft->getCursorY()+3);  // workaround for tab positions?
            return tft->getCursorY();
        }

        // draw the menu display
        int display();

        // display the 'pinned' controls
        int display_pinned();
        
        // call when ticks have updated, to notify all menuitem controls in case they need to do something
        void update_ticks(unsigned long ticks) {
            // avoid updating twice for the same tick
            static uint32_t last_updated_ticks = 0;
            if (ticks==last_updated_ticks)
                return;
            last_updated_ticks = ticks;

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

        // check encoder and buttons and fire off events if necessary
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
                    if ( pushButtonA.pressed() ) {
                        button_count++;
                        button_select();
                    } else if ( pushButtonA.released() ) {
                        button_select_released();
                    }
                }
            #endif
            #ifdef PIN_BUTTON_B
                if (pushButtonB.update()) {
                    if ( pushButtonB.released() && pushButtonB.previousDuration()<=LONGPRESS_MILLIS) {
                        button_count++;
                        button_back();
                    } /*else if (pushButtonB.isPressed()) {
                        Serial.printf("B button is pressed, duration is %i!\n", pushButtonB.currentDuration());
                    }*/
                } else if ( pushButtonB.isPressed() && pushButtonB.currentDuration()>LONGPRESS_MILLIS ) {
                    button_back_longpress();
                }                    
            #endif
            #ifdef PIN_BUTTON_C
                if (pushButtonC.update()) {
                    if ( pushButtonC.pressed() ) {
                        button_count++;
                        button_right();
                    } /*else if (pushBufttonC.fell()) {
                        button_right_released();
                    }*/
                }
            #endif
        }

        uint16_t get_next_colour() {
            static int index = 0;
            // https://rgbcolorpicker.com/565/table
            static const uint16_t colours[] = {
                tft->rgb(255,200,200),  // bright pastel colours
                tft->rgb(200,255,200),
                tft->rgb(200,200,255),
                tft->rgb(200,255,255),
                tft->rgb(255,200,255),
                tft->rgb(255,255,200),
                tft->rgb(255,100,100),
                tft->rgb(100,255,100),
                0x867d, // skyblue //tft->rgb(117, 117, 255),
                tft->rgb(255,100,100),
                tft->rgb(255,100,255),
                tft->rgb(100,255,255),
                tft->rgb(255,255,100)
            };
            index%=(sizeof(colours)/sizeof(uint16_t));
            return colours[index++];
        }

    // tell the DisplayTranslator to display its buffer
    void updateDisplay() {
        tft->updateDisplay();
    }

};

extern Menu *menu;

#endif

