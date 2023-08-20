#ifndef MENUITEMS_TOGGLE__INCLUDED
#define MENUITEMS_TOGGLE__INCLUDED

#include "menuitems.h"

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
            //int textSize = tft->get_textsize_for_width(label, tft->width());
            int textSize = 0;
            pos.y = header(label, pos, selected, opened, textSize);
            tft->setCursor(pos.x,pos.y);

            colours(opened, opened ? GREEN : this->default_fg, BLACK);
            //tft->setTextSize(2);        // was 2 ?
            //char tmp[MENU_C_MAX] = "";
            //tft->setTextSize(2);

            this->renderValue(selected, opened, MENU_C_MAX);

            return tft->getCursorY();
        }

        // render the current value at current position
        virtual int renderValue(bool selected, bool opened, uint16_t max_character_width) override {
            const char *txt = *this->target_variable ? label_on : label_off;
            //bool use_small = strlen(txt) <= (max_character_width/2);
            //int textSize = use_small ? 2 : 1;
            int textSize = tft->get_textsize_for_width(txt, max_character_width*tft->characterWidth());
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

#endif