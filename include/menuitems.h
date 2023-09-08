#pragma GCC diagnostic ignored "-Wformat-truncation"
#pragma GCC diagnostic ignored "-Wstringop-truncation"

#ifndef MENUITEMS__INCLUDED
#define MENUITEMS__INCLUDED

#include <Arduino.h>

//#include "menu.h"
#include "display_abstract.h"
#include "colours.h"

#define MAX_LABEL_LENGTH 40

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

        MenuItem(const char *in_label) {
            strcpy(label, in_label);
        }
        virtual void on_add();
        virtual void update_label(const char *new_label);

        MenuItem *set_default_colours(uint16_t fg, uint16_t bg = BLACK);

        // called every tick, in case anything needs doing
        virtual void update_ticks(unsigned long ticks) {
            // nothing to do by default
        };
        
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

// from midi_helpers library
String get_note_name(int pitch);
const char *get_note_name_c(int pitch);

class HarmonyStatus : public MenuItem {
    public:

        int *last_note = nullptr;
        int *current_note = nullptr;
        int *other_value = nullptr;

        HarmonyStatus(const char *label) : MenuItem(label) {
            this->selectable = false;
        };
        HarmonyStatus(const char *label, int *last_note, int *current_note) : HarmonyStatus(label) {
            //MenuItem(label);
            this->last_note = last_note;
            this->current_note = current_note;
        }
        HarmonyStatus(const char *label, int *last_note, int *current_note, int *other_value) : HarmonyStatus(label, last_note, current_note) {
            this->other_value = other_value;
        }
        /*virtual void configure(int *last_note, int *current_note) {   // for if we need to late-bind the harmony note sources
            this->last_note = last_note;
            this->current_note = current_note;   
        }*/
        virtual int display(Coord pos, bool selected, bool opened) override {
            tft->setCursor(pos.x, pos.y);
            header(label, pos, selected, opened);
            //tft->setTextColor(rgb(0xFFFFFF),0);
            tft->setTextSize(2);
            colours(opened);

            if (!last_note || !current_note) {
                tft->println((char *)"[not set]");
            } else if (this->other_value!=nullptr) {
                tft->printf("%4s : %4s : %4s\n",     // \n not needed on smaller screen because already fills row.. is needed on big tft?
                    (char*)(get_note_name_c(*last_note)), 
                    (char*)(get_note_name_c(*current_note)),
                    (char*)(get_note_name_c(*other_value))
                );
            } else {
                tft->printf("%4s : %4s\n",     // \n not needed on smaller screen because already fills row.. is needed on big tft?
                    (char*)(get_note_name_c(*last_note)), 
                    (char*)(get_note_name_c(*current_note))
                );
            }
            return tft->getCursorY();
        }
};

class SeparatorMenuItem : public MenuItem {
    public:
        //int16_t colour = C_WHITE;
        SeparatorMenuItem(const char *label) : MenuItem(label) {
            this->selectable = false;
        }
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