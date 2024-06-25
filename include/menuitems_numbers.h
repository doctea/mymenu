#ifndef MENUITEM_NUMBERS__INCLUDED
#define MENUITEM_NUMBERS__INCLUDED

#include "Arduino.h"

#include "menu.h"
#include "colours.h"

// type-agnostic ancestor
class NumberControlBase : public MenuItem {
    public:
        NumberControlBase(const char *label) : MenuItem(label) {};

        //virtual int renderValue(bool selected, bool opened, uint16_t max_character_width);
};

// generic control for selecting a number
template<class DataType = int>
class NumberControl : public NumberControlBase {
    public:
        bool debug = false;
        void (*on_change_handler)(DataType last_value, DataType new_value) = nullptr;

        DataType (*getter)() = nullptr;
        void (*setter)(DataType value) = nullptr;

        DataType *target_variable = nullptr;
        DataType internal_value = (DataType)0;
        DataType minimumDataValue = (DataType)0;
        DataType maximumDataValue = (DataType)100;
        DataType step = this->get_default_step_for_type(internal_value);

        float float_mult = 100.0;
        char int_unit = '\0';
        char float_unit = '%';

        bool readOnly = false;
        bool wrap = false;

        bool direct = false;

        uint32_t last_changed_at_decreased = 0, last_changed_at_increased = 0;

        NumberControl(const char* label, bool go_back_on_select = false, bool direct = false) : NumberControlBase(label) {
            this->step = this->get_default_step_for_type((DataType)0);    // setup default step based on our template DataType
            this->go_back_on_select = go_back_on_select;
            this->direct = direct;
        }
        NumberControl(const char* label, DataType start_value, DataType min_value, DataType max_value, bool go_back_on_select = false, bool direct = false) 
            : NumberControl(label, go_back_on_select, direct) 
            //: internal_value(start_value), minimumDataValue(min_value), maximumDataValue(max_value)
            {
            this->internal_value = start_value;
            this->minimumDataValue = min_value;
            this->maximumDataValue = max_value;
        };
        NumberControl(const char* label, DataType *target_variable_, DataType start_value, DataType min_value, DataType max_value, bool go_back_on_select = false, bool direct = false, void (*on_change_handler_)(DataType last_value, DataType new_value) = nullptr) 
            // : target_variable(target_variable_), on_change_handler(on_change_handler_) 
            : NumberControl(label, start_value, min_value, max_value, go_back_on_select, direct)
            {
            this->target_variable = target_variable_;
            this->on_change_handler = on_change_handler_;
        };
        NumberControl(const char* label, DataType (*getter_)(), void (*setter_)(DataType value), DataType min_value, DataType max_value, bool go_back_on_select = false, bool direct = false, void (*on_change_handler_)(DataType last_value, DataType new_value) = nullptr) 
            : NumberControl(label, go_back_on_select, direct) 
            //: getter(getter_), setter(setter_), minimumDataValue(min_value), maximumDataValue(max_value), on_change_handler(on_change_handler_)
            {
            this->getter = getter_;
            this->setter = setter_;

            this->internal_value = getter();
            this->minimumDataValue = min_value;
            this->maximumDataValue = max_value;
            this->on_change_handler = on_change_handler_;
        };

        virtual DataType getMinimumDataValue() {
            return this->minimumDataValue;
        }
        virtual DataType getMaximumDataValue() {
            return this->maximumDataValue;
        }

        // step value passed here doesn't matter -- we're just using the datatype overload to set the default
        constexpr DataType get_default_step_for_type(double step) {
            return (DataType)0.01;
        }
        constexpr DataType get_default_step_for_type(int step) {
            return (DataType)1;
        }
        constexpr DataType get_default_step_for_type(long step) {
            return (DataType)1;
        }
        constexpr DataType get_default_step_for_type(unsigned long step) {
            return (DataType)1;
        }
        /*virtual DataType get_default_step_for_type(unsigned long long step) {
            return 1;
        }*/

        virtual void setStep(DataType step) {
            this->step = step;
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
            return value ? label_on : label_off;
        }
        virtual const char *getFormattedValue(int value) {
            static char fmt[15] = "      ";
            if (this->debug)
                snprintf(fmt, 15, "%-5i [int]", value);
            else
                snprintf(fmt, 15, "%-5i%c", value, this->int_unit);
            return fmt;
        }
        virtual const char *getFormattedValue(uint32_t value) {
            static char fmt[15] = "      ";
            if (this->debug)
                snprintf(fmt, 15, "%-5u [ulong]", (unsigned int) value);
            else
                snprintf(fmt, 15, "%-5u%c", (unsigned int) value, this->int_unit);
            return fmt;
        }
        /*virtual const char *getFormattedValue(unsigned long long value) {
            static char fmt[MENU_C_MAX] = "      ";
            if (this->debug)
                sprintf(fmt, "%-5u [ulong]", (unsigned long long) value);
            else
                sprintf(fmt, "%-5u", (unsigned long long) value);
            return fmt;
        }*/
        virtual const char *getFormattedValue(int32_t value) {
            static char fmt[15] = "      ";
            if (this->debug)
                snprintf(fmt, 15, "%-5i [long]", (int)value);
            else
                snprintf(fmt, 15, "%-5i%c", (int)value, this->int_unit);
            return fmt;
        }
        virtual const char *getFormattedValue(double value) {
            static char fmt[20] = "      ";
            if (this->debug)
                snprintf(fmt, 20, "%-3.2f [double]", value);
            else {
                if (this->float_unit != '\0')
                    snprintf(fmt, 20, "%-3.0f%c", value*this->float_mult, this->float_unit);
                else
                    snprintf(fmt, 20, "%-3.0f", value*this->float_mult);
            }
            return fmt;
        }

        virtual const char *getFormattedValue() {
            return this->getFormattedValue((DataType)this->get_current_value());
        }
        virtual const char *getFormattedInternalValue() {
            static char tmp[MAX_LABEL_LENGTH];
            snprintf(tmp, MAX_LABEL_LENGTH, "%s", this->getFormattedValue((DataType)this->get_internal_value()));
            return tmp;
        }

        virtual DataType get_internal_value() {
            return this->internal_value;
        }

        virtual int display(Coord pos, bool selected, bool opened) override {
            /*if (this->debug) {
                Serial.println(F("NumberControl#display starting!")); Serial_flush();
                Serial.printf(F("NumberControl#display in %s starting\n"), this->label); Serial_flush();
            }*/
            pos.y = header(label, pos, selected, opened);
            //if (this->debug) { Serial.println(F("did header")); Serial_flush(); }
            tft->setCursor(pos.x,pos.y);
            //if (this->debug) { Serial.println(F("did setcursor")); Serial_flush(); }

            //if (this->debug) { Serial.printf(F("NumberControl#display in %s about to do colours...\n"), this->label); Serial_flush(); }
            //colours(opened, opened ? GREEN : C_WHITE, BLACK);
            colours(opened, opened ? GREEN : this->default_fg, this->default_bg);
            //if (this->debug) { Serial.println("did colours"); Serial_flush(); }

            // render the value
            this->renderValue(selected, opened, tft->get_c_max()); //, strlen(tmp)<tft->get_c_max()/2);

            const char *tmp;
            tmp = this->getFormattedExtra();
            if (tmp!=nullptr) {
                tft->printf(tmp);
                //tft->setTextColor(C_WHITE, BLACK); tft->print((char*)"   ");    // cheap blank
                tft->println();
            }
            //if (this->debug) { Serial.printf(F("NumberControl base display finished in %s\n"), label); }

            return tft->getCursorY();
        }

        // just render the value without the rest of the widget, restricting it to the max_character_width characters
        // TODO: actually limit the output to the width (currently it just uses a smaller size if it doesn't fit in the requested number of characters)
        virtual int renderValue(bool selected, bool opened, uint16_t max_character_width) override {
            //if (this->debug) Serial.printf(F("\t\trenderValue() in NumberControl with max_character_width %i\n"), max_character_width);
            const char *tmp = opened ? this->getFormattedInternalValue() : this->getFormattedValue();
            //if (this->debug) { Serial.println("renderValue() did setting tmp"); Serial_flush(); }
            
            //if (this->debug) { Serial.printf("NumberControl#renderValue in %s about to do getFormattedValue() ting...\n", this->label); Serial_flush(); }
            /*if (opened) {
                //tft->printf("value: %*i\n", 4, internal_value);
                tmp = this->getFormattedInternalValue();
                //Serial.printf("NumberControl#display is opened for '%s', formatted value is '%s' (strlen %i)\n", this->label, printable_value, strlen(printable_value));
                //Serial.printf("in opened NumberControl for %s, with internal_value %i, got formattedvalue '%s'\n", this->label, internal_value, this->getFormattedValue(internal_value));
            } else {
                //tft->printf("value: %*i\n", 4, get_current_value()); // *target_variable); //target->transpose);
                //sprintf(tmp, "%s", this->getFormattedValue()); //get_current_value());
                tmp = this->getFormattedValue();
            }*/
            //if (this->debug) { Serial.printf("NumberControl#renderValue in %s just did getFormattedValue() ting!\n", this->label); Serial_flush(); }
            //tmp = opened ? this->getFormattedInternalValue() : this->getFormattedValue();

            // adjust size, dependent on size of formatted value and passed-in max_width
            /*if (strlen(tmp) > max_character_width/2) {
                Serial.printf("%s\t#renderValue string is \t%s (length=%i), max_character_width is\t%i, setting text size to 0\n", this->label, tmp, strlen(tmp), max_character_width);
                tft->setTextSize(0);
            } else {
                Serial.printf("%s\t#renderValue string is \t%s (length=%i), max_character_width is\t%i, setting text size to 1\n", this->label, tmp, strlen(tmp), max_character_width);
                tft->setTextSize(1);
            }*/
            //tft->setTextSize(0);
            colours(false); //, this->default_fg, this->)
            //Serial.printf("renderValue in NumberControl\tfor %s,\tgot default_fg colour %04x\n", this->label, this->default_fg);

            //byte textSize = (strlen(tmp) > max_character_width/2) ? 1 : 2;
            //byte textSize = (strlen(tmp) / max_character_width) + 1;
            //Serial.printf("%s\t#renderValue string is \t%s (length=%i), max_character_width is\t%i, setting text size to\t%i\n", this->label, tmp, strlen(tmp), max_character_width, textSize);
            if (this->debug) {
                Serial.printf("%s\t#renderValue string is \t'%s'\t(length=%i), max_character_width is\t%i, current text width is %i, setting text size to\t%i\n", this->label, tmp, strlen(tmp), max_character_width, tft->currentCharacterWidth(), tft->get_textsize_for_width(tmp, max_character_width*tft->currentCharacterWidth()));
                Serial_flush();
            }
            bool original_wrap_status = tft->isTextWrap();
            int pixel_width = max_character_width*tft->currentCharacterWidth();
            byte textSize = tft->get_textsize_for_width(tmp, pixel_width);
            tft->setTextSize(textSize);
            tft->setTextWrap(false);
            tft->println(tmp);
            tft->setTextWrap(original_wrap_status);

            /*if (this->debug) {
                int characterwidth = tft->currentCharacterWidth();
                tft->setTextSize(0);
                tft->printf("tmp is length %i\n", strlen(tmp));
                tft->printf("pixel_width=%i, tft width=%i\n", pixel_width, tft->width());
                tft->printf("char_width=%i\n", characterwidth);
                tft->printf("tmp='%s'\n", (char*)tmp);
                //tft->println("123456789ABCDEF123456789ABCDEF");
            }*/
            return tft->getCursorY();
        }

        virtual const char *getFormattedExtra() {
            return nullptr;
        }

        virtual void set_internal_value(DataType value) {
            //this->internal_value = value;
            this->internal_value = constrain(value, this->getMinimumDataValue(), this->getMaximumDataValue());
            //if (this->debug) 
            //Serial.printf(F("%s: NumberControl.set_internal_value(%i)\twith constraint (%i:%i) resulted in %i\n"), this->label, value, (int)this->minimumDataValue, (int)this->maximumDataValue, this->internal_value);
        }

        virtual DataType get_current_step_down() {
            // do knob acceleration
            if (last_changed_at_decreased==0)
                return this->step;

            uint32_t time_since_changed = constrain(millis() - this->last_changed_at_decreased, (uint32_t)0, (uint32_t)200);
            return this->get_current_step_generic(time_since_changed);
        }
        virtual DataType get_current_step_up() {
            // do knob acceleration
            if (last_changed_at_increased==0)
                return this->step;

            uint32_t time_since_changed = constrain(millis() - this->last_changed_at_increased, (uint32_t)0, (uint32_t)200);
            return this->get_current_step_generic(time_since_changed);
        }

        virtual DataType get_current_step_generic(uint32_t time_since_changed) {           
            if      (time_since_changed>=200)  return (DataType)  this->step;
            else if (time_since_changed>=100)  return (DataType) (this->step * 2.0f);
            //else if (time_since_changed>=100)  return (DataType) (this->step * 4.0f);
            else if (time_since_changed>=25 )  return (DataType) (this->step * 8.0f);
            else                               return (DataType) (this->step * 10.0f);
        }


        virtual void decrease_value() {
            if (this->get_internal_value()>this->getMinimumDataValue() 
                && get_internal_value() - get_current_step_down() < this->getMaximumDataValue()
            ) {   // so that unsigned datatypes don't wrap back around when they try to go below 0
                this->set_internal_value((DataType)(get_internal_value() - get_current_step_down()));
                last_changed_at_decreased = millis();
                last_changed_at_increased = 0;
            } else if (wrap) {
                this->set_internal_value(this->getMaximumDataValue());
            }
            if (direct)
                this->change_value(this->get_internal_value());
        }
        virtual void increase_value() {
            //Serial.printf("NumberControl '%s'#increase_value with internal value %3.3f and step %3.3f makes %3.3f\n", this->label, get_internal_value(), this->get_current_step(), get_internal_value()+this->get_current_step());
            if (this->get_internal_value()!=this->getMaximumDataValue()) {
                this->set_internal_value((DataType)(get_internal_value() + get_current_step_up()));
                last_changed_at_increased = millis();
                last_changed_at_decreased = 0;
            } else if (wrap) {
                this->set_internal_value(this->getMinimumDataValue());
            }
            if (direct)
                this->change_value(this->get_internal_value());
        }

        virtual bool knob_left() override {
            if (!this->readOnly)
                decrease_value();
            return true;
        }
        virtual bool knob_right() override {
            if (!this->readOnly)
                increase_value();
            return true;
        }

        virtual void change_value(DataType new_value) {
            if (this->readOnly) 
                return;
            DataType last_value = this->get_current_value();
            //Serial.printf("NumberControl#change_value(%f) about to call set_current_value(%f)", (double)new_value, (double)new_value);
            this->set_current_value(new_value);
            //Serial.printf("NumberControl#change_value(%f) after set_current_value(%f) get_current_value got %f\n", (double)new_value, (double)new_value, this->get_current_value());
            if (on_change_handler!=nullptr) {
                //if (this->debug)  { Serial.println(F("NumberControl calling on_change_handler")); Serial_flush(); }
                on_change_handler(last_value, this->get_internal_value());
                //if (this->debug)  { Serial.println(F("NumberControl after on_change_handler")); Serial_flush(); }
            }
        }

        // return true if should exit back to main menu
        virtual bool button_select() override {
            if (readOnly) return true;
            //Serial.printf("NumberControl#button_select calling change_value(%3.3f)\n", this->get_internal_value());
            this->change_value((DataType)this->get_internal_value());

            return go_back_on_select;
        }

        virtual bool action_opened() override {
            if (!readOnly) {
                this->set_internal_value((DataType)this->get_current_value());
            }
            return !readOnly;
        }

        // override in subclass if need to do something special eg getter/setter
        virtual DataType get_current_value() {
            //if (this->debug)  { Serial.printf("About to get_current_value in %s\n", this->label); Serial_flush(); }
            if (this->target_variable!=nullptr)
                return *this->target_variable;
            if (this->getter!=nullptr)
                return this->getter();
            //if (this->debug) { Serial.printf("Did get_current_value in %s\n", this->label); Serial_flush(); }

            return (DataType)0;
        }

        // override in subclass if need to do something special eg getter/setter
        virtual void set_current_value(DataType value) {
            //this->internal_value = value;
            if (this->target_variable!=nullptr)
                *this->target_variable = value;
            if (this->setter!=nullptr)
                this->setter(value);
        }

};

// todo: deprecate DirectNumberControl, in favour of NumberControl with direct = true
template<class DataType = int>
class DirectNumberControl : public NumberControl<DataType> {
    public:

    DirectNumberControl(const char* label) : NumberControl<DataType>(label) {};
    DirectNumberControl(const char* label, DataType *target_variable, DataType start_value, DataType min_value, DataType max_value, void (*on_change_handler)(DataType last_value, DataType new_value) = nullptr) 
            : NumberControl<DataType>(label, target_variable, start_value, min_value, max_value, true, true, on_change_handler) {
    }
    DirectNumberControl(const char* label, DataType (*getter)(), void (*setter)(DataType value), DataType min_value, DataType max_value, void (*on_change_handler)(DataType last_value, DataType new_value) = nullptr)
            : NumberControl<DataType>(label, getter, setter, min_value, max_value, true, true, on_change_handler) {
    }

    /*virtual bool knob_left() override {
        if (this->readOnly) return false;
        //Serial.printf("------ DirectNumberControl#knob_left, internal_value=%f\n", (double)this->internal_value);
        this->decrease_value();
        //Serial.printf("------ DirectNumberControl#knob_left, about to call change_value(%f)\n", (double)this->internal_value);
        //this->change_value(this->internal_value);
        //Serial.printf(">------<\n");

        //this->last_changed_at = millis();
        return true;
    }
    virtual bool knob_right() override {
        if (this->readOnly) return false;
        //Serial.printf(F("------ DirectNumberControl#knob_right, internal_value=%f\n"), (double)this->internal_value);
        this->increase_value();
        //Serial.printf(F("------ DirectNumberControl#knob_right, about to call change_value(%f)\n"), (double)this->internal_value);
        //this->change_value(this->internal_value);
        //Serial.printf(F(">------<\n"));

        //this->last_changed_at = millis();
        return true;
    }
    virtual bool button_select() override {
        if (this->readOnly) return true;

        //Serial.printf("DirectNumberControl#button_select() in %s - value is %i (%3.3f)\n", this->label, this->get_current_value(), this->get_current_value());

        this->change_value(this->get_internal_value());

        return this->go_back_on_select;
    }*/
};

#endif
