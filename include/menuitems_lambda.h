#ifndef MENUITEMS_LAMBDA__INCLUDED
#define MENUITEMS_LAMBDA__INCLUDED

#include "functional-vlpp.h"

#include "menuitems_numbers.h"

template<class DataType>
class LambdaNumberControl : public NumberControl<DataType> {
    public:
    //void(TargetClass::*setter)(DataType) = nullptr;
    //DataType(TargetClass::*getter)() = nullptr;
    //void (*on_change_handler)(DataType last_value, DataType new_value) = nullptr;
    //TargetClass *target_object = nullptr;

    vl::Func<void(DataType)> setter_func;
    vl::Func<DataType(void)> getter_func;

    bool direct = false;

    LambdaNumberControl(const char* label, 
        vl::Func<void(DataType)> setter_func,
        vl::Func<DataType(void)> getter_func,
        void (*on_change_handler)(DataType last_value, DataType new_value) = nullptr,
        bool go_back_on_select = false
    ) : NumberControl<DataType>(label) {
        //this->target_object = target_object;
        this->getter_func = getter_func;
        this->setter_func = setter_func;

        this->on_change_handler = on_change_handler;
        this->minimum_value = (DataType)0;
        this->maximum_value = (DataType)100;
        this->go_back_on_select = go_back_on_select;

        //if (this->target_object!=nullptr && this->getter!=nullptr) 
            this->set_internal_value( getter_func() );
    }
    LambdaNumberControl(const char* label, 
        vl::Func<void(DataType)> setter_func,
        vl::Func<DataType(void)> getter_func,
        void (*on_change_handler)(DataType last_value, DataType new_value),
        DataType minimum_value,
        DataType maximum_value,
        bool go_back_on_select = false,
        bool direct = false
    ) : LambdaNumberControl<DataType>(label, setter_func, getter_func, on_change_handler, go_back_on_select) {
        this->minimum_value = minimum_value;
        this->maximum_value = maximum_value;
        this->direct = direct;
    }

    virtual void on_add() override {
        NumberControl<DataType>::on_add();
        //if (this->target_object!=nullptr && this->getter!=nullptr) 
            this->set_internal_value( getter_func() );
    }

    virtual void increase_value() override {
        NumberControl<DataType>::increase_value();
        if (direct)
            this->change_value(this->internal_value);
    }

    virtual void decrease_value() override {
        NumberControl<DataType>::decrease_value();
        if (direct)
            this->change_value(this->internal_value);
    }

    /*virtual DataType get_internal_value() override {
        return this->internal_value;
    }*/

    /*virtual void set_internal_value(DataType value) override {
        if (this->debug) Serial.printf("ObjectNumberControl.set_internal_value(%i)..\n", value);
        this->internal_value = constrain(value, this->minimum_value, this->maximum_value);
        if (this->debug) Serial.printf("constrained to %i (%i:%i)\n", this->internal_value, this->minimum_value, this->maximum_value);
    }*/

    virtual void change_value(DataType new_value) override {
        if (this->readOnly) return;
        //if (this->debug) { Serial.printf(F("ObjectNumberControl#change_value(%i)..\n"), new_value); Serial_flush(); }
        DataType last_value = this->get_current_value();
        //Serial.println("ObjectNumberControl#change_value about to call set_current_value");
        this->set_current_value(new_value);
        if (this->on_change_handler!=nullptr) {
            //if (this->debug)  { Serial.println(F("ObjectNumberControl calling on_change_handler")); Serial_flush(); }
            (this->on_change_handler)(last_value, this->internal_value);
        }
    }

    // override in subclass if need to do something special eg getter/setter
    virtual DataType get_current_value() override {
        //if (this->target_object!=nullptr && this->getter!=nullptr) {
            //if (this->debug) { Serial.printf("%s: ObjectNumberControl#get_current_value in %s about to call getter\n", this->label); Serial_flush(); }
            //DataType v = (this->target_object->*getter)();
            DataType v = getter_func();
            //if (this->debug) { Serial.printf(F("%s: Called getter and got value %i!\n"), this->label, v); Serial_flush(); }
            return v;
        //}
        
        return (DataType)0;
    }

    // override in subclass if need to do something special eg getter/setter
    virtual void set_current_value(DataType value) override { 
        //this->internal_value = value;
        //if (this->debug) { Serial.printf(F("ObjectNumberControl#set_current_value(%i)\n"), value); Serial_flush(); }
        //if (this->target_object!=nullptr && this->setter!=nullptr) {
            //(this->target_object->*setter)(value);
            setter_func(value);

            char msg[MENU_MESSAGE_MAX];
            //Serial.printf("about to build msg string...\n");
            snprintf(msg, MENU_MESSAGE_MAX, "Set %8s to %s", this->label, this->getFormattedValue(value)); //(int)value);
            //Serial.printf("about to set_last_message!");
            //msg[this->tft->get_c_max()] = '\0'; // limit the string so we don't overflow set_last_message
            menu_set_last_message(msg,GREEN);
        //}
        //if (this->debug) { Serial.println(F("Done.")); Serial_flush(); }
    }
};

class LambdaToggleControl : public MenuItem {
    public:
        vl::Func<void(bool)> setter;
        vl::Func<bool(void)> getter;

        void (*on_change_handler)(bool last_value, bool new_value) = nullptr;

        LambdaToggleControl(
            const char *label, 
            vl::Func<void(bool)> setter,
            vl::Func<bool(void)> getter,
            void (*on_change_handler)(bool last_value, bool new_value) = nullptr
        ) : MenuItem(label) {
            this->setter = setter;
            this->getter = getter;
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
            const char *txt = this->getter() ? label_on : label_off;
            //bool use_small = strlen(txt) <= (max_character_width/2);
            //int textSize = use_small ? 2 : 1;
            int textSize = tft->get_textsize_for_width(txt, max_character_width*tft->characterWidth());
            //if (this->debug) Serial.printf(F("%s:\trenderValue '%s' (len %i) with max_character_width %i got textSize %i\n"), this->label, txt, strlen(txt), max_character_width/2, textSize);
            tft->setTextSize(textSize);
            tft->println(txt);
            return tft->getCursorY();
        }

        virtual bool action_opened() override {
            //if (this->debug) Serial.printf(F("LambdaToggleControl#action_opened on %s\n"), this->label);
            bool value = !this->getter();
            //this->internal_value = !this->internal_value;

            this->setter(value); //(bool)this->internal_value);
            return false;   // don't 'open'
        }
};


#include "menuitems_lambda_selector.h"

#endif
