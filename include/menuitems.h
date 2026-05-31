#if (defined __GNUC__) && (__GNUC__ >= 5) && (__GNUC_MINOR__ >= 3) && (__GNUC_PATCHLEVEL__ >= 1)
    #pragma GCC diagnostic ignored "-Wpragmas"
    #pragma GCC diagnostic ignored "-Wformat-truncation"
    #pragma GCC diagnostic ignored "-Wstringop-truncation"
#endif

#ifndef MENUITEMS__INCLUDED
#define MENUITEMS__INCLUDED

#include <Arduino.h>

//#include "menu.h"
#include "display_abstract.h"
#include "colours.h"

#define MAX_LABEL_LENGTH 40

extern const char *fired_message;
extern const char *sure_message;
extern const char *set_message;
extern const char *label_on;
extern const char *label_off;

#ifndef MENU_SELECTIVE_STATIC_REDRAW
    #define MENU_SELECTIVE_STATIC_REDRAW 0
#endif

enum MenuItem_RedrawPolicy {
    REDRAW_ALWAYS = 0,          // always redraw (default, safest)
    REDRAW_ON_SELECTION = 1,    // redraw only if selection state changed
    REDRAW_ON_OPEN_STATE = 2,   // redraw only if opened state changed
    REDRAW_ON_SELECTION_OR_OPEN = 3, // redraw if either selection or opened state changed
    REDRAW_ON_VALUE_CHANGE = 4  // redraw only if value changed (for dynamic controls)
};

class Coord {
    public:
        int x, y;
        Coord(int in_x, int in_y) {
            x = in_x;
            y = in_y;
        }
};

void menu_set_last_message(const char *msg, int colour);

// Lightweight flat-array list to replace LinkedList<option> in selector controls.
// Eliminates per-entry malloc overhead (LinkedList paid 8-byte alloc overhead per 8-byte option node).
template<typename T>
class OptionList {
    T* _items = nullptr;
    uint16_t _count = 0;
    uint16_t _capacity = 0;

    void grow() {
        uint16_t new_cap = (_capacity == 0) ? 8 : (_capacity + 8);
        T* new_items = (T*)realloc(_items, new_cap * sizeof(T));
        if (new_items == nullptr) return;
        _items = new_items;
        _capacity = new_cap;
    }

public:
    OptionList() = default;
    ~OptionList() { free(_items); }
    OptionList(const OptionList&) = delete;
    OptionList& operator=(const OptionList&) = delete;

    void add(T item) {
        if (_count >= _capacity) grow();
        if (_count < _capacity) _items[_count++] = item;
    }
    T& get(int idx) { return _items[idx]; }
    const T& get(int idx) const { return _items[idx]; }
    uint16_t size() const { return _count; }
    bool empty() const { return _count == 0; }
    void clear() { _count = 0; }

    T* begin() { return _items; }
    T* end()   { return _items + _count; }
    const T* begin() const { return _items; }
    const T* end()   const { return _items + _count; }
};

// basic line
class MenuItem {
    public:
        bool debug = false;

        DisplayTranslator *tft = nullptr;
        int menu_c_max = MENU_C_MAX;

        char label[MAX_LABEL_LENGTH];

        uint16_t default_fg = C_WHITE; //0xFFFF;
        uint16_t default_bg = BLACK;

        bool show_header = true;
        bool selectable = true;
        bool go_back_on_select = false;

        // Cache text-size decisions for static labels to avoid repeated width scans.
        int8_t cached_label_textsize = -1;
        uint16_t cached_label_width_px = 0;

        #if MENU_SELECTIVE_STATIC_REDRAW
            MenuItem_RedrawPolicy redraw_policy = REDRAW_ALWAYS;
            int8_t last_rendered_selected = -1;
            int8_t last_rendered_opened = -1;
            bool needs_redraw(bool current_selected, bool current_opened) const {
                if (redraw_policy == REDRAW_ALWAYS) return true;
                if (redraw_policy == REDRAW_ON_SELECTION && (int8_t)current_selected != last_rendered_selected) return true;
                if (redraw_policy == REDRAW_ON_OPEN_STATE && (int8_t)current_opened != last_rendered_opened) return true;
                if (redraw_policy == REDRAW_ON_SELECTION_OR_OPEN && (
                    (int8_t)current_selected != last_rendered_selected ||
                    (int8_t)current_opened != last_rendered_opened
                )) return true;
                return false;
            }
            void mark_rendered(bool selected, bool opened) {
                last_rendered_selected = (int8_t)selected;
                last_rendered_opened = (int8_t)opened;
            }
        #endif

        MenuItem set_tft(DisplayTranslator *tft) {
            this->tft = tft;
            return *this;
        }

        MenuItem(const char *in_label, bool selectable = true, bool show_header = true) {
            strncpy(label, in_label, MAX_LABEL_LENGTH);
            label[MAX_LABEL_LENGTH - 1] = '\0';
            this->selectable = selectable;
            this->show_header = show_header;
        }
        virtual void on_add();
        virtual void update_label(const char *new_label);
        virtual void invalidate_render_cache();
        virtual int get_textsize_for_label(uint16_t max_width_px);

        MenuItem *set_default_colours(uint16_t fg, uint16_t bg = BLACK);

        // called every tick, in case anything needs doing
        virtual void update_ticks(unsigned long ticks) {
            // nothing to do by default
        };

        virtual const char *get_label() {
            return this->label;
        }
        
        virtual int display(Coord pos, bool selected, bool opened);
        virtual int renderValue(bool selected, bool opened, uint16_t max_character_width);

        virtual void colours(bool inverted);
        virtual void colours(bool inverted, uint16_t fg);
        virtual void colours(bool inverted, uint16_t fg, uint16_t bg);
        
        virtual int header(const char *text, Coord pos, bool selected = false, bool opened = false, int textSize = 0);

        // called when item is selected ie opened from the main menu - return true to open, return false to 'refuse to open'
        virtual bool action_opened();

        // default to returning true to exit out to main menu after setting (IF OPENED, otherwise button_select is not sent!)
        virtual bool button_select();
        virtual bool button_select_released();
        virtual bool button_back();
        virtual bool button_right();
        virtual bool knob_left();
        virtual bool knob_right();
        virtual bool allow_takeover();
        // When true, SubMenuItemBar will render this control once more after row layout
        // so it can draw an overlay/popout while opened.
        virtual bool wants_fullscreen_overlay_when_opened_in_bar() { return false; }
        // whether we should be allowed to hover over this one
        virtual bool is_selectable ();
        // whether 'tis openable -- ie, that it can be 'opened' without having an effect, eg submenuitem... basically anything except an action?!
        virtual bool is_openable () {
            return true;
        }
};

// TODO: verify that this is actually what happens!
#define SELECT_EXIT true
#define SELECT_DONTEXIT false

// this IS actually what happens
#define BACK_EXIT false
#define BACK_DONTEXIT true

class FixedSizeMenuItem : public MenuItem {
    public:
    int fixed_size = 0;
    FixedSizeMenuItem(const char *label, int size) : MenuItem(label) {
        this->fixed_size = size;
    }
    virtual int renderValue(bool selected, bool opened, uint16_t max_character_width);
};

class PinnedPanelMenuItem : public MenuItem {
    public:
        unsigned long ticks = 0;

        #if MENU_SELECTIVE_STATIC_REDRAW
            bool redraw_needed = true;
            int16_t cached_draw_height = 0;

            virtual void request_redraw() {
                redraw_needed = true;
            }
            virtual void refresh_redraw_state() {
                // default no-op
            }
            virtual bool should_redraw() const {
                return redraw_needed || cached_draw_height <= 0;
            }
            virtual int16_t get_cached_draw_height() const {
                return cached_draw_height;
            }
            virtual void mark_drawn(int16_t draw_height) {
                cached_draw_height = (draw_height > 0) ? draw_height : 0;
                redraw_needed = false;
            }
        #endif

        PinnedPanelMenuItem(const char *label) : MenuItem(label) {};

        virtual void update_ticks(unsigned long ticks) override {
            #if MENU_SELECTIVE_STATIC_REDRAW
                if (this->ticks != ticks) {
                    request_redraw();
                }
            #endif
            this->ticks = ticks;
        }
};

class DoublePinnedPanelMenuItem : public PinnedPanelMenuItem {
    public:
        unsigned long ticks = 0;
        PinnedPanelMenuItem *item1, *item2;

        DoublePinnedPanelMenuItem(PinnedPanelMenuItem *item1, PinnedPanelMenuItem *item2) : PinnedPanelMenuItem(label) {
            this->item1 = item1;
            this->item2 = item2;
        };

        virtual void update_ticks(unsigned long ticks) override {
            if (this->item1!=nullptr) this->item1->update_ticks(ticks);
            if (this->item2!=nullptr) this->item2->update_ticks(ticks);
            this->ticks = ticks;

            #if MENU_SELECTIVE_STATIC_REDRAW
                if ((this->item1!=nullptr && this->item1->should_redraw()) ||
                    (this->item2!=nullptr && this->item2->should_redraw())) {
                    this->request_redraw();
                }
            #endif
        }

        #if MENU_SELECTIVE_STATIC_REDRAW
            virtual void refresh_redraw_state() override {
                if (this->item1!=nullptr) this->item1->refresh_redraw_state();
                if (this->item2!=nullptr) this->item2->refresh_redraw_state();

                if ((this->item1!=nullptr && this->item1->should_redraw()) ||
                    (this->item2!=nullptr && this->item2->should_redraw())) {
                    this->request_redraw();
                }
            }
        #endif

        int display(Coord pos, bool selected, bool opened) override {
            int y = pos.y;
            if (this->item1!=nullptr) {
                y += this->item1->display(Coord(pos.x, y), selected, opened);
            }
            if (this->item2!=nullptr) {
                y += this->item2->display(Coord(pos.x, y), selected, opened);
            }
            return y;
        }
};


#include "menuitems_numbers.h"
#include "menuitems_selector.h"
#include "menuitems_action.h"
//#include "menuitems_pinned.h"

class SeparatorMenuItem : virtual public MenuItem {
    public:
        bool draw_lines = true;
        int textSize = 0;
        //int16_t colour = C_WHITE;
        SeparatorMenuItem(const char *label, int textSize = 0, bool draw_lines = true) : MenuItem(label, false) {
            this->textSize = textSize;
            this->draw_lines = draw_lines;
            #if MENU_SELECTIVE_STATIC_REDRAW
                redraw_policy = REDRAW_ON_SELECTION;  // separators only change on selection
            #endif
        }
        SeparatorMenuItem(const char *label, uint16_t default_fg, int textSize = 0, bool draw_lines = true) : SeparatorMenuItem(label, textSize, draw_lines) {
            this->default_fg = default_fg;
            // redraw_policy already set in primary constructor
        }

        virtual int display(Coord pos, bool selected, bool opened) override;
        virtual int header(const char *text, Coord pos, bool selected = false, bool opened = false, int textSize = 0);
};

#include "menuitems_toggle.h"
#include "menuitems_object.h"
#include "menuitems_fileviewer.h"

#endif