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
//#include "menuitems_pinned.h"

// from midi_helpers library
String get_note_name(int pitch);
const char *get_note_name_c(int pitch);

class HarmonyStatus : public MenuItem {
    int *last_note = nullptr;
    int *current_note = nullptr;
    int *other_value = nullptr;

    public:
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

extern const char *fired_message;
extern const char *sure_message;

class ActionItem : public MenuItem {
    public:

    char button_label[20] = "";
    void(*on_open)() = nullptr;

    ActionItem(const char *label, void (*on_open)()) : MenuItem(label) {
        this->on_open = on_open;
        snprintf(button_label, 20, ">> %s <<", label);
    }

    virtual int display(Coord pos, bool selected, bool opened) override {
        //int textSize = ((int)strlen(button_label)*tft->currentCharacterWidth() < tft->width()/2 );
        int textSize = tft->get_textsize_for_width(button_label, tft->width());
        pos.y = header(button_label, pos, selected, opened, textSize);
        tft->setCursor(pos.x,pos.y);

        colours(opened, opened ? GREEN : this->default_fg, this->default_bg);

        return tft->getCursorY() + 1;
    }

    virtual bool action_opened() override {
        //Debug_println(F("ActionItem#action_opened"));
        this->on_open();

        char msg[MENU_MESSAGE_MAX];
        //Serial.printf("about to build msg string...\n");
        snprintf(msg, MENU_MESSAGE_MAX, fired_message, label);
        //Serial.printf("about to set_last_message!");
        //msg[tft->get_c_max()] = '\0'; // limit the string so we don't overflow set_last_message
        menu_set_last_message(msg,GREEN);

        return false;   // don't 'open'
    }
};

class ActionFeedbackItem : public MenuItem {
    public:

    char button_label_false[20] = "";
    char button_label_true[20] = "";
    //void(*on_open)() = nullptr;

    using setter_def_2 = void(*)();
    using setter_def = void(*)(bool);
    //using get_label_def = char*(TargetClass::*)(bool); //, get_label_def *get_label
    using getter_def = bool(*)();

    setter_def setter = nullptr;
    setter_def_2 setter2 = nullptr;
    getter_def getter = nullptr;

    ActionFeedbackItem(const char *label, setter_def setter) : 
        MenuItem(label) {
            this->setter = setter;
        }
    ActionFeedbackItem(const char *label, setter_def_2 setter) : 
        MenuItem(label) {
            this->setter2 = setter;
        }

    ActionFeedbackItem(const char *label, getter_def getter, const char *button_label_true, const char *button_label_false = nullptr) 
        : MenuItem(label) {
        this->getter = getter;

        if (button_label_true!=nullptr)
            strncpy(this->button_label_true, button_label_true, 20);
        else
            snprintf(this->button_label_true, 20, label);

        if (button_label_false!=nullptr)
            snprintf(this->button_label_false, 20, button_label_false);
        else
            snprintf(this->button_label_false, 20, label);
    }

    ActionFeedbackItem(const char *label, setter_def setter, getter_def getter, const char *button_label_true, const char *button_label_false = nullptr) 
        : ActionFeedbackItem(label, getter, button_label_true, button_label_false) {
            this->setter = setter;
        }

    ActionFeedbackItem(const char *label, setter_def_2 setter, getter_def getter, const char *button_label_true, const char *button_label_false = nullptr) 
        : ActionFeedbackItem(label, getter, button_label_true, button_label_false) {
            this->setter2 = setter;
        }

    virtual int display(Coord pos, bool selected, bool opened) override {
        this->tft->setCursor(pos.x, pos.y);
        return this->renderValue(selected, opened, MENU_C_MAX);
    }

    virtual int renderValue(bool selected, bool opened, uint16_t max_character_width) override {
        char *button_label = nullptr;
        if (this->getter!=nullptr && this->button_label_true[0]) {
            button_label = (this->getter)() ? this->button_label_true : this->button_label_false;
        } else if (button_label_false[0]) {
            //Serial.printf(F("%s: rendering button_label_false '%s'\n"), this->label, this->button_label_false);
            button_label = this->button_label_false;
        } else {
            button_label = label;
        }
        //int y = header(button_label, Coord(this->tft->getCursorX(), this->tft->getCursorY()), selected, opened);
        colours(selected);

        // determine size font to use
        //bool use_small = strlen(button_label) <= (max_character_width/2);
        //int textSize = use_small ? 2 : 1;
        int textSize = tft->get_textsize_for_width(button_label, tft->width());
        tft->setTextSize(textSize);

        tft->println(button_label);
        const int y = tft->getCursorY();
        return y;
    }

    virtual bool action_opened() override {
        //Serial.println(F("ObjectActionItem#action_opened"));
        //this->on_open();
        if (this->setter!=nullptr)
            (this->setter)(true);
        else if (this->setter2!=nullptr)
            (this->setter2)();

        char msg[MENU_MESSAGE_MAX];
        //Serial.printf("about to build msg string...\n");
        snprintf(msg, MENU_MESSAGE_MAX, fired_message, label);
        //Serial.printf("about to set_last_message!");
        //msg[tft->get_c_max()] = '\0'; // limit the string so we don't overflow set_last_message
        menu_set_last_message(msg,GREEN);

        return false;   // don't 'open'
    }

};

class ActionConfirmItem : public ActionItem {
    public:

    ActionConfirmItem(const char *label, void (*on_open)()) : ActionItem(label, on_open) {
        go_back_on_select = true;
    }

    virtual int display(Coord pos, bool selected, bool opened) override {
        const char *text_to_render = opened ? sure_message : button_label;

        int textSize = tft->get_textsize_for_width(text_to_render, tft->width());
        pos.y = header(text_to_render, pos, selected, opened, textSize);

        //tft->setCursor(pos.x,pos.y);
        //tft->setTextSize(textSize);
        colours(opened, opened ? GREEN : this->default_fg, this->default_bg);

        return pos.y; //tft->getCursorY();
    }

    virtual bool action_opened() override {
        //Debug_println(F("ActionConfirmItem#action_opened"));
        //this->on_open();
        return true; 
    }

    virtual bool button_select() override {
        //Debug_println(F("ActionConfirmItem#button_select"));

        this->on_open();

        char msg[MENU_MESSAGE_MAX];
        //Serial.printf("about to build msg string...\n");
        snprintf(msg, MENU_MESSAGE_MAX, fired_message, label);
        //Serial.printf("about to set_last_message!");
        //msg[tft->get_c_max()] = '\0'; // limit the string so we don't overflow set_last_message
        menu_set_last_message(msg,GREEN);

        return go_back_on_select;    // return to menu
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


template<class TargetClass>
class ToggleControl : public MenuItem {
    public:
        bool *target_variable;
        void (*on_change_handler)(bool last_value, bool new_value) = nullptr;

        ToggleControl(
            const char *label, 
            bool *target_variable, 
            void (*on_change_handler)(bool last_value, bool new_value) = nullptr
        ) : MenuItem(label) {
            this->target_variable = target_variable;
            this->on_change_handler = on_change_handler;
        }

        virtual int display(Coord pos, bool selected, bool opened) override {
            //int textSize = ((int)strlen(label)*tft->currentCharacterWidth() < tft->width()/2 );
            int textSize = tft->get_textsize_for_width(label, tft->width());

            pos.y = header(label, pos, selected, opened, textSize);
            tft->setCursor(pos.x,pos.y);

            colours(opened, opened ? GREEN : this->default_fg, BLACK);
            //tft->setTextSize(2);        // was 2 ?
            //char tmp[MENU_C_MAX] = "";
            tft->setTextSize(2);

            this->renderValue(selected, opened, MENU_C_MAX);

            return tft->getCursorY();
        }

        // render the current value at current position
        virtual int renderValue(bool selected, bool opened, uint16_t max_character_width) override {
            const char *txt = *this->target_variable ? label_on : label_off;
            //bool use_small = strlen(txt) <= (max_character_width/2);
            //int textSize = use_small ? 2 : 1;
            int textSize = tft->get_textsize_for_width(txt, tft->width());
            //if (this->debug) Serial.printf(F("%s:\trenderValue '%s' (len %i) with max_character_width %i got textSize %i\n"), this->label, txt, strlen(txt), max_character_width/2, textSize);
            tft->setTextSize(textSize);
            tft->println(txt);
            return tft->getCursorY();
        }

        virtual bool action_opened() override {
            //if (this->debug) Serial.printf(F("ObjectToggleControl#action_opened on %s\n"), this->label);
            //bool value = !(this->target_object->*getter)();
            //this->internal_value = !this->internal_value;
            *this->target_variable = !*this->target_variable;

            //(this->target_object->*setter)(value); //(bool)this->internal_value);
            return false;   // don't 'open'
        }
};


#include "menuitems_object.h"

#include "menuitems_fileviewer.h"

#endif