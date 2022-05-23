#ifndef MENUITEMS__INCLUDED
#define MENUITEMS__INCLUDED

#include "menu.h"

// basic line
class MenuItem {
    public:
        DisplayTranslator *tft;

        char label[20];

        MenuItem(const char *in_label) {
            strcpy(label, in_label);
        }
        virtual int display(Coord pos, bool selected, bool opened) {
            //Serial.printf("base display for %s\n", label);
            // display this item however that may be
            tft->setCursor(pos.x,pos.y);
            //tft->setTextColor(ST77XX_WHITE, ST77XX_BLACK);
            colours(selected);
            tft->print(label);
            //return (tft->getTextSizeY() * 8) + 2;
            return tft->getCursorY();
        }

        void colours(bool selected) {
            colours(selected, tft->WHITE, tft->BLACK);
        }
        void colours(bool selected, int fg) {
            colours(selected, fg, tft->BLACK);
        }

        void colours(bool selected, int fg, int bg) {
            if (!selected) {
                tft->setTextColor(fg, bg);
            } else {
                //Serial.printf("%s selected, setting colours %02x, %02x\n", label, bg, fg);
                tft->setTextColor(bg, fg) ;//ST77XX_BLACK, ST77XX_WHITE);
            }
        }
        
        int header(const char *text, Coord pos, bool selected = false, bool opened = false) {
            tft->drawLine(pos.x, pos.y, tft->width(), pos.y, tft->WHITE);
            tft->setCursor(pos.x, pos.y+1);
            colours(selected);
            tft->setTextSize(0);
            if (opened) {
                tft->print(">>>");
                tft->printf((const char*)"%-19s",text);   // \n not needed as reaching to edge
            } else {
                tft->printf((const char*)"%-22s",text);   // \n not needed as reaching to edge
            }
            //return (tft->getTextSize()+1)*6;
            return tft->getCursorY();
        }

        // default to returning true to exit out to main menu after setting
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
};


// generic control for selecting a number
class NumberControl : public MenuItem {
    void (*on_change_handler)(int last_value, int new_value);

    public:
        int *target_variable = nullptr;
        int internal_value = 0;
        int minimum_value = 0;
        int maximum_value = 4;
        int step = 1;


        NumberControl(const char* label, int in_start_value, int min_value, int max_value) : MenuItem(label) {
            internal_value = in_start_value;
            minimum_value = min_value;
            maximum_value = max_value;
        };
        NumberControl(const char* label, int *in_target_variable, int start_value, int min_value, int max_value, void (*on_change_handler)(int last_value, int new_value)) 
            : MenuItem(label) {
            //NumberControl(label, start_value, min_value, max_value) {
            internal_value = start_value;
            minimum_value = min_value;
            maximum_value = max_value; 
            target_variable = in_target_variable;
            this->on_change_handler = on_change_handler;
        };

        virtual int display(Coord pos, bool selected, bool opened) override {
            pos.y = header(label, pos, selected, opened);
            tft->setCursor(pos.x,pos.y);

            colours(opened, opened ? tft->GREEN : tft->WHITE, tft->BLACK);
            tft->setTextSize(2);
            if (opened) {
                tft->printf("%*i\n", 4, internal_value);
            } else {
                tft->printf("%*i\n", 4, get_current_value()); //*target_variable); //target->transpose);
            }

            return tft->getCursorY();
        }

        virtual void increase_value() {
            internal_value-=step;
            if (internal_value < minimum_value)
                internal_value = minimum_value; // = NUM_LOOPS_PER_PROJECT-1;
            //project.select_loop_number(ui_selected_loop_number);
        }
        virtual void decrease_value() {
            internal_value+=step;
            if (internal_value >= maximum_value)
                internal_value = maximum_value;
        }

        virtual bool knob_left() {
            increase_value();
            return true;
        }
        virtual bool knob_right() {
            decrease_value();
            //project.select_loop_number(internal_value);
            return true;
        }

        virtual void change_value(int new_value) {
            int last_value = get_current_value();
            set_current_value(new_value);
            if (on_change_handler!=nullptr) {
                Serial.println("calling on_change_handler");
                on_change_handler(last_value, internal_value);
            }
        }

        virtual bool button_select() {
            //this->target->set_transpose(internal_value);           
            change_value(internal_value);
            return false;
        }

        // override in subclass if need to do something special eg getter/setter
        virtual int get_current_value() {
            if (target_variable!=nullptr)
                //return 0;
                return *target_variable;
            else
                return 0;
        }

        // override in subclass if need to do something special eg getter/setter
        virtual void set_current_value(int value) { 
            if (target_variable!=nullptr)
                *target_variable = value;
        }

        virtual void setStep(int step) {
            this->step = step;
        }
};

class DirectNumberControl : public NumberControl {
    public:
    DirectNumberControl(const char* label, int *in_target_variable, int start_value, int min_value, int max_value, void (*on_change_handler)(int last_value, int new_value)) 
            : NumberControl(label, in_target_variable, start_value, min_value, max_value, on_change_handler) {
    }

    virtual bool knob_left() {
        Serial.printf("DirectNumberControl knob_left, internal_value=%i\n", internal_value);
        decrease_value();
        change_value(internal_value);
        //project.select_loop_number(ui_selected_loop_number);
        return true;
    }
    virtual bool knob_right() {
        Serial.printf("DirectNumberControl knob_right, internal_value=%i\n", internal_value);
        increase_value();
        change_value(internal_value);
        //project.select_loop_number(internal_value);
        return true;
    }
};

// generic control for selecting one option from a selection of values
// TODO: keep currently selected option centred in display and scroll through the rest
class SelectorControl : public MenuItem {
    public:
        int num_values;
        int selected_value_index;
        int *available_values;

        virtual void setter (int new_value) {
        }
        virtual int getter () {
            return 0;
        }
        /*int on_change() {
            Serial.printf("SelectorControl %s changed to %i!\n", label, available_values[selected_value_index]);
        }*/
        virtual const char*get_label_for_value(int value) {
            static char value_label[20];
            sprintf(value_label, "%i", value);
            return value_label;
        }

        SelectorControl(const char *label) : MenuItem(label) {};
        SelectorControl(const char *label, byte initial_selected_value_index) : 
            SelectorControl(label) {
            this->selected_value_index = initial_selected_value_index;
        }

        // classic fixed display version
        virtual int display(Coord pos, bool selected, bool opened) override {
            pos.y = header(label, pos, selected, opened);
            tft->setTextSize(2);

            int current_value = this->getter();

            for (int i = 0 ; i < num_values ; i++) {
                //bool is_current_value_selected = selected_value_index==i; //available_values[i]==current_value;
                bool is_current_value_selected = available_values[i]==current_value; //getter();
                int col = is_current_value_selected ? tft->GREEN : tft->WHITE;
                colours(opened && selected_value_index==i, col, tft->BLACK);
                //colours(true, col, ST7735_BLUE);
                //Serial.printf("for item %i/%i, printing %s\n", i, num_values, get_label_for_value(available_values[i]));
                tft->printf("%s", get_label_for_value(available_values[i])); //available_values[i]);
                //tft->printf("%i", available_values[i]);
                tft->setTextColor(tft->BLACK,tft->BLACK);
                if (i<num_values-1) 
                    tft->printf(" ");
            }
            if (tft->getCursorX()>0) // if we haven't wrapped onto next line then do it manually
                tft->println("");
            return tft->getCursorY();
        }

        // cool, non-working rotating-option version
        /*virtual int display(Coord pos, bool selected, bool opened) override {
            pos.y = header(label, pos, selected, opened);
            tft->setTextSize(2);

            int start_item = selected_value_index - 2;

            int current_value = this->getter();

            for (int i = start_item ; i < selected_value_index ; i++) {
                int actual_item = i;
                if (i<0) actual_item = num_values - i;
                if (i>=num_values) actual_item = selected_value_index + (num_values - i);

                bool is_current_value_selected = available_values[actual_item]==current_value; //getter();
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

        virtual bool knob_left() {
            selected_value_index--;
            if (selected_value_index < 0)
                selected_value_index = num_values-1;
            return true;
        }

        virtual bool knob_right() {
            selected_value_index++;
            if (selected_value_index >= num_values)
                selected_value_index = 0;
            return true;
        }

        virtual bool button_select() {
            //Serial.printf("button_select with selected_value_index %i\n", selected_value_index);
            //Serial.printf("that is available_values[%i] of %i\n", selected_value_index, available_values[selected_value_index]);
            this->setter(available_values[selected_value_index]);

            char msg[255];
            //Serial.printf("about to build msg string...\n");
            sprintf(msg, "Set %s to %s (%i)", label, get_label_for_value(available_values[selected_value_index]), available_values[selected_value_index]);
            //Serial.printf("about to set_last_message!");
            msg[20] = '\0'; // limit the string so we don't overflow set_last_message
            // TODO: reenable menu.set_last_message(msg);
            // TODO: reenable menu.set_message_colour(tft->GREEN);
            return false;
        }

};


String get_note_name(int pitch);
class HarmonyStatus : public MenuItem {
    int *last_note;
    int *current_note;

    public:
        HarmonyStatus() : MenuItem("Harmony") {};
        HarmonyStatus(const char *label, int *last_note, int *current_note) : MenuItem(label) {
            //MenuItem(label);
            this->last_note = last_note;
            this->current_note = current_note;
        }
        virtual int display(Coord pos, bool selected, bool opened) override {
            tft->setCursor(pos.x, pos.y);
            header(label, pos, selected, opened);
            //tft->setTextColor(rgb(0xFFFFFF),0);
            tft->setTextSize(2);
            colours(opened);

            if (!last_note || !current_note) {
                tft->println((const char *)"[not set]");
            } else {
                tft->printf("%4s : %4s",     // \n not needed because already fills row..
                    get_note_name(*last_note).c_str(), 
                    get_note_name(*current_note).c_str()
                );
            }
            return tft->getCursorY();
        }
};


#endif