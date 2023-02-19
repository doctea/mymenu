#ifndef MENUITEMS__INCLUDED
#define MENUITEMS__INCLUDED

#include <Arduino.h>

#include "menu.h"
#include "colours.h"

#define MAX_LABEL_LENGTH 40

void menu_set_last_message(const char *msg, int colour);

// basic line
class MenuItem {
    public:
        bool debug = false;

        DisplayTranslator *tft = nullptr;
        int menu_c_max = MENU_C_MAX;

        char label[MAX_LABEL_LENGTH];

        const char *label_on    = "On";
        const char *label_off   = "Off";

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
        virtual void on_add() {
            Debug_printf(F("MenuItem#on_add in %s\n"), this->label);
            menu_c_max = tft->get_c_max();
        }    // called when this menuitem is added to menu

        virtual void update_label(const char *new_label) {
            Debug_printf("%s#update_label('%s')\n", this->label, new_label);
            strcpy(this->label, new_label);
        }

        MenuItem *set_default_colours(uint16_t fg, uint16_t bg = BLACK) {
            this->default_fg = fg;
            this->default_bg = bg;
            return this;
        }

        // called every tick, in case anything needs doing
        virtual void update_ticks(unsigned long ticks) {
        };
        
        virtual int display(Coord pos, bool selected, bool opened) {
            //Serial.printf("MenuItem display()")
            //tft_print("hello?");
            //char state[10];
            //if (selected) sprintf(state,"Sel");
            //if (opened) sprintf(&state[3]," Open");
            //sprintf(state,"%s%s",selected?'Sel':'   ', opened?'Ope':'   ');
            // display this item however that may be
            //tft->fillRect(random(20), random(20), random(20), random(20), C_WHITE);
            //Serial.printf("MenuItem: base display for %s at (%i,%i) [s:%i o:%i]\n", label, tft->getCursorX(), tft->getCursorY(), selected, opened);
            tft->setTextSize(0);
            tft->setCursor(pos.x,pos.y);
            //tft->setTextColor(ST77XX_WHITE, ST77XX_BLACK);
            colours(selected);
            //tft->printf("%s [s:%i o:%i]", label, (int)selected, (int)opened);
            this->renderValue(selected, opened, MENU_C_MAX);
            this->tft->println();
            //return (tft->getTextSizeY() * 8) + 2;
            return tft->getCursorY();
        }

        virtual int renderValue(bool selected, bool opened, uint16_t max_character_width) {
            //tft->printf("%s [s:%i o:%i]", label, (int)selected, (int)opened);
            colours(selected);
            tft->setTextSize((strlen(label) < max_character_width/2) ? 2 : 1 );
            tft->println(label);
            return tft->getCursorY();
        }

        virtual void colours(bool inverted) {
            colours(inverted, /*tft->WHITE*/this->default_fg, this->default_bg);
        }
        virtual void colours(bool inverted, uint16_t fg) {
            colours(inverted, fg, this->default_bg);
        }
        virtual void colours(bool inverted, uint16_t fg, uint16_t bg) {
            if (!inverted) {
                tft->setTextColor(fg, bg);
            } else {
                //Serial.printf("%s selected, setting colours %02x, %02x\n", label, bg, fg);
                tft->setTextColor(bg, fg) ;//ST77XX_BLACK, ST77XX_WHITE);
            }
        }
        
        virtual int header(const char *text, Coord pos, bool selected = false, bool opened = false, int textSize = 0) {
            if (!this->show_header) return pos.y;

            tft->drawLine(pos.x, pos.y, tft->width(), pos.y, this->default_fg);
            pos.y++;
            tft->setCursor(pos.x, pos.y);
            colours(selected, this->default_fg, this->default_bg);
            tft->setTextSize(textSize);
            if (opened) {
                //tft->print(">>>");
                //tft->printf((char*)"%-19s",(char*)text);   // \n not needed as reaching to edge
                tft->printf((char*)tft->get_header_open_format(), (char*)text);
            } else if (selected) {
                //tft->printf((char*)"%-22s",(char*)text);   // \n not needed as reaching to edge
                tft->printf((char*)tft->get_header_selected_format(), (char*)text);
                //tft->println();
            } else {
                tft->printf((char*)tft->get_header_format(), (char*)text);
            }
            colours(false);
            //return (tft->getTextSize()+1)*6;
            if (tft->getCursorY() <= pos.y) {
                // we havent had enough characters to move down a line, so force one
                tft->println();
            }
            return tft->getCursorY();
        }

        // called when item is selected ie opened from the main menu - return true to open, return false to 'refuse to open'
        virtual bool action_opened() {
            return true;
        }

        // default to returning true to exit out to main menu after setting (IF OPENED, otherwise button_select is not sent!)
        virtual bool button_select() {
            return go_back_on_select;
        }
        virtual bool button_select_released() {
            return false;
        }
        
        virtual bool button_back() {
            return false;
        }
        
        virtual bool button_right() {
            return false;
        }

        virtual bool knob_left() {
            return false;
        }

        virtual bool knob_right() {
            return false;
        }

        virtual bool allow_takeover() {
            return false;
        }

        // whether we should be allowed to hover over this one
        virtual bool is_selectable () {
            return this->selectable;
        }

};

#include "menuitems_numbers.h"
#include "menuitems_selector.h"
#include "menuitems_pinned.h"

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


class ActionItem : public MenuItem {
    public:

    char button_label[20] = "";

    void(*on_open)() = nullptr;

    ActionItem(const char *label, void (*on_open)()) : MenuItem(label) {
        this->on_open = on_open;
        snprintf(button_label, 20, ">> %s <<", label);
    }

    virtual int display(Coord pos, bool selected, bool opened) override {
        pos.y = header(button_label, pos, selected, opened);
        tft->setCursor(pos.x,pos.y);
        tft->setTextSize(1);

        colours(opened, opened ? GREEN : this->default_fg, this->default_bg);

        return tft->getCursorY();
    }

    virtual bool action_opened() override {
        Debug_println(F("ActionItem#action_opened"));
        this->on_open();

        char msg[MENU_MESSAGE_MAX];
        //Serial.printf("about to build msg string...\n");
        snprintf(msg, MENU_MESSAGE_MAX, "Fired %8s", label);
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
        const char *text_to_render = opened ? "??? Sure ???" : button_label;

        int textSize = ((int)strlen(text_to_render)*tft->characterWidth() < tft->width()/2 );
        pos.y = header(text_to_render, pos, selected, opened, textSize);

        //tft->setCursor(pos.x,pos.y);
        //tft->setTextSize(textSize);
        colours(opened, opened ? GREEN : this->default_fg, this->default_bg);

        return tft->getCursorY();
    }

    virtual bool action_opened() override {
        Debug_println(F("ActionConfirmItem#action_opened"));
        //this->on_open();
        return true; 
    }

    virtual bool button_select() override {
        Debug_println(F("ActionConfirmItem#button_select"));

        this->on_open();

        char msg[MENU_MESSAGE_MAX];
        //Serial.printf("about to build msg string...\n");
        snprintf(msg, MENU_MESSAGE_MAX, "Fired %8s", label);
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

        virtual int display(Coord pos, bool selected, bool opened) override {
            tft->drawLine(pos.x, pos.y, tft->width(), pos.y, this->default_fg);
            pos.y += 2;

            pos.y = header(label, pos, selected, opened);

            return pos.y;
        }

        virtual int header(const char *text, Coord pos, bool selected = false, bool opened = false) {
            if (!this->show_header) return pos.y;

            /*tft->drawLine(pos.x, pos.y, tft->width(), pos.y, this->default_fg);
            tft->setCursor(pos.x, pos.y+1);
            //colours(!selected, this->default_fg, this->default_bg);
            colours(false, this->default_fg, this->default_bg);
            tft->setTextSize(0);
            tft->printf((char*)tft->get_header_format(), (char*)text);
            int start_x = tft->characterWidth() * strlen(text);
            //tft->drawLine(start_x, pos.y, tft->width(), pos.y, this->default_fg);
            tft->drawLine(start_x, pos.y+3, tft->width(), pos.y+2, this->default_fg);
            tft->drawLine(start_x, pos.y+6, tft->width(), pos.y+4, this->default_fg);
            tft->drawLine(start_x, pos.y+8, tft->width(), pos.y+6, this->default_fg);*/
            tft->setTextSize(0);

            colours(false, this->default_fg, this->default_bg);
            tft->drawLine(pos.x, pos.y, tft->width(), pos.y, this->default_fg);
            tft->setCursor(pos.x, pos.y+1);
            int end_x = (tft->width() - (tft->characterWidth() * strlen(text))) - 2;
            tft->drawLine(0, pos.y+2, end_x, pos.y+2, this->default_fg);
            tft->drawLine(0, pos.y+4, end_x, pos.y+4, this->default_fg);
            tft->drawLine(0, pos.y+6, end_x, pos.y+6, this->default_fg);
            tft->drawLine(0, pos.y+8, end_x, pos.y+8, this->default_fg);
            tft->setCursor(end_x+2, pos.y+1);
            tft->printf((char*)tft->get_header_format(), (char*)text);

            colours(false);
            //return (tft->getTextSize()+1)*6;
            return tft->getCursorY();
        }
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
            pos.y = header(label, pos, selected, opened);
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
            const char *txt = *this->target_variable ? "On" : "Off";
            bool use_small = strlen(txt) <= (max_character_width/2);
            int textSize = use_small ? 2 : 1;
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