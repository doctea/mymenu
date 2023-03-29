#ifndef MENUITEM_SELECTOR__INCLUDED
#define MENUITEM_SELECTOR__INCLUDED

#include "menuitems.h"

// generic control for selecting one option from a selection of values
// TODO: keep currently selected option centred in display and scroll through the rest
template<class DataType = int>
class SelectorControl : public MenuItem {
    public:
        unsigned int num_values;
        int selected_value_index = 0;
        DataType *available_values;
        int actual_value_index = -1;

        void (*f_setter)(DataType) = nullptr;
        DataType (*f_getter)() = nullptr;

        virtual void setter (DataType new_value) {
            if (f_setter!=nullptr)
                this->f_setter(new_value);
        }
        virtual int getter () {
            if (f_getter!=nullptr)
                return this->f_getter();
            return available_values[this->selected_value_index];
        }
        /*int on_change() {
            Serial.printf("SelectorControl %s changed to %i!\n", label, available_values[selected_value_index]);
        }*/
        virtual const char *get_label_for_value(DataType value) {
            //static char value_label[MENU_C_MAX];
            //snprintf(value_label, MENU_C_MAX, "%i", value);
            //return value_label;
            return getFormattedValue(value);
        }

        SelectorControl(const char *label) : MenuItem(label) {};
        SelectorControl(const char *label, byte initial_selected_value_index) : 
            SelectorControl(label) {
            this->selected_value_index = initial_selected_value_index;
        }

        // used when needs to be told to update externally -- eg when ParameterInput mapping changes and need to tell its ParameterInputSelector
        virtual void update_actual_index(int new_index) {
            this->actual_value_index = new_index;
        }

        // classic fixed display version
        virtual int display(Coord pos, bool selected, bool opened) override {
            pos.y = header(label, pos, selected, opened);
            tft->setTextSize(2);

            DataType current_value = this->getter(); //f_getter!=nullptr ? this->f_getter() : available_values[this->selected_value_index];

            for (unsigned int i = 0 ; i < num_values ; i++) {
                //bool is_current_value_selected = selected_value_index==i; //available_values[i]==currentValue;
                bool is_current_value_selected = available_values[i]==current_value; //getter();
                int col = is_current_value_selected ? GREEN : C_WHITE;
                colours(opened && selected_value_index==(int)i, col, BLACK);

                tft->printf("%s", (char*)get_label_for_value(available_values[i]));

                //this->renderValue(selected, opened, MENU_C_MAX);

                tft->setTextColor(BLACK,BLACK);
                if (i<num_values-1) 
                    tft->printf(" ");
            }
            if (tft->getCursorX()>0) // if we haven't wrapped onto next line then do it manually
                tft->println("");
            return tft->getCursorY();
        }

        char fmt[20];
        virtual const char *getFormattedValue(int v) {
            snprintf(fmt, 20, "%i", v);
            return fmt;
        }
        virtual const char *getFormattedValue(uint32_t v) {
            snprintf(fmt, 20, "%u", v);
            return fmt;
        }
        virtual const char *getFormattedValue(float v) {
            if (abs(v-(int)v)>0.01)
                snprintf(fmt, 20, "%3.2f", v);
            else        // don't print .00 fractional part if there isn't one
                snprintf(fmt, 20, "%3i", (int)v);
                
            return fmt;
        }

        virtual int renderValue(bool selected, bool opened, uint16_t max_character_width) override {
            char label[MAX_LABEL_LENGTH];
            /*const char *src_label = opened ?
                this->get_label_for_value(available_values[selected_value_index])
                :
                this->get_label_for_value(this->getter());*/
            const char *src_label = this->get_label_for_value(
                opened ? available_values[selected_value_index] : this->getter()
            );

            //strcpy(label, get_label_for_value(available_values[opened ? selected_value_index : this->getter()]));
            if (strlen(label) > max_character_width) {
                label[max_character_width] = '\0';
            }
            snprintf(label, max_character_width, "%s", src_label);

            tft->printf("%s", (char*)label);
            return tft->getCursorY();
        }

        // cool, non-working rotating-option version
        /*virtual int display(Coord pos, bool selected, bool opened) override {
            pos.y = header(label, pos, selected, opened);
            tft->setTextSize(2);

            int start_item = selected_value_index - 2;

            int currentValue = this->getter();

            for (unsigned int i = start_item ; i < selected_value_index ; i++) {
                int actual_item = i;
                if (i<0) actual_item = num_values - i;
                if (i>=num_values) actual_item = selected_value_index + (num_values - i);

                bool is_current_value_selected = available_values[actual_item]==currentValue; //getter();
                int col = is_current_value_selected ? ST7735_GREEN : ST7735_WHITE;
                colours(opened && selected_value_index==actual_item, col, ST7735_BLACK);
                tft->printf("%s", get_label_for_value(available_values[actual_item]));
                tft->setTextColor(ST77XX_BLACK);
                if (actual_item<num_values-1) 
                    tft->printf(" ");
                if (tft->getCursorX()>=tft->width()) 
                    break;
            }

            if (tft->getCursorX()>0) // if we haven't wrapped onto next line then do it manually
                tft->println();
            return tft->getCursorY();
        }*/

        virtual bool knob_left() override {
            selected_value_index--;
            if (selected_value_index < 0)
                selected_value_index = num_values-1;
            return true;
        }

        virtual bool knob_right() override {
            selected_value_index++;
            if (selected_value_index >= (int)num_values)
                selected_value_index = 0;
            return true;
        }

        virtual bool button_select() override {
            //Serial.printf("button_select with selected_value_index %i\n", selected_value_index);
            //Serial.printf("that is available_values[%i] of %i\n", selected_value_index, available_values[selected_value_index]);
            if (this->f_setter!=nullptr)
                this->f_setter(available_values[selected_value_index]);

            char msg[MENU_MESSAGE_MAX];
            //Serial.printf("about to build msg string...\n");
            snprintf(msg, MENU_MESSAGE_MAX, "Set %s to %s (%i)", 
                label, 
                get_label_for_value(available_values[selected_value_index]), 
                //getFormattedValue(available_values[selected_value_index])
                selected_value_index
            );
            //Serial.printf("about to set_last_message!");
            //msg[tft->get_c_max()] = '\0'; // limit the string so we don't overflow set_last_message
            menu_set_last_message(msg, GREEN);

            return go_back_on_select;
        }

};



#endif