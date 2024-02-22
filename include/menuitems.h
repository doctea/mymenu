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

class Coord {
    public:
        int x, y;
        Coord(int in_x, int in_y) {
            x = in_x;
            y = in_y;
        }
};

void menu_set_last_message(const char *msg, int colour);

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

        MenuItem set_tft(DisplayTranslator *tft) {
            this->tft = tft;
            return *this;
        }

        MenuItem(const char *in_label, bool selectable = true) {
            strncpy(label, in_label, MAX_LABEL_LENGTH);
            this->selectable = selectable;
        }
        virtual void on_add();
        virtual void update_label(const char *new_label);

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

        PinnedPanelMenuItem(const char *label) : MenuItem(label) {};

        virtual void update_ticks(unsigned long ticks) override {
            this->ticks = ticks;
        }
};


#include "menuitems_numbers.h"
#include "menuitems_selector.h"
#include "menuitems_action.h"
//#include "menuitems_pinned.h"

class SeparatorMenuItem : public MenuItem {
    public:
        //int16_t colour = C_WHITE;
        SeparatorMenuItem(const char *label) : MenuItem(label, false) {}
        SeparatorMenuItem(const char *label, uint16_t default_fg) : SeparatorMenuItem(label) {
            this->default_fg = default_fg;
        }

        virtual int display(Coord pos, bool selected, bool opened) override;
        virtual int header(const char *text, Coord pos, bool selected = false, bool opened = false, int textSize = 0);
};

#include "menuitems_toggle.h"
#include "menuitems_object.h"
#include "menuitems_fileviewer.h"

#endif