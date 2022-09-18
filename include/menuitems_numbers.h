#ifndef MENUITEM_NUMBERS__INCLUDED
#define MENUITEM_NUMBERS__INCLUDED

#include "Arduino.h"

#include "menu.h"
#include "colours.h"

// generic control for selecting a number
template<class DataType = int>
class NumberControl : public MenuItem {
    public:
        bool debug = false;
        void (*on_change_handler)(DataType last_value, DataType new_value) = nullptr;

        DataType (*getter)() = nullptr;
        void (*setter)(DataType value) = nullptr;

        DataType *target_variable = nullptr;
        DataType internal_value = 0;
        DataType minimum_value = 0;
        DataType maximum_value = 100;
        DataType step = this->get_default_step_for_type(internal_value);
        bool readOnly = false;
        bool go_back_on_select = false;

        NumberControl(const char* label) : MenuItem(label) {
            this->step = this->get_default_step_for_type((DataType)0);    // setup default step
        }
        NumberControl(const char* label, DataType in_start_value, DataType min_value, DataType max_value) : NumberControl(label) {
            internal_value = in_start_value;
            minimum_value = min_value;
            maximum_value = max_value;
        };
        NumberControl(const char* label, DataType *in_target_variable, DataType start_value, DataType min_value, DataType max_value, void (*on_change_handler)(DataType last_value, DataType new_value)) 
            : NumberControl(label) {
            //NumberControl(label, start_value, min_value, max_value) {
            internal_value = start_value;
            minimum_value = min_value;
            maximum_value = max_value; 
            target_variable = in_target_variable;
            this->on_change_handler = on_change_handler;
        };
        NumberControl(const char* label, DataType (*getter)(), void (*setter)(DataType value), DataType min_value, DataType max_value, void (*on_change_handler)(DataType last_value, DataType new_value)) : 
            NumberControl(label) {
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
        // value doesn't matter, we're just using the datatype to set the default
        virtual DataType get_default_step_for_type(double step) {
            return 0.1;
        }
        virtual DataType get_default_step_for_type(int step) {
            return 1;
        }

        virtual void setReadOnly(bool readOnly = true) {
            this->readOnly = readOnly;
        }

        /*virtual const char *getFormattedValue(DataType value) {
            static char fmt[MENU_C_MAX] = "      ";
            sprintf(fmt, "%i", value);
            return fmt;
        }*/
        virtual const char *getFormattedValue(bool value) {
            static char fmt[MENU_C_MAX] = "      ";
            if (this->debug)
                sprintf(fmt, "%s [bool]", value?"On":"Off");
            else
                sprintf(fmt, "%s", value?"On":"Off");
            return fmt;
        }
        virtual const char *getFormattedValue(int value) {
            static char fmt[MENU_C_MAX] = "      ";
            if (this->debug)
                sprintf(fmt, "%5i [int]", value);
            else
                sprintf(fmt, "%5i", value);
            return fmt;
        }
        virtual const char *getFormattedValue(double value) {
            static char fmt[MENU_C_MAX] = "      ";
            if (this->debug)
                sprintf(fmt, "%3.2f [double]", value);
            else
                sprintf(fmt, "%3.2f", value);

            return fmt;
        }

        virtual const char *getFormattedValue() {
            return this->getFormattedValue((DataType)this->get_current_value());
        }
        virtual const char *getFormattedInternalValue() {
            static char tmp[MAX_LABEL_LENGTH];
            sprintf(tmp, this->getFormattedValue((DataType)this->get_internal_value()));
            return tmp;
        }

        virtual DataType get_internal_value() {
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
            //char tmp[tft->get_c_max()] = "                   ";
            const char *tmp;
            if (this->debug) { Serial.println("did setting tmp"); Serial.flush(); }
            
            if (this->debug) { Serial.printf("NumberControl#display in %s about to do getFormattedValue() ting...\n", this->label); Serial.flush(); }
            if (opened) {
                //tft->printf("value: %*i\n", 4, internal_value);
                tmp = this->getFormattedInternalValue();
                //Serial.printf("NumberControl#display is opened for '%s', formatted value is '%s' (strlen %i)\n", this->label, printable_value, strlen(printable_value));
                //Serial.printf("in opened NumberControl for %s, with internal_value %i, got formattedvalue '%s'\n", this->label, internal_value, this->getFormattedValue(internal_value));
            } else {
                //tft->printf("value: %*i\n", 4, get_current_value()); //*target_variable); //target->transpose);
                //sprintf(tmp, "%s", this->getFormattedValue()); //get_current_value());
                tmp = this->getFormattedValue();
            }
            if (this->debug) { Serial.printf("NumberControl#display in %s just did getFormattedValue() ting!\n", this->label); Serial.flush(); }

            // adjust size dependent on size of formatted value
            if (strlen(tmp)<tft->get_c_max()/2) 
                tft->setTextSize(2);
            else
                tft->setTextSize(1);

            tft->printf(tmp);
            tmp = this->getFormattedExtra();
            if (tmp!=nullptr)
                tft->printf(tmp);
            tft->setTextColor(C_WHITE, BLACK); tft->print((char*)"   ");    // cheap blank
            tft->println();
            if (this->debug) { Serial.printf("NumberControl base display finished in %s\n", label); }

            return tft->getCursorY();
        }

        virtual const char *getFormattedExtra() {
            return nullptr;
        }

        virtual void set_internal_value(DataType value) {
            if (this->debug)  { Serial.printf("NumberControl.set_internal_value(%i)..\n", value); }
            this->internal_value = value;
        }

        virtual void decrease_value() {
            this->set_internal_value(get_internal_value() - this->step);
            if (get_internal_value() < this->minimum_value)
                set_internal_value(this->minimum_value); // = NUM_LOOP_SLOTS_PER_PROJECT-1;
            //project.select_loop_number(ui_selected_loop_number);
        }
        virtual void increase_value() {
            //internal_value += step;
            //Serial.printf("NumberControl#increase_value() with %f and %f?\n", get_internal_value(), this->step);
            this->set_internal_value(get_internal_value() + this->step);
            if (get_internal_value() >= this->maximum_value)
                set_internal_value(this->maximum_value);
        }

        virtual bool knob_left() {
            if (!this->readOnly)
                decrease_value();
            return true;
        }
        virtual bool knob_right() {
            if (!this->readOnly)
                increase_value();
            //project.select_loop_number(internal_value);
            return true;
        }

        virtual void change_value(DataType new_value) {
            if (this->readOnly) 
                return;
            DataType last_value = this->get_current_value();
            Serial.printf("NumberControl#change_value(%f) about to call set_current_value(%f)", (double)new_value, (double)new_value);
            this->set_current_value(new_value);
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
        virtual DataType get_current_value() {
            if (this->debug)  { Serial.printf("About to get_current_value in %s\n", this->label); Serial.flush(); }
            if (this->target_variable!=nullptr)
                return *this->target_variable;
            if (this->getter!=nullptr)
                return this->getter();
            if (this->debug) { Serial.printf("Did get_current_value in %s\n", this->label); Serial.flush(); }

            return 0;
        }

        // override in subclass if need to do something special eg getter/setter
        virtual void set_current_value(DataType value) {
            //this->internal_value = value;
            if (this->target_variable!=nullptr)
                *this->target_variable = value;
            if (this->setter!=nullptr)
                this->setter(value);
        }

        virtual void setStep(DataType step) {
            this->step = step;
        }
};

template<class DataType = int>
class DirectNumberControl : public NumberControl<DataType> {
    public:
    DirectNumberControl(const char* label) : NumberControl<DataType>(label) {};
    DirectNumberControl(const char* label, DataType *in_target_variable, DataType start_value, DataType min_value, DataType max_value, void (*on_change_handler)(DataType last_value, DataType new_value)) 
            : NumberControl<DataType>(label, in_target_variable, start_value, min_value, max_value, on_change_handler) {
    }
    DirectNumberControl(const char* label, DataType (*getter)(), void (*setter)(DataType value), DataType min_value, DataType max_value, void (*on_change_handler)(DataType last_value, DataType new_value))
            : NumberControl<DataType>(label, getter, setter, min_value, max_value, on_change_handler) {
    }

    virtual bool knob_left() override {
        if (this->readOnly) return false;
        Serial.printf("------ DirectNumberControl#knob_left, internal_value=%f\n", (double)this->internal_value);
        this->decrease_value();
        Serial.printf("------ DirectNumberControl#knob_left, about to call change_value(%f)\n", (double)this->internal_value);
        this->change_value(this->internal_value);
        Serial.printf(">------<\n");

        return true;
    }
    virtual bool knob_right() override {
        if (this->readOnly) return false;
        Serial.printf("------ DirectNumberControl#knob_right, internal_value=%f\n", (double)this->internal_value);
        this->increase_value();
        Serial.printf("------ DirectNumberControl#knob_right, about to call change_value(%f)\n", (double)this->internal_value);
        this->change_value(this->internal_value);
        Serial.printf(">------<\n");

        return true;
    }
    virtual bool button_select() override {
        if (this->readOnly) return true;

        this->internal_value = this->get_current_value();// * this->maximum_value; 
        this->change_value(this->internal_value);
        return true;
    }
};


/*template<class DataType>
class DirectDataNumberControl : public NumberControl {
    public:

    void (*on_change_handler)(DataType last_value, DataType new_value) = nullptr;

    DataType (*getter)() = nullptr;
    void (*setter)(DataType value) = nullptr;

    DataType *target_variable = nullptr;
    DataType internal_value = 0.0;
    DataType minimum_value = 0.0;
    DataType maximum_value = 1.0;
    DataType step = 0.1;

    DirectDataNumberControl(const char* label) : NumberControl(label) {};
    DirectDataNumberControl(const char* label, DataType *in_target_variable, DataType start_value, DataType min_value, DataType max_value, void (*on_change_handler)(DataType last_value, int new_value)) 
            : NumberControl(label, in_target_variable, start_value, min_value, max_value, on_change_handler) {
    }
    DirectDataNumberControl(const char* label, DataType (*getter)(), void (*setter)(DataType value), DataType min_value, DataType max_value, void (*on_change_handler)(DataType last_value, int new_value))
            : NumberControl(label, getter, setter, min_value, max_value, on_change_handler) {
    }

    virtual DataType get_internal_value() {
        return this->internal_value;
    }
    // override in subclass if need to do something special eg getter/setter
    virtual void set_current_value(DataType value) {
        //this->internal_value = value;
        if (target_variable!=nullptr)
            *target_variable = value;
        if (setter!=nullptr)
            setter(value);
    }

    virtual void setStep(DataType step) {
        this->step = step;
    }

    virtual void increase_value() {
        this->internal_value = constrain(internal_value + step, this->minimum_value, this->maximum_value);
    }
    virtual void decrease_value() {
        this->internal_value = constrain(internal_value - step, this->minimum_value, this->maximum_value);
    }

    virtual void change_value(DataType new_value) {
        if (readOnly) return;
        int last_value = get_current_value();
        Serial.printf("NumberControl#change_value(%i) about to call set_current_value(%i)", new_value, new_value);
        this->set_current_value(new_value);
        if (on_change_handler!=nullptr) {
            if (this->debug)  { Serial.println("NumberControl calling on_change_handler"); Serial.flush(); }
            on_change_handler(last_value, this->get_internal_value());
            if (this->debug)  { Serial.println("NumberControl after on_change_handler"); Serial.flush(); }
        }
    }

    virtual bool knob_left() override {
        if (readOnly) return false;
        Serial.printf("------ DirectNumberControl#knob_left, internal_value=%i\n", internal_value);
        increase_value();
        Serial.printf("------ DirectNumberControl#knob_left, about to call change_value(%i)\n", internal_value);
        change_value(internal_value);
        Serial.printf(">------<\n");
        //project.select_loop_number(ui_selected_loop_number);
        return true;
    }
    virtual bool knob_right() override {
        if (readOnly) return false;
        Serial.printf("------ DirectNumberControl#knob_right, internal_value=%i\n", internal_value);
        decrease_value();
        Serial.printf("------ DirectNumberControl#knob_right, about to call change_value(%i)\n", internal_value);
        change_value(internal_value);
        Serial.printf(">------<\n");
        //project.select_loop_number(internal_value);
        return true;
    }
    virtual bool button_select() override {
        if (readOnly) return true;
        //this->target->set_transpose(internal_value);
        internal_value = this->get_current_value();// * this->maximum_value; 
        change_value(internal_value);
        return true;
    }
};*/

#endif