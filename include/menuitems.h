#ifndef MENUITEMS__INCLUDED
#define MENUITEMS__INCLUDED

#include "Arduino.h"

#include "menu.h"
#include "colours.h"

#define MAX_LABEL_LENGTH 40

void menu_set_last_message(const char *msg, int colour);

// basic line
class MenuItem {
    public:
        DisplayTranslator *tft;
        int menu_c_max = MENU_C_MAX;

        char label[MAX_LABEL_LENGTH];

        uint16_t colour = 0xFFFF;

        bool show_header = true;

        bool debug = false;

        bool selectable = true;

        MenuItem set_tft(DisplayTranslator *tft) {
            this->tft = tft;
            return *this;
        }

        MenuItem(const char *in_label) {
            strcpy(label, in_label);
        }
        virtual void on_add() {
            if (this->debug) Serial.printf(F("MenuItem#on_add in %s\n"), this->label);
            menu_c_max = tft->get_c_max();
        }    // called when this menuitem is added to menu

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
            tft->printf("%s [s:%i o:%i]", label, (int)selected, (int)opened);
            return tft->getCursorY();
        }

        virtual void colours(bool inverted) {
            colours(inverted, /*tft->WHITE*/C_WHITE, BLACK);
        }
        virtual void colours(bool inverted, uint16_t fg) {
            colours(inverted, fg, BLACK);
        }
        virtual void colours(bool inverted, uint16_t fg, uint16_t bg) {
            if (!inverted) {
                tft->setTextColor(fg, bg);
            } else {
                //Serial.printf("%s selected, setting colours %02x, %02x\n", label, bg, fg);
                tft->setTextColor(bg, fg) ;//ST77XX_BLACK, ST77XX_WHITE);
            }
        }
        
        virtual int header(const char *text, Coord pos, bool selected = false, bool opened = false) {
            if (!this->show_header) return pos.y;

            tft->drawLine(pos.x, pos.y, tft->width(), pos.y, C_WHITE);
            tft->setCursor(pos.x, pos.y+1);
            colours(selected, C_WHITE, BLACK);
            tft->setTextSize(0);
            if (opened) {
                //tft->print(">>>");
                //tft->printf((char*)"%-19s",(char*)text);   // \n not needed as reaching to edge
                tft->printf((char*)tft->get_header_open_format(), (char*)text);
            } else if (selected) {
                //tft->printf((char*)"%-22s",(char*)text);   // \n not needed as reaching to edge
                tft->printf((char*)tft->get_header_selected_format(), (char*)text);
            } else {
                tft->printf((char*)tft->get_header_format(), (char*)text);
            }
            colours(false);
            //return (tft->getTextSize()+1)*6;
            return tft->getCursorY();
        }

        // called when item is selected ie opened from the main menu - return true to open, return false to 'refuse to open'
        virtual bool action_opened() {
            return true;
        }

        // default to returning true to exit out to main menu after setting (IF OPENED, otherwise button_select is not sent!)
        virtual bool button_select() {
            return true;
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
class HarmonyStatus : public MenuItem {
    int *last_note = nullptr;
    int *current_note = nullptr;
    int *other_value = nullptr;

    public:
        HarmonyStatus() : MenuItem("Harmony") {};
        HarmonyStatus(const char *label, int *last_note, int *current_note) : MenuItem(label) {
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
                    (char*)(get_note_name(*last_note).c_str()), 
                    (char*)(get_note_name(*current_note).c_str()),
                    (char*)(get_note_name(*other_value).c_str())
                );
            } else {
                tft->printf("%4s : %4s\n",     // \n not needed on smaller screen because already fills row.. is needed on big tft?
                    (char*)(get_note_name(*last_note).c_str()), 
                    (char*)(get_note_name(*current_note).c_str())
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
        sprintf(button_label,">> %s <<", label);
    }

    virtual int display(Coord pos, bool selected, bool opened) override {
        pos.y = header(button_label, pos, selected, opened);
        tft->setCursor(pos.x,pos.y);
        tft->setTextSize(1);

        colours(opened, opened ? GREEN : C_WHITE, BLACK);

        return tft->getCursorY();
    }

    virtual bool action_opened() override {
        Serial.println(F("ActionItem#action_opened"));
        this->on_open();

        char msg[255];
        //Serial.printf("about to build msg string...\n");
        sprintf(msg, "Fired %8s", label);
        //Serial.printf("about to set_last_message!");
        msg[tft->get_c_max()] = '\0'; // limit the string so we don't overflow set_last_message
        menu_set_last_message(msg,GREEN);

        return false;   // don't 'open'
    }

};


class ActionConfirmItem : public ActionItem {
    public:

    ActionConfirmItem(const char *label, void (*on_open)()) : ActionItem(label, on_open) {}

    virtual int display(Coord pos, bool selected, bool opened) override {
        if (opened)
            pos.y = header("??? Sure ???", pos, selected, opened);
        else
            pos.y = header(button_label, pos, selected, opened);
        tft->setCursor(pos.x,pos.y);
        tft->setTextSize(1);

        colours(opened, opened ? GREEN : C_WHITE, BLACK);

        return tft->getCursorY();
    }

    virtual bool action_opened() override {
        Serial.println(F("ActionConfirmItem#action_opened"));
        //this->on_open();
        return true; 
    }

    virtual bool button_select() override {
        Serial.println(F("ActionConfirmItem#button_select"));

        this->on_open();

        char msg[255];
        //Serial.printf("about to build msg string...\n");
        sprintf(msg, "Fired %8s", label);
        //Serial.printf("about to set_last_message!");
        msg[tft->get_c_max()] = '\0'; // limit the string so we don't overflow set_last_message
        menu_set_last_message(msg,GREEN);

        return true;    // return to menu
    }

};

class SeparatorMenuItem : public MenuItem {
    public:
        int16_t colour = C_WHITE;
        SeparatorMenuItem(char *label, int16_t colour = C_WHITE) : MenuItem(label) {
            this->selectable = false;
            this->colour = colour;
        }

        virtual int display(Coord pos, bool selected, bool opened) override {
            tft->drawLine(pos.x, pos.y, tft->width(), pos.y, colour);
            pos.y += 2;

            pos.y = header(label, pos, selected, opened);

            return pos.y;
        }

        virtual int header(const char *text, Coord pos, bool selected = false, bool opened = false) {
            if (!this->show_header) return pos.y;

            tft->drawLine(pos.x, pos.y, tft->width(), pos.y, this->colour);
            tft->setCursor(pos.x, pos.y+1);
            colours(!selected, this->colour, BLACK);
            tft->setTextSize(0);
            /*if (opened) {
                //tft->print(">>>");
                //tft->printf((char*)"%-19s",(char*)text);   // \n not needed as reaching to edge
                tft->printf((char*)tft->get_header_open_format(), (char*)text);
            } else if (selected) {
                //tft->printf((char*)"%-22s",(char*)text);   // \n not needed as reaching to edge
                tft->printf((char*)tft->get_header_selected_format(), (char*)text);
            } else {*/
                tft->printf((char*)tft->get_header_format(), (char*)text);
            //}
            colours(false);
            //return (tft->getTextSize()+1)*6;
            return tft->getCursorY();
        }
};

#include "menuitems_object.h"

#endif