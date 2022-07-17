#ifndef MENUITEMS__INCLUDED
#define MENUITEMS__INCLUDED

#include "menu.h"
#include "colours.h"


void menu_set_last_message(char *msg, int colour);

// basic line
class MenuItem {
    public:
        DisplayTranslator *tft;

        char label[20];

        bool debug = false;

        MenuItem set_tft(DisplayTranslator *tft) {
            this->tft = tft;
            return *this;
        }

        MenuItem(const char *in_label) {
            strcpy(label, in_label);
        }
        virtual void on_add() {
            if (this->debug) Serial.printf("MenuItem#on_add in %s\n", this->label);
        }    // called when object is added to menu
        
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
            tft->printf("%s [s:%i o:%i]", label, (int)selected, (int)opened);
            //return (tft->getTextSizeY() * 8) + 2;
            return tft->getCursorY();
        }

        void colours(bool selected) {
            colours(selected, /*tft->WHITE*/C_WHITE, BLACK);
        }
        void colours(bool selected, int fg) {
            colours(selected, fg, BLACK);
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
            tft->drawLine(pos.x, pos.y, tft->width(), pos.y, C_WHITE);
            tft->setCursor(pos.x, pos.y+1);
            colours(selected);
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
};


// generic control for selecting a number
class NumberControl : public MenuItem {
    public:
        bool debug = false;
        void (*on_change_handler)(int last_value, int new_value) = nullptr;

        int (*getter)() = nullptr;
        void (*setter)(int value) = nullptr;

        int *target_variable = nullptr;
        int internal_value = 0;
        int minimum_value = 0;
        int maximum_value = 100;
        int step = 1;
        bool readOnly = false;
        bool go_back_on_select = false;

        NumberControl(const char* label) : MenuItem(label) {
        }
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
        NumberControl(const char* label, int (*getter)(), void (*setter)(int value), int min_value, int max_value, void (*on_change_handler)(int last_value, int new_value)) : 
            MenuItem(label) {
            this->getter = getter;
            this->setter = setter;

            internal_value = getter();
            minimum_value = min_value;
            maximum_value = max_value;
            this->on_change_handler = on_change_handler;
        };

        /*virtual void on_add() override {
            internal_value = getter();
        }*/

        virtual void setReadOnly(bool readOnly = true) {
            this->readOnly = readOnly;
        }

        virtual const char *getFormattedValue(int value) {
            static char fmt[20] = "      ";
            sprintf(fmt, "%i", value);
            return fmt;
        }

        virtual const char *getFormattedValue() {
            return this->getFormattedValue(get_current_value());
        }

        virtual int get_internal_value() {
            return this->internal_value;
        }

        virtual int display(Coord pos, bool selected, bool opened) override {
            if (this->debug) {
                Serial.println("NumberControl#display starting!"); Serial.flush();
                Serial.printf("NumberControl#display in %s starting\n", this->label); Serial.flush();
            }
            pos.y = header(label, pos, selected, opened);
            if (this->debug) { Serial.println("did header"); Serial.flush(); }
            tft->setCursor(pos.x,pos.y);
            if (this->debug) { Serial.println("did setcursor"); Serial.flush(); }

            if (this->debug) { Serial.printf("NumberControl#display in %s about to do colours...\n", this->label); Serial.flush(); }
            colours(opened, opened ? GREEN : C_WHITE, BLACK);
            if (this->debug) { Serial.println("did colours"); Serial.flush(); }
            //tft->setTextSize(2);        // was 2 ?
            char tmp[20] = "                   ";
            if (this->debug) { Serial.println("did setting tmp"); Serial.flush(); }
            
            if (this->debug) { Serial.printf("NumberControl#display in %s about to do getFormattedValue() ting...\n", this->label); Serial.flush(); }
            if (opened) {
                //tft->printf("value: %*i\n", 4, internal_value);
                sprintf(tmp, "%s\n", this->getFormattedValue(this->get_internal_value()));
                //Serial.printf("in opened NumberControl for %s, with internal_value %i, got formattedvalue '%s'\n", this->label, internal_value, this->getFormattedValue(internal_value));
            } else {
                //tft->printf("value: %*i\n", 4, get_current_value()); //*target_variable); //target->transpose);
                sprintf(tmp, "%s\n", this->getFormattedValue()); //get_current_value());
            }
            if (this->debug) { Serial.printf("NumberControl#display in %s just did getFormattedValue() ting!\n", this->label); Serial.flush(); }

            // adjust size dependent on size of formatted value
            if (strlen(tmp)<10) 
                tft->setTextSize(2);
            else
                tft->setTextSize(1);

            tft->printf(tmp);
            if (this->debug) { Serial.printf("NumberControl base display finished in %s\n", label); }

            return tft->getCursorY();
        }

        virtual void set_internal_value(int value) {
            if (this->debug)  { Serial.printf("NumberControl.set_internal_value(%i)..\n", value); }
            this->internal_value = value;
        }

        virtual void decrease_value() {
            this->set_internal_value(get_internal_value() - step);
            if (get_internal_value() < this->minimum_value)
                set_internal_value(this->minimum_value); // = NUM_LOOP_SLOTS_PER_PROJECT-1;
            //project.select_loop_number(ui_selected_loop_number);
        }
        virtual void increase_value() {
            //internal_value += step;
            this->set_internal_value(get_internal_value() + step);
            if (get_internal_value() >= this->maximum_value)
                set_internal_value(this->maximum_value);
        }

        virtual bool knob_left() {
            if (!readOnly)
                decrease_value();
            return true;
        }
        virtual bool knob_right() {
            if (!readOnly)
                increase_value();
            //project.select_loop_number(internal_value);
            return true;
        }

        virtual void change_value(int new_value) {
            if (readOnly) return;
            int last_value = get_current_value();
            set_current_value(new_value);
            if (on_change_handler!=nullptr) {
                if (this->debug)  { Serial.println("NumberControl calling on_change_handler"); Serial.flush(); }
                on_change_handler(last_value, this->get_internal_value());
                if (this->debug)  { Serial.println("NumberControl after on_change_handler"); Serial.flush(); }
            }
        }

        virtual bool button_select() {
            if (readOnly) return true;
            //this->target->set_transpose(internal_value);           
            change_value(this->get_internal_value());

            return (go_back_on_select);
        }

        // override in subclass if need to do something special eg getter/setter
        virtual int get_current_value() {
            if (this->debug)  { Serial.printf("About to get_current_value in %s\n", this->label); Serial.flush(); }
            if (target_variable!=nullptr)
                return *target_variable;
            if (getter!=nullptr)
                return getter();
            if (this->debug) { Serial.printf("Did get_current_value in %s\n", this->label); Serial.flush(); }

            return 0;
        }

        // override in subclass if need to do something special eg getter/setter
        virtual void set_current_value(int value) { 
            if (target_variable!=nullptr)
                *target_variable = value;
            if (setter!=nullptr)
                setter(value);
        }

        virtual void setStep(int step) {
            this->step = step;
        }
};

template<class TargetClass, class DataType>
class ObjectNumberControl : public NumberControl {
    public:
    bool debug = false;
    DataType internal_value;

    void(TargetClass::*setter)(DataType) = nullptr;
    DataType(TargetClass::*getter)() = nullptr;
    void (*on_change_handler)(DataType last_value, DataType new_value) = nullptr;
    TargetClass *target_object = nullptr;

    ObjectNumberControl(const char* label, 
                        TargetClass *target_object, 
                        void(TargetClass::*setter_func)(DataType), 
                        DataType(TargetClass::*getter_func)(), 
                        void (*on_change_handler)(DataType last_value, DataType new_value) = nullptr
                ) : NumberControl(label) {
        this->target_object = target_object;
        this->getter = getter_func;
        this->setter = setter_func;
        this->on_change_handler = on_change_handler;

        if (this->target_object!=nullptr && this->getter!=nullptr) 
            this->set_internal_value( (this->target_object->*getter)() );
    }
    ObjectNumberControl(const char* label, 
                        TargetClass *target_object, 
                        void(TargetClass::*setter_func)(DataType), 
                        DataType(TargetClass::*getter_func)(), 
                        void (*on_change_handler)(DataType last_value, DataType new_value),
                        DataType minimum_value,
                        DataType maximum_value
                ) : ObjectNumberControl(label, target_object, setter_func, getter_func, on_change_handler) {
        this->minimum_value = minimum_value;
        this->maximum_value = maximum_value;
    }

    virtual void on_add() override {
        NumberControl::on_add();
        if (this->target_object!=nullptr && this->getter!=nullptr) 
            this->set_internal_value( (this->target_object->*getter)() );
    }

    /*virtual void decrease_value() override {
        this->set_internal_value(get_internal_value() - step);
        if (get_internal_value() < minimum_value)
            set_internal_value(minimum_value); // = NUM_LOOP_SLOTS_PER_PROJECT-1;
        //project.select_loop_number(ui_selected_loop_number);
    }
    virtual void increase_value() override {
        //internal_value += step;
        this->set_internal_value(get_internal_value() + step);
        if (get_internal_value() >= maximum_value)
            set_internal_value(maximum_value);
    }*/

    virtual int display(Coord pos, bool selected, bool opened) override {
        Serial.printf("display in ObjectNumberControl %s\n", this->label); Serial.flush();
        //Serial.printf("nopped display for ObjectNumberControl %s\n", this->label); Serial.flush();
        return NumberControl::display(pos, selected, opened);
        return pos.y;
    }

    virtual DataType get_internal_value() override {
        return this->internal_value;
    }

    virtual void set_internal_value(DataType value) {
        if (this->debug) { Serial.printf("ObjectNumberControl.set_internal_value(%i)..\n", value); }
        this->internal_value = value;
    }

    virtual void change_value(int new_value) override {
        if (readOnly) return;
        if (this->debug) { Serial.printf("ObjectNumberControl#change_value(%i)..\n", new_value); Serial.flush(); }
        DataType last_value = get_current_value();
        this->set_current_value(new_value);
        if (on_change_handler!=nullptr) {
            if (this->debug)  { Serial.println("ObjectNumberControl calling on_change_handler"); Serial.flush(); }
            on_change_handler(last_value, internal_value);
        }
    }

    /*virtual bool button_select() override {
        if (readOnly) return;
        //this->target->set_transpose(internal_value);           
        change_value(this->get_internal_value());
        return false;
    }*/

    // override in subclass if need to do something special eg getter/setter
    virtual DataType get_current_value() override {
        if (this->target_object!=nullptr && this->getter!=nullptr) {
            if (this->debug) { Serial.printf("ObjectNumberControl#get_current_value in %s about to call getter\n", this->label); Serial.flush(); }
            DataType v = (this->target_object->*getter)();
            if (this->debug) { Serial.println("Called getter!"); Serial.flush(); }
            return v;
        }
        
        return 0;
    }

    // override in subclass if need to do something special eg getter/setter
    virtual void set_current_value(DataType value) override { 
        //this->internal_value = value;
        if (this->debug) { Serial.printf("ObjectNumberControl#set_current_value(%i)\n", value); Serial.flush(); }
        if (this->target_object!=nullptr && this->setter!=nullptr) {
            (this->target_object->*setter)(value);

            char msg[255];
            //Serial.printf("about to build msg string...\n");
            sprintf(msg, "Set %8s to %i", label, value);
            //Serial.printf("about to set_last_message!");
            msg[tft->get_c_max()] = '\0'; // limit the string so we don't overflow set_last_message
            menu_set_last_message(msg,GREEN);
        }
        if (this->debug) { Serial.println("Done."); Serial.flush(); }
    }
};


class DirectNumberControl : public NumberControl {
    public:
    DirectNumberControl(const char* label) : NumberControl(label) {};
    DirectNumberControl(const char* label, int *in_target_variable, int start_value, int min_value, int max_value, void (*on_change_handler)(int last_value, int new_value)) 
            : NumberControl(label, in_target_variable, start_value, min_value, max_value, on_change_handler) {
    }
    DirectNumberControl(const char* label, int (*getter)(), void (*setter)(int value), int min_value, int max_value, void (*on_change_handler)(int last_value, int new_value))
            : NumberControl(label, getter, setter, min_value, max_value, on_change_handler) {
    }

    virtual bool knob_left() override {
        if (readOnly) return false;
        Serial.printf("DirectNumberControl knob_left, internal_value=%i\n", internal_value);
        decrease_value();
        change_value(internal_value);
        //project.select_loop_number(ui_selected_loop_number);
        return true;
    }
    virtual bool knob_right() override {
        if (readOnly) return false;
        Serial.printf("DirectNumberControl knob_right, internal_value=%i\n", internal_value);
        increase_value();
        change_value(internal_value);
        //project.select_loop_number(internal_value);
        return true;
    }
    virtual bool button_select() override {
        if (readOnly) return true;
        //this->target->set_transpose(internal_value);           
        change_value(internal_value);
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
                //bool is_current_value_selected = selected_value_index==i; //available_values[i]==currentValue;
                bool is_current_value_selected = available_values[i]==current_value; //getter();
                int col = is_current_value_selected ? GREEN : C_WHITE;
                colours(opened && selected_value_index==i, col, BLACK);
                //colours(true, col, ST7735_BLUE);
                //Serial.printf("for item %i/%i, printing %s\n", i, num_values, get_label_for_value(available_values[i]));
                tft->printf("%s", (char*)get_label_for_value(available_values[i])); //available_values[i]);
                //tft->printf("%i", available_values[i]);
                tft->setTextColor(BLACK,BLACK);
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

            int currentValue = this->getter();

            for (int i = start_item ; i < selected_value_index ; i++) {
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
            if (selected_value_index >= num_values)
                selected_value_index = 0;
            return true;
        }

        virtual bool button_select() override {
            //Serial.printf("button_select with selected_value_index %i\n", selected_value_index);
            //Serial.printf("that is available_values[%i] of %i\n", selected_value_index, available_values[selected_value_index]);
            this->setter(available_values[selected_value_index]);

            char msg[255];
            //Serial.printf("about to build msg string...\n");
            sprintf(msg, "Set %s to %s (%i)", label, get_label_for_value(available_values[selected_value_index]), available_values[selected_value_index]);
            //Serial.printf("about to set_last_message!");
            msg[tft->get_c_max()] = '\0'; // limit the string so we don't overflow set_last_message
            menu_set_last_message(msg,GREEN);
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
                tft->println((char *)"[not set]");
            } else {
                tft->printf("%4s : %4s\n",     // \n not needed on smaller screen because already fills row.. is needed on big tft?
                    (char*)(get_note_name(*last_note).c_str()), 
                    (char*)(get_note_name(*current_note).c_str())
                );
            }
            return tft->getCursorY();
        }
};


class PinnedPanelMenuItem : public MenuItem {
    public:
        unsigned long ticks = 0;

        PinnedPanelMenuItem(const char *label) : MenuItem(label) {};

        void update_ticks(unsigned long ticks) {
            this->ticks = ticks;
        }
};

class LoopMarkerPanel : public PinnedPanelMenuItem {
    unsigned long loop_length;
    int beats_per_bar = 4;
    int bars_per_phrase = 4;
    int ppqn;

    public:
        LoopMarkerPanel(int loop_length, int ppqn, int beats_per_bar = 4, int bars_per_phrase = 4) : PinnedPanelMenuItem("Loop Position Header") {
            this->loop_length = loop_length;
            this->beats_per_bar = beats_per_bar;
            this->bars_per_phrase = bars_per_phrase;
            this->ppqn = ppqn;
        };

        void set_loop_length(unsigned long loop_length) {
            this->loop_length = loop_length;
        }
        void set_beats_per_bar(unsigned long beats_per_bar) {
            this->beats_per_bar = beats_per_bar;
        }        

        virtual int display(Coord pos, bool selected = false, bool opened = false) override {
            //Serial.printf("PinnedPanel display colour RED is %4x, WHITE is %4x\n", RED, C_WHITE);

            tft->setTextColor(C_WHITE, BLACK);
            //tft.setCursor(pos.x,pos.y);
            //int LOOP_LENGTH = PPQN * BEATS_PER_BAR * BARS_PER_PHRASE;
            int y = 0;
            //y+=2;
            static unsigned long last_serviced_tick;
            static int last_position_width;

            // save some float maths by only recalculating if tick is different from last time
            if (last_serviced_tick != ticks) {
                float percent = float(ticks % loop_length) / (float)loop_length;
                last_position_width = (percent*(float)tft->width());
            }
            tft->fillRect(0, y, last_position_width, 6, RED);

            //float percent = float(ticks % loop_length) / (float)loop_length;
            //tft->fillRect(0, y, (percent*(float)tft->width()), 6, RED);

            /*static float px_per_pos = tft->width() / loop_length;
            int loop_position = ticks % loop_length;
            tft->fillRect(0, y, px_per_pos * loop_position, 6, RED);*/

            static int tft_width = tft->width();

            static int step_size_beats = tft_width / (beats_per_bar*bars_per_phrase);  // safe to make static so long as beats_per_bar/bars_per_phrase is not configurable!
            for (int i = 0 ; i < tft_width ; i += step_size_beats) {
                tft->drawLine(i, y, i, y+2, C_WHITE);
                //if (i%BEATS_PER_BAR==0)
                    //tft.drawLine(i, y, i, y+4, ST7735_CYAN);
            }

            static int step_size_bars = tft_width / bars_per_phrase;
            for (int i = 0 ; i < tft_width ; i += step_size_bars) {
                //tft.drawLine(i, y, i, y+4, ST7735_WHITE);
                tft->fillRect(i, y+1, 2, 5, C_WHITE);
            }

            //Serial.printf("percent %f, width %i\n", percent, tft->width());
            y += 6;
            return y;
        }
        //#endif
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
        //tft->setTextSize(2);        // was 2 ?
        //char tmp[20] = "";
        
        /*if (opened) {
            //tft->printf("value: %*i\n", 4, internal_value);
            sprintf(tmp, "%s\n", this->getFormattedValue(internal_value));
            Serial.printf("in opened NumberControl for %s, with internal_value %i, got formattedvalue '%s'\n", this->label, internal_value, this->getFormattedValue(internal_value));
        } else {
            //tft->printf("value: %*i\n", 4, get_current_value()); // *target_variable); //target->transpose);
            sprintf(tmp, "%s\n", this->getFormattedValue()); //get_current_value());
        }*/

        // adjust size dependent on size of formatted value
        /*if (strlen(tmp)<10) 
            tft->setTextSize(2);
        else
            tft->setTextSize(1);

        tft->printf(tmp);*/
        //Serial.printf("NumberControl base display in %s?\n", label);

        return tft->getCursorY();
    }

    virtual bool action_opened() override {
        Serial.println("ActionItem#action_opened");
        this->on_open();

        char msg[255];
        //Serial.printf("about to build msg string...\n");
        sprintf(msg, "Fired %8s", label);
        //Serial.printf("about to set_last_message!");
        msg[tft->get_c_max()] = '\0'; // limit the string so we don't overflow set_last_message
        menu_set_last_message(msg,GREEN);

        /*if (parameter->getCurrentValue()<0.5) {
            Serial.println("sending value_on");
            parameter->setParamValue(value_on);
        } else {
            Serial.println("sending value_off");
            parameter->setParamValue(value_off);
        }
        //this->parameter->setParamValue(((DataParameter*)parameter)->getCurrentValue() < 0.5);*/
        return false;   // don't 'open'
    }

};

template<class TargetClass>
class ObjectToggleControl : public MenuItem {
    public:
        TargetClass *target_object;

        void(TargetClass::*setter)(bool) = nullptr;
        bool(TargetClass::*getter)() = nullptr;
        void (*on_change_handler)(int last_value, int new_value) = nullptr;

        ObjectToggleControl(const char *label, TargetClass *target_object, void(TargetClass::*setter)(bool), bool(TargetClass::*getter)(), void (*on_change_handler)(int last_value, int new_value))
            : MenuItem(label) {
            this->target_object = target_object;
            this->setter = setter;
            this->getter = getter;
            this->on_change_handler = on_change_handler;
        }

        virtual int display(Coord pos, bool selected, bool opened) override {
            pos.y = header(label, pos, selected, opened);
            tft->setCursor(pos.x,pos.y);

            colours(opened, opened ? GREEN : C_WHITE, BLACK);
            //tft->setTextSize(2);        // was 2 ?
            //char tmp[20] = "";
            tft->setTextSize(2);

            if ((this->target_object->*getter)()) {
                tft->println("On");
            } else {
                tft->println("Off");
            }
            
            /*if (opened) {
                //tft->printf("value: %*i\n", 4, internal_value);
                sprintf(tmp, "%s\n", this->getFormattedValue(this->get_internal_value()));
                //Serial.printf("in opened NumberControl for %s, with internal_value %i, got formattedvalue '%s'\n", this->label, internal_value, this->getFormattedValue(internal_value));
            } else {
                //tft->printf("value: %*i\n", 4, get_current_value()); // *target_variable); //target->transpose);
                sprintf(tmp, "%s\n", this->getFormattedValue()); //get_current_value());
            }

            // adjust size dependent on size of formatted value
            if (strlen(tmp)<10) 
                tft->setTextSize(2);
            else
                tft->setTextSize(1);*/

            //tft->printf(tmp);
            //Serial.printf("NumberControl base display in %s?\n", label);

            return tft->getCursorY();
        }

        virtual bool action_opened() override {
            if (this->debug) Serial.printf("ObjectToggleControl#action_opened on %s\n", this->label);
            bool value = !(this->target_object->*getter)();
            //this->internal_value = !this->internal_value;

            (this->target_object->*setter)(value); //(bool)this->internal_value);
            return false;   // don't 'open'
        }

};

#endif